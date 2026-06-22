#ifndef NETWORK_READER_H
#define NETWORK_READER_H

#include <string>
#include <vector>
#include <set>
#include <stdexcept>

/**
 * Represents a single isotope as known to the network reader.
 *   element      = lower-case element symbol, e.g. "c"
 *   atomicNumber = Z, e.g. 6 for carbon
 *   atomicWeight = mass number (A), e.g. 12
 *   isotopeName  = full token as it appears in the net file, e.g. "c12"
 */
struct NetIsotope {
    std::string element;
    int atomicNumber;
    int atomicWeight;
    std::string isotopeName;

    // Ordering so NetIsotope can be used as a set/map key: sort by
    // (Z, A), which gives a natural physical ordering.
    bool operator<(const NetIsotope& other) const {
        if (atomicNumber != other.atomicNumber) {
            return atomicNumber < other.atomicNumber;
        }
        return atomicWeight < other.atomicWeight;
    }
};

/**
 * NetworkReader
 *
 * Parses MESA-style reaction network definition files (.net), as
 * used by mesastar.org, and extracts the resulting set of isotopes.
 * Reaction rate specifications are recognized (so they don't
 * interfere with parsing) but are otherwise ignored, since this
 * class is only concerned with the isotope list.
 *
 * Supported net-file syntax:
 *   - include 'other_file.net'
 *       Recursively processes another net file as if its contents
 *       were inserted at this point. May appear anywhere in the
 *       file (top, middle, bottom), and multiple times.
 *   - add_isos(...) / add_isos (...)
 *       Adds each listed isotope to the current set. Isotopes
 *       inside the parentheses may be separated by whitespace,
 *       newlines, and/or commas.
 *   - add_isos_and_reactions(...)
 *       Same as add_isos for isotope purposes (the reaction part is
 *       ignored).
 *   - remove_iso(...) / remove_isos(...)
 *       Removes each listed isotope from the current set, if present.
 *   - add_reaction(...), remove_reaction(...), and other
 *       reaction-related commands are recognized and skipped (their
 *       contents are not interpreted) since reaction rates are not
 *       tracked by this class.
 *   - Comments start with '!' and run to the end of the line.
 *
 * Isotope tokens are of the form <element symbol><mass number>,
 * e.g. "c12", "he4", "fe56", with the special cases "neut" (or
 * "neutron"/"n") and "prot" (or "h1" for protium) handled like any
 * other element/isotope pair already present in the periodic table
 * lookup. (See atomicNumberForSymbol().)
 */
class NetworkReader {
public:
    NetworkReader() = default;

    // Parses the given net file (and recursively, any files it
    // includes), accumulating the isotope set. Calling this again
    // on the same object starts fresh (clear() is called first).
    // Throws std::runtime_error if the top-level file, or any
    // included file, cannot be opened, or if a malformed
    // add_isos/remove_isos/include statement is encountered (e.g.
    // unterminated parentheses or quotes).
    void readFile(const std::string& filename);

    // The resulting isotopes, in the order first added. (Removed
    // isotopes do not appear here.) Use getSortedIsotopes() instead
    // if you want them ordered by (Z, A).
    const std::vector<NetIsotope>& getIsotopes() const { return isotopes_; }

    // Same isotopes, but sorted by (atomic number, atomic weight).
    std::vector<NetIsotope> getSortedIsotopes() const;

    size_t size() const { return isotopes_.size(); }
    bool empty() const { return isotopes_.empty(); }

    // Look up a single isotope by its full name, e.g. "c12".
    // Returns nullptr if not present in the current set.
    const NetIsotope* find(const std::string& isotopeName) const;

    // Look up a single isotope by atomic number (Z) and atomic
    // weight (A). Returns nullptr if not present.
    const NetIsotope* findByZA(int atomicNumber, int atomicWeight) const;

    // Names of files that were processed (the top-level file plus
    // every file pulled in via 'include', in the order opened).
    const std::vector<std::string>& getProcessedFiles() const { return processedFiles_; }

    // Clears all parsed data.
    void clear();

private:
    std::vector<NetIsotope> isotopes_;
    std::vector<std::string> processedFiles_;

    // Guards against infinite recursion from circular includes.
    std::vector<std::string> includeStack_;

    // Reads one file's full contents, strips comments, and appends
    // the cleaned token stream (with command keywords and
    // parenthesized argument lists still distinguishable) to
    // 'tokens'. Handles 'include' statements recursively as they
    // are encountered, inserting the included file's tokens in
    // place. baseDir is the directory of the file doing the
    // including, used to resolve relative include paths.
    void processFile(const std::string& filename, const std::string& baseDir);

    // Adds an isotope (by name) to the current set if not already
    // present. Returns true if added, false if it was a duplicate.
    bool addIsotope(const std::string& isotopeName);

    // Removes an isotope (by name) from the current set if present.
    // Returns true if removed, false if it wasn't present.
    bool removeIsotope(const std::string& isotopeName);

    // Parses a single isotope token like "c12" into element + mass
    // number. Handles the special-case names "neut"/"neutron" (Z=0,
    // A=1) and "prot" (Z=1, A=1) used in some MESA net files.
    // Returns false if the token doesn't match any recognized
    // pattern.
    static bool parseIsotopeName(const std::string& token,
                                  std::string& element,
                                  int& atomicNumber,
                                  int& atomicWeight);

    // Returns the atomic number (Z) for a lower-case element symbol
    // (e.g. "c" -> 6). Returns -1 if the symbol is not recognized.
    static int atomicNumberForSymbol(const std::string& element);

    // Strips '!' comments (to end of line) from raw file text,
    // leaving the rest, including newlines, intact (newlines are
    // kept as plain separators, equivalent to whitespace, for later
    // tokenizing).
    static std::string stripComments(const std::string& text);

    // Resolves an include path against the including file's
    // directory: if 'includePath' is absolute, it is returned
    // unchanged; otherwise it is joined with 'baseDir'.
    static std::string resolveIncludePath(const std::string& includePath,
                                           const std::string& baseDir);

    // Returns the directory portion of a path (everything before
    // the last '/'), or "" if there is no directory component.
    static std::string directoryOf(const std::string& path);
};

#endif // NETWORK_READER_H
