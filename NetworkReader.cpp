#include "NetworkReader.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <map>
#include <unordered_set>

namespace {

// Element symbol (lower-case) -> atomic number (Z), for elements 1-118.
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

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Lookup helpers
// ---------------------------------------------------------------------------

int NetworkReader::atomicNumberForSymbol(const std::string& element) {
    const auto& table = elementSymbolTable();
    auto it = table.find(toLower(element));
    return (it != table.end()) ? it->second : -1;
}

bool NetworkReader::parseIsotopeName(const std::string& token,
                                      std::string& element,
                                      int& atomicNumber,
                                      int& atomicWeight) {
    std::string lower = toLower(token);
    if (lower.empty()) {
        return false;
    }

    // Special-case names used in MESA net files that don't fit the
    // <symbol><number> pattern.
    if (lower == "neut" || lower == "neutron") {
        element = "n";
        atomicNumber = 0;
        atomicWeight = 1;
        return true;
    }
    if (lower == "prot" || lower == "proton") {
        element = "h";
        atomicNumber = 1;
        atomicWeight = 1;
        return true;
    }

    // General case: split into leading alphabetic part (element
    // symbol) and trailing numeric part (mass number).
    size_t i = 0;
    while (i < lower.size() && std::isalpha(static_cast<unsigned char>(lower[i]))) {
        ++i;
    }

    if (i == 0 || i == lower.size()) {
        return false; // no letters, or no digits
    }

    std::string letters = lower.substr(0, i);
    std::string digits = lower.substr(i);

    if (!std::all_of(digits.begin(), digits.end(),
                      [](unsigned char c) { return std::isdigit(c); })) {
        return false;
    }

    int z = atomicNumberForSymbol(letters);
    if (z < 0) {
        return false; // unrecognized element symbol
    }

    int a = 0;
    try {
        a = std::stoi(digits);
    } catch (const std::exception&) {
        return false;
    }

    element = letters;
    atomicNumber = z;
    atomicWeight = a;
    return true;
}

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

std::string NetworkReader::directoryOf(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return "";
    }
    return path.substr(0, pos + 1); // keep trailing slash
}

std::string NetworkReader::resolveIncludePath(const std::string& includePath,
                                               const std::string& baseDir) {
    if (!includePath.empty() && includePath[0] == '/') {
        return includePath; // already absolute
    }
    return baseDir + includePath;
}

// ---------------------------------------------------------------------------
// Comment stripping
// ---------------------------------------------------------------------------

std::string NetworkReader::stripComments(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    bool inComment = false;
    for (char c : text) {
        if (c == '!') {
            inComment = true;
            continue;
        }
        if (c == '\n') {
            inComment = false; // comment ends at end of line
            result.push_back(c);
            continue;
        }
        if (!inComment) {
            result.push_back(c);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Set manipulation
// ---------------------------------------------------------------------------

bool NetworkReader::addIsotope(const std::string& isotopeName) {
    std::string lower = toLower(isotopeName);

    for (const auto& iso : isotopes_) {
        if (iso.isotopeName == lower) {
            return false; // already present; add_isos is a set-union op
        }
    }

    std::string element;
    int z = 0, a = 0;
    if (!parseIsotopeName(lower, element, z, a)) {
        throw std::runtime_error("NetworkReader: could not parse isotope name '" +
                                  isotopeName + "'");
    }

    NetIsotope iso;
    iso.element = element;
    iso.atomicNumber = z;
    iso.atomicWeight = a;
    iso.isotopeName = lower;
    isotopes_.push_back(iso);
    return true;
}

bool NetworkReader::removeIsotope(const std::string& isotopeName) {
    std::string lower = toLower(isotopeName);
    auto it = std::find_if(isotopes_.begin(), isotopes_.end(),
                            [&lower](const NetIsotope& iso) {
                                return iso.isotopeName == lower;
                            });
    if (it == isotopes_.end()) {
        return false;
    }
    isotopes_.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// Main parsing logic
// ---------------------------------------------------------------------------

void NetworkReader::clear() {
    isotopes_.clear();
    processedFiles_.clear();
    includeStack_.clear();
}

void NetworkReader::readFile(const std::string& filename) {
    clear();
    processFile(filename, directoryOf(filename));
}

void NetworkReader::processFile(const std::string& filename, const std::string& baseDir) {
    // Guard against circular includes.
    if (std::find(includeStack_.begin(), includeStack_.end(), filename) != includeStack_.end()) {
        throw std::runtime_error("NetworkReader: circular include detected for file: " +
                                  filename);
    }

    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("NetworkReader: unable to open net file: " + filename);
    }

    std::ostringstream ss;
    ss << infile.rdbuf();
    std::string contents = ss.str();
    infile.close();

    processedFiles_.push_back(filename);
    includeStack_.push_back(filename);

    contents = stripComments(contents);

    // Walk through the cleaned text, recognizing commands one at a
    // time. We scan for the next "word" (a maximal run of
    // non-whitespace, non-parenthesis, non-quote, non-comma
    // characters) and dispatch based on what it is.
    size_t pos = 0;
    const size_t n = contents.size();

    auto skipWhitespaceAndCommas = [&](size_t p) {
        while (p < n && (isWhitespace(contents[p]) || contents[p] == ',')) {
            ++p;
        }
        return p;
    };

    while (pos < n) {
        pos = skipWhitespaceAndCommas(pos);
        if (pos >= n) {
            break;
        }

        // Read the next bare word (command name), up to whitespace,
        // '(', or end of input. Note: a quote character can't start
        // a bare word in valid syntax (include is the only
        // quote-using command and it's handled via its own keyword).
        size_t wordStart = pos;
        while (pos < n && !isWhitespace(contents[pos]) && contents[pos] != '(') {
            ++pos;
        }
        std::string word = toLower(contents.substr(wordStart, pos - wordStart));

        if (word.empty()) {
            // Stray character (e.g. an unmatched ')' or other
            // stray symbol); skip it and continue.
            ++pos;
            continue;
        }

        if (word == "include") {
            // Expect: include 'filename'
            pos = skipWhitespaceAndCommas(pos);
            if (pos >= n || (contents[pos] != '\'' && contents[pos] != '"')) {
                throw std::runtime_error(
                    "NetworkReader: expected quoted filename after 'include' in " + filename);
            }
            char quoteChar = contents[pos];
            ++pos; // skip opening quote
            size_t nameStart = pos;
            while (pos < n && contents[pos] != quoteChar) {
                ++pos;
            }
            if (pos >= n) {
                throw std::runtime_error(
                    "NetworkReader: unterminated quote in 'include' statement in " + filename);
            }
            std::string includedName = contents.substr(nameStart, pos - nameStart);
            ++pos; // skip closing quote

            std::string resolvedPath = resolveIncludePath(includedName, baseDir);
            processFile(resolvedPath, directoryOf(resolvedPath));
            continue;
        }

        if (word == "add_isos" || word == "add_iso" ||
            word == "add_isos_and_reactions") {
            pos = skipWhitespaceAndCommas(pos);
            if (pos >= n || contents[pos] != '(') {
                throw std::runtime_error("NetworkReader: expected '(' after '" + word +
                                          "' in " + filename);
            }
            ++pos; // skip '('

            while (true) {
                pos = skipWhitespaceAndCommas(pos);
                if (pos >= n) {
                    throw std::runtime_error("NetworkReader: unterminated '" + word +
                                              "(' in " + filename);
                }
                if (contents[pos] == ')') {
                    ++pos; // skip ')'
                    break;
                }
                size_t isoStart = pos;
                while (pos < n && !isWhitespace(contents[pos]) &&
                       contents[pos] != ',' && contents[pos] != ')') {
                    ++pos;
                }
                std::string isoToken = contents.substr(isoStart, pos - isoStart);
                if (!isoToken.empty()) {
                    addIsotope(isoToken);
                }
            }
            continue;
        }

        if (word == "remove_iso" || word == "remove_isos") {
            pos = skipWhitespaceAndCommas(pos);
            if (pos >= n || contents[pos] != '(') {
                throw std::runtime_error("NetworkReader: expected '(' after '" + word +
                                          "' in " + filename);
            }
            ++pos; // skip '('

            while (true) {
                pos = skipWhitespaceAndCommas(pos);
                if (pos >= n) {
                    throw std::runtime_error("NetworkReader: unterminated '" + word +
                                              "(' in " + filename);
                }
                if (contents[pos] == ')') {
                    ++pos; // skip ')'
                    break;
                }
                size_t isoStart = pos;
                while (pos < n && !isWhitespace(contents[pos]) &&
                       contents[pos] != ',' && contents[pos] != ')') {
                    ++pos;
                }
                std::string isoToken = contents.substr(isoStart, pos - isoStart);
                if (!isoToken.empty()) {
                    removeIsotope(isoToken);
                }
            }
            continue;
        }

        // Any other command we don't specifically care about (e.g.
        // add_reaction(...), remove_reaction(...), or anything else
        // followed by a parenthesized argument list): skip over its
        // argument list without interpreting the contents, since
        // reaction rates are explicitly out of scope.
        pos = skipWhitespaceAndCommas(pos);
        if (pos < n && contents[pos] == '(') {
            // Skip a balanced parenthesized group, respecting
            // quoted strings so a ')' inside a quote isn't
            // mistaken for the closing paren.
            int depth = 0;
            do {
                if (pos >= n) {
                    throw std::runtime_error(
                        "NetworkReader: unterminated '(' for command '" + word +
                        "' in " + filename);
                }
                char c = contents[pos];
                if (c == '\'' || c == '"') {
                    char q = c;
                    ++pos;
                    while (pos < n && contents[pos] != q) {
                        ++pos;
                    }
                    if (pos < n) {
                        ++pos; // skip closing quote
                    }
                    continue;
                }
                if (c == '(') {
                    ++depth;
                } else if (c == ')') {
                    --depth;
                }
                ++pos;
            } while (depth > 0);
        }
        // If 'word' wasn't followed by '(' at all, it's a bare
        // token we don't recognize (e.g. a stray identifier); just
        // move on, having already consumed it above.
    }

    includeStack_.pop_back();
}

// ---------------------------------------------------------------------------
// Query interface
// ---------------------------------------------------------------------------

std::vector<NetIsotope> NetworkReader::getSortedIsotopes() const {
    std::vector<NetIsotope> sorted = isotopes_;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

const NetIsotope* NetworkReader::find(const std::string& isotopeName) const {
    std::string lower = toLower(isotopeName);
    for (const auto& iso : isotopes_) {
        if (iso.isotopeName == lower) {
            return &iso;
        }
    }
    return nullptr;
}

const NetIsotope* NetworkReader::findByZA(int atomicNumber, int atomicWeight) const {
    for (const auto& iso : isotopes_) {
        if (iso.atomicNumber == atomicNumber && iso.atomicWeight == atomicWeight) {
            return &iso;
        }
    }
    return nullptr;
}
