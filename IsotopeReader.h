#ifndef ISOTOPE_READER_H
#define ISOTOPE_READER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

/**
 * Represents a single isotope entry parsed from the data file.
 * Example line: "u235 7.200000E-03"
 *   element = "u"
 *   atomicNumber = 92
 *   atomicWeight = 235
 *   abundance = 7.2e-3
 */
struct IsotopeEntry {
    std::string element;      // lower-case element symbol, e.g. "u"
    int atomicNumber;         // Z, e.g. 92 for uranium
    int atomicWeight;         // mass number (A), e.g. 235
    double abundance;         // abundance value
    std::string isotopeName;  // full token as read, e.g. "u235"
};

/**
 * IsotopeReader
 *
 * Reads a text file containing isotope/abundance data.
 *
 * File format:
 *   - Each data line starts in column 1 with an isotope name:
 *       lowercase element symbol immediately followed by the
 *       atomic weight (mass number), e.g. "u235", "pu239", "h1".
 *   - Followed by whitespace, then the abundance value, which may
 *     be in engineering/scientific notation (e.g. 7.2E-03, 1.0e+00,
 *     0.0072, etc.)
 *   - Blank lines are ignored.
 *   - Comment lines start with '!' or '#' (leading whitespace before
 *     the comment character is allowed) and are ignored entirely.
 *   - Inline trailing comments (e.g. "u235 7.2E-03 ! weapons grade")
 *     are also stripped.
 */
class IsotopeReader {
public:
    IsotopeReader() = default;

    // Reads and parses the given file. Throws std::runtime_error on
    // I/O failure. Parsing errors on individual lines are collected
    // and accessible via getParseErrors() rather than thrown, so a
    // single malformed line doesn't abort the whole read.
    void readFile(const std::string& filename);

    // Writes all current entries out to the given filename, one per
    // line, in the same "<isotopeName> <abundance>" format that
    // readFile() expects (isotopeName starting in column 1, abundance
    // in scientific notation). A header comment line is written
    // first. Entries are written in the order returned by
    // getEntries(). Throws std::runtime_error on I/O failure.
    void writeFile(const std::string& filename) const;

    // Access to parsed results
    const std::vector<IsotopeEntry>& getEntries() const { return entries_; }
    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    // Look up a single isotope by its full name, e.g. "u235"
    // Returns nullptr if not found.
    const IsotopeEntry* find(const std::string& isotopeName) const;

    // Look up a single isotope by atomic number (Z) and atomic weight (A).
    // Returns nullptr if not found.
    const IsotopeEntry* findByZA(int atomicNumber, int atomicWeight) const;

    // Look up a single isotope by element symbol (case-insensitive,
    // e.g. "U" or "u") and atomic weight (A).
    // Returns nullptr if not found.
    const IsotopeEntry* findBySymbolA(const std::string& element, int atomicWeight) const;

    // Adds a new entry or overwrites an existing one with the same
    // isotopeName. The element's atomic number (Z) is filled in
    // automatically from the element symbol if not already known;
    // if the element symbol is not recognized, atomicNumber is set
    // to 0 and this function returns false (the entry is still
    // stored). Returns true on success.
    bool setEntry(const IsotopeEntry& entry);

    // Convenience overload: builds an IsotopeEntry from its parts
    // (isotopeName is derived as element + atomicWeight) and stores
    // it via setEntry().
    bool setEntry(const std::string& element, int atomicWeight, double abundance);

    // Sum of all abundances (useful sanity check; often should be ~1.0)
    double totalAbundance() const;

    // Lines that failed to parse, with line number and reason.
    struct ParseError {
        int lineNumber;
        std::string lineText;
        std::string reason;
    };
    const std::vector<ParseError>& getParseErrors() const { return parseErrors_; }
    bool hasErrors() const { return !parseErrors_.empty(); }

    // Clears all parsed data and errors.
    void clear();

private:
    std::vector<IsotopeEntry> entries_;
    std::vector<ParseError> parseErrors_;

    // Returns true if the line is blank or a full-line comment.
    static bool isSkippableLine(const std::string& line);

    // Strips inline comments (anything from ! or # onward) and
    // trims leading/trailing whitespace.
    static std::string stripCommentAndTrim(const std::string& line);

    // Parses a single isotope token like "u235" into element + weight.
    // Returns false if the token doesn't match the expected pattern.
    static bool parseIsotopeName(const std::string& token,
                                  std::string& element,
                                  int& atomicWeight);

    // Returns the atomic number (Z) for a given lower-case element
    // symbol (e.g. "u" -> 92). Returns 0 if the symbol is not
    // recognized.
    static int atomicNumberForSymbol(const std::string& element);
};

#endif // ISOTOPE_READER_H
