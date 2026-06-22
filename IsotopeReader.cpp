#include "IsotopeReader.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iomanip>

namespace {

// Element symbol (lower-case) -> atomic number (Z), for elements 1-118.
// Stored as a static map built once on first use.
const std::map<std::string, int>& elementSymbolTable() {
    static const std::map<std::string, int> table = {
        {"h", 1},   {"he", 2},  {"li", 3},  {"be", 4},  {"b", 5},
        {"c", 6},   {"n", 7},   {"o", 8},   {"f", 9},   {"ne", 10},
        {"na", 11}, {"mg", 12}, {"al", 13}, {"si", 14}, {"p", 15},
        {"s", 16},  {"cl", 17}, {"ar", 18}, {"k", 19},  {"ca", 20},
        {"sc", 21}, {"ti", 22}, {"v", 23},  {"cr", 24}, {"mn", 25},
        {"fe", 26}, {"co", 27}, {"ni", 28}, {"cu", 29}, {"zn", 30},
        {"ga", 31}, {"ge", 32}, {"as", 33}, {"se", 34}, {"br", 35},
        {"kr", 36}, {"rb", 37}, {"sr", 38}, {"y", 39},  {"zr", 40},
        {"nb", 41}, {"mo", 42}, {"tc", 43}, {"ru", 44}, {"rh", 45},
        {"pd", 46}, {"ag", 47}, {"cd", 48}, {"in", 49}, {"sn", 50},
        {"sb", 51}, {"te", 52}, {"i", 53},  {"xe", 54}, {"cs", 55},
        {"ba", 56}, {"la", 57}, {"ce", 58}, {"pr", 59}, {"nd", 60},
        {"pm", 61}, {"sm", 62}, {"eu", 63}, {"gd", 64}, {"tb", 65},
        {"dy", 66}, {"ho", 67}, {"er", 68}, {"tm", 69}, {"yb", 70},
        {"lu", 71}, {"hf", 72}, {"ta", 73}, {"w", 74},  {"re", 75},
        {"os", 76}, {"ir", 77}, {"pt", 78}, {"au", 79}, {"hg", 80},
        {"tl", 81}, {"pb", 82}, {"bi", 83}, {"po", 84}, {"at", 85},
        {"rn", 86}, {"fr", 87}, {"ra", 88}, {"ac", 89}, {"th", 90},
        {"pa", 91}, {"u", 92},  {"np", 93}, {"pu", 94}, {"am", 95},
        {"cm", 96}, {"bk", 97}, {"cf", 98}, {"es", 99}, {"fm", 100},
        {"md", 101}, {"no", 102}, {"lr", 103}, {"rf", 104}, {"db", 105},
        {"sg", 106}, {"bh", 107}, {"hs", 108}, {"mt", 109}, {"ds", 110},
        {"rg", 111}, {"cn", 112}, {"nh", 113}, {"fl", 114}, {"mc", 115},
        {"lv", 116}, {"ts", 117}, {"og", 118}
    };
    return table;
}

} // anonymous namespace

int IsotopeReader::atomicNumberForSymbol(const std::string& element) {
    std::string lower = element;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return std::tolower(c); });

    const auto& table = elementSymbolTable();
    auto it = table.find(lower);
    return (it != table.end()) ? it->second : 0;
}

void IsotopeReader::clear() {
    entries_.clear();
    parseErrors_.clear();
}

bool IsotopeReader::isSkippableLine(const std::string& line) {
    // Find first non-whitespace character
    size_t pos = line.find_first_not_of(" \t\r\n");
    if (pos == std::string::npos) {
        return true; // blank line
    }
    char c = line[pos];
    return (c == '!' || c == '#');
}

std::string IsotopeReader::stripCommentAndTrim(const std::string& line) {
    // Cut off at the first ! or # (inline comment)
    size_t commentPos = line.find_first_of("!#");
    std::string result = (commentPos == std::string::npos)
                              ? line
                              : line.substr(0, commentPos);

    // Trim leading whitespace
    size_t start = result.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return ""; // nothing but whitespace
    }
    // Trim trailing whitespace
    size_t end = result.find_last_not_of(" \t\r\n");

    return result.substr(start, end - start + 1);
}

bool IsotopeReader::parseIsotopeName(const std::string& token,
                                     std::string& element,
                                     int& atomicWeight) {
    if (token.empty()) {
        return false;
    }

    // Split into leading alphabetic part (element symbol) and
    // trailing numeric part (atomic weight).
    size_t i = 0;
    while (i < token.size() && std::isalpha(static_cast<unsigned char>(token[i]))) {
        ++i;
    }

    if (i == 0 || i == token.size()) {
        // No letters at all, or no digits at all
        return false;
    }

    std::string letters = token.substr(0, i);
    std::string digits = token.substr(i);

    // Verify remainder is all digits
    if (!std::all_of(digits.begin(), digits.end(),
                      [](unsigned char c) { return std::isdigit(c); })) {
        return false;
    }

    // Lower-case the element symbol (file is documented as lower
    // case already, but normalize defensively).
    std::transform(letters.begin(), letters.end(), letters.begin(),
                    [](unsigned char c) { return std::tolower(c); });

    try {
        atomicWeight = std::stoi(digits);
    } catch (const std::exception&) {
        return false;
    }

    element = letters;
    return true;
}

void IsotopeReader::readFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("IsotopeReader: unable to open file: " + filename);
    }

    clear();

    std::string rawLine;
    int lineNumber = 0;

    while (std::getline(infile, rawLine)) {
        ++lineNumber;

        if (isSkippableLine(rawLine)) {
            continue;
        }

        std::string cleaned = stripCommentAndTrim(rawLine);
        if (cleaned.empty()) {
            continue; // line was only an inline comment, or whitespace
        }

        std::istringstream iss(cleaned);
        std::string isotopeToken;
        std::string abundanceToken;

        if (!(iss >> isotopeToken)) {
            parseErrors_.push_back({lineNumber, rawLine, "missing isotope name"});
            continue;
        }

        if (!(iss >> abundanceToken)) {
            parseErrors_.push_back({lineNumber, rawLine, "missing abundance value"});
            continue;
        }

        std::string element;
        int atomicWeight = 0;
        if (!parseIsotopeName(isotopeToken, element, atomicWeight)) {
            parseErrors_.push_back({lineNumber, rawLine,
                                     "could not parse isotope name '" + isotopeToken + "'"});
            continue;
        }

        double abundance = 0.0;
        try {
            size_t consumed = 0;
            abundance = std::stod(abundanceToken, &consumed);
            if (consumed != abundanceToken.size()) {
                parseErrors_.push_back({lineNumber, rawLine,
                                         "trailing characters in abundance value '" +
                                             abundanceToken + "'"});
                continue;
            }
        } catch (const std::exception&) {
            parseErrors_.push_back({lineNumber, rawLine,
                                     "could not parse abundance value '" + abundanceToken + "'"});
            continue;
        }

        IsotopeEntry entry;
        entry.element = element;
        entry.atomicNumber = atomicNumberForSymbol(element);
        entry.atomicWeight = atomicWeight;
        entry.abundance = abundance;
        entry.isotopeName = isotopeToken;

        entries_.push_back(entry);
    }
}

void IsotopeReader::writeFile(const std::string& filename, const std::string & comments) const {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        throw std::runtime_error("IsotopeReader: unable to open file for writing: " + filename);
    }

    outfile << "! Isotope abundance data written by IsotopeReader" << std::endl;
    outfile << "! Format: <isotope> <abundance>" << std::endl;
    if (!comments.empty())
	    outfile << "! " << comments << std::endl;

    for (const auto& entry : entries_) {
        outfile << entry.isotopeName << " "
                << std::scientific << std::setprecision(14) << entry.abundance
                << "\n";
    }

    if (!outfile.good()) {
        throw std::runtime_error("IsotopeReader: error writing to file: " + filename);
    }
}

const std::vector<IsotopeEntry> IsotopeReader::getEntries(const std::string & element) const
{
	int Z = atomicNumberForSymbol(element);
	return this->getEntries(Z);
}
const std::vector<IsotopeEntry> IsotopeReader::getEntries(int atomicNumber) const
{
	std::vector<IsotopeEntry> cResults;
    for (const auto& entry : entries_) {
        if (entry.atomicNumber == atomicNumber) {
        	cResults.push_back(entry);
        }
    }
    return cResults;
}


const IsotopeEntry* IsotopeReader::find(const std::string& isotopeName) const {
    for (const auto& entry : entries_) {
        if (entry.isotopeName == isotopeName) {
            return &entry;
        }
    }
    return nullptr;
}

const IsotopeEntry* IsotopeReader::find(int atomicNumber, int atomicWeight) const {
    for (const auto& entry : entries_) {
        if (entry.atomicNumber == atomicNumber && entry.atomicWeight == atomicWeight) {
            return &entry;
        }
    }
    return nullptr;
}

const IsotopeEntry* IsotopeReader::find(const std::string& element,
                                                  int atomicWeight) const {
    std::string lower = element;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return std::tolower(c); });

    for (const auto& entry : entries_) {
        if (entry.element == lower && entry.atomicWeight == atomicWeight) {
            return &entry;
        }
    }
    return nullptr;
}

bool IsotopeReader::setEntry(const IsotopeEntry& entry) {
    IsotopeEntry toStore = entry;

    // Normalize element symbol to lower case.
    std::transform(toStore.element.begin(), toStore.element.end(),
                    toStore.element.begin(),
                    [](unsigned char c) { return std::tolower(c); });

    // Always (re)derive the atomic number from the element symbol,
    // so callers don't need to supply a correct Z themselves.
    int z = atomicNumberForSymbol(toStore.element);
    toStore.atomicNumber = z;

    // Derive isotopeName if the caller didn't set it.
    if (toStore.isotopeName.empty()) {
        toStore.isotopeName = toStore.element + std::to_string(toStore.atomicWeight);
    }

    // Overwrite an existing entry with the same isotopeName, if present.
    for (auto& existing : entries_) {
        if (existing.isotopeName == toStore.isotopeName) {
            existing = toStore;
            return z != 0;
        }
    }

    // Otherwise add as a new entry.
    entries_.push_back(toStore);
    return z != 0;
}

bool IsotopeReader::setEntry(const std::string& element, int atomicWeight, double abundance) {
    IsotopeEntry entry;
    entry.element = element;
    entry.atomicWeight = atomicWeight;
    entry.abundance = abundance;
    entry.atomicNumber = 0;       // filled in by setEntry(const IsotopeEntry&)
    entry.isotopeName = "";       // derived by setEntry(const IsotopeEntry&)
    return setEntry(entry);
}



bool IsotopeReader::addAbundanceToEntry(const std::string& isotopeName, const double &dAbundance )
{
    for (auto& entry : entries_) {
        if (entry.isotopeName == isotopeName) {
        	entry.abundance += dAbundance;
            return true;
        }
    }
    return false;
}
bool IsotopeReader::addAbundanceToEntry(const std::string& element, int atomicWeight, const double &dAbundance )
{
    std::string lower = element;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return std::tolower(c); });

    for (auto& entry : entries_) {
        if (entry.element == lower && entry.atomicWeight == atomicWeight) {
        	entry.abundance += dAbundance;
            return true;
        }
    }
    return false;
}
bool IsotopeReader::addAbundanceToEntry(int atomicNumber, int atomicWeight, const double &dAbundance )
{

    for (auto& entry : entries_) {
        if (entry.atomicNumber == atomicNumber && entry.atomicWeight == atomicWeight) {
        	entry.abundance += dAbundance;
            return true;
        }
    }
    return false;
}


double IsotopeReader::totalXAbundance() const {
    double total = 0.0;
    for (const auto& entry : entries_) {
    	if (entry.atomicNumber == 1)
	        total += entry.abundance;
    }
    return total;
}
double IsotopeReader::totalYAbundance() const {
    double total = 0.0;
    for (const auto& entry : entries_) {
    	if (entry.atomicNumber == 2)
	        total += entry.abundance;
    }
    return total;
}
double IsotopeReader::totalZAbundance() const {
    double total = 0.0;
    for (const auto& entry : entries_) {
    	if ((entry.atomicNumber != 1) && (entry.atomicNumber != 2))
	        total += entry.abundance;
    }
    return total;
}
double IsotopeReader::totalAbundance() const {
    double total = 0.0;
    for (const auto& entry : entries_) {
        total += entry.abundance;
    }
    return total;
}
