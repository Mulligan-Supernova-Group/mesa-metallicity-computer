#include "NetworkReader.h"

#include <iostream>
#include <iomanip>

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : "my_test.net";

    NetworkReader reader;
    try {
        reader.readFile(filename);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "Processed files (in order):\n";
    for (const auto& f : reader.getProcessedFiles()) {
        std::cout << "  " << f << "\n";
    }

    std::cout << "\nFinal isotope set (" << reader.size() << " isotopes), sorted by (Z, A):\n";
    std::cout << std::left
               << std::setw(12) << "Isotope"
               << std::setw(10) << "Element"
               << std::setw(6)  << "Z"
               << "A\n";

    for (const auto& iso : reader.getSortedIsotopes()) {
        std::cout << std::left
                   << std::setw(12) << iso.isotopeName
                   << std::setw(10) << iso.element
                   << std::setw(6)  << iso.atomicNumber
                   << iso.atomicWeight << "\n";
    }

    // Spot checks
    std::cout << "\nSpot checks:\n";
    std::cout << "  find(\"c13\")  -> " << (reader.find("c13") ? "present" : "absent (removed)") << "\n";
    std::cout << "  find(\"n15\")  -> " << (reader.find("n15") ? "present" : "absent (removed)") << "\n";
    std::cout << "  find(\"p31\")  -> " << (reader.find("p31") ? "present" : "absent (removed)") << "\n";
    std::cout << "  find(\"c12\")  -> " << (reader.find("c12") ? "present" : "absent") << "\n";

    const NetIsotope* o16 = reader.findByZA(8, 16);
    if (o16) {
        std::cout << "  findByZA(8, 16) -> " << o16->isotopeName << "\n";
    }

    return 0;
}
