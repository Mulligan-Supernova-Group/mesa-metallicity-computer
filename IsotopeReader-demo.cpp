#include "IsotopeReader.h"

#include <iostream>
#include <iomanip>

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : "isotopes.dat";

    IsotopeReader reader;
    try {
        reader.readFile(filename);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "Parsed " << reader.size() << " isotope entries:\n";
    std::cout << std::left
               << std::setw(12) << "Isotope"
               << std::setw(10) << "Element"
               << std::setw(6)  << "Z"
               << std::setw(8)  << "Mass"
               << "Abundance\n";

    for (const auto& e : reader.getEntries()) {
        std::cout << std::left
                   << std::setw(12) << e.isotopeName
                   << std::setw(10) << e.element
                   << std::setw(6)  << e.atomicNumber
                   << std::setw(8)  << e.atomicWeight
                   << std::scientific << std::setprecision(6) << e.abundance
                   << "\n";
    }

    std::cout << std::defaultfloat
               << "\nTotal abundance (all entries summed): "
               << reader.totalAbundance() << "\n";

    if (reader.hasErrors()) {
        std::cout << "\nParse errors:\n";
        for (const auto& err : reader.getParseErrors()) {
            std::cout << "  Line " << err.lineNumber << ": \"" << err.lineText
                       << "\" -- " << err.reason << "\n";
        }
    }

    // Example lookup
    const IsotopeEntry* u235 = reader.find("u235");
    if (u235) {
        std::cout << "\nLookup u235 -> abundance = " << std::scientific
                   << u235->abundance << "\n";
    }

    // Lookup by atomic number (Z) and atomic weight (A)
    const IsotopeEntry* byZA = reader.find(92, 238); // uranium-238
    if (byZA) {
        std::cout << "Lookup Z=92, A=238 -> " << byZA->isotopeName
                   << ", abundance = " << byZA->abundance << "\n";
    }

    // Lookup by element symbol and atomic weight
    const IsotopeEntry* bySymbol = reader.find("Fe", 56); // case-insensitive
    if (bySymbol) {
        std::cout << "Lookup symbol=Fe, A=56 -> " << bySymbol->isotopeName
                   << ", Z = " << bySymbol->atomicNumber
                   << ", abundance = " << bySymbol->abundance << "\n";
    }

    // setEntry: add a brand new isotope not in the file
    reader.setEntry("u", 236, 1.0e-09);
    const IsotopeEntry* u236 = reader.find("u236");
    if (u236) {
        std::cout << "\nAfter setEntry, u236 -> Z = " << u236->atomicNumber
                   << ", abundance = " << u236->abundance << "\n";
    }

    // setEntry: overwrite an existing isotope's abundance
    reader.setEntry("u", 235, 7.5e-03);
    const IsotopeEntry* u235updated = reader.find("u235");
    if (u235updated) {
        std::cout << "After overwrite, u235 -> abundance = "
                   << u235updated->abundance << "\n";
    }

    // Write the current entries out to a new file
    const std::string outFilename = "isotopes_out.dat";
    try {
        reader.writeFile(outFilename);
        std::cout << "\nWrote " << reader.size() << " entries to "
                   << outFilename << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error writing file: " << ex.what() << std::endl;
        return 1;
    }

    // Round-trip check: read the written file back in and confirm
    // it parses cleanly with no errors.
    IsotopeReader roundTrip;
    try {
        roundTrip.readFile(outFilename);
        std::cout << "Round-trip read back " << roundTrip.size()
                   << " entries, " << roundTrip.getParseErrors().size()
                   << " parse errors.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error reading back file: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
