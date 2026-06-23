Mesa-metallicity-computer is a tool for creating a custom abundance data set based on the standard abundances (...) 
and accounting for the selected isotopes included in the reaction network. 

This tool allows for specification of the X, Y, and Z fractions and allows choice of how to handle isotopes that are part of the
base abundance but are not part of the reaction network.

The three methods are: 
- Normalization
  -- The same method as is used in mesa: excluded isotopes are omitted and the included isotopes are normalized to achieve the selected X, Y, and Z.
- Dump into heaviest isotope [--dump]
  -- This method is an option in mesa as well: all isotopes that are part of the base abundance but not part of the network will be included in whatever isotope in the network has the largest atomic weight
- Best guess [--best]
  -- This method will try to mix isotopes that are part of the base abundance into isotopes or elements that are in the network and most similar to the missing isotope; The first choice is to mix the missing isotopes into other isotopes of the same element, distributing the missing isotope equally among the other isotopes. If there are no isotopes of the same element, then the missing isoptoic abundance will be added into the nearest element by atomic number. If there are two nearest elements (e.g. Co is missing from the net, but the net includes Ni and Fe), then the abudnance of the missing isotope will be evenly mixed between the isotopes of the elements present.

*Prerequisites
- A c++ compiler
- automake (typically installed on any unix/linux/MacOS system)
  
*Installing
After cloning the repository, do
  make

The binaries will be found in the mesa-metallicity-computer/bin folder. Add this folder to your PATH or copy the mesa-metallicity-computer executable to the directory where you swish to use it.
