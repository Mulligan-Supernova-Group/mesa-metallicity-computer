#include "IsotopeReader.h"
#include "NetworkReader.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cmath>


int main(int i_iArgCount, char ** i_lppArgValues)
{
    std::string sReadFile;
    std::string sOutFile;
    std::string sNetfile;
    double dX = -1;
    double dY = -1;
    double dZ = -1;
    
	IsotopeReader 	cAbundancereader;
	NetworkReader	cNetReader;
	IsotopeReader	cOutput;
	
	bool bDump = false;
	bool bBestGuess = false;
	
	// read command line options and extract the input file name, output file name (if specified) and other options.
	// options are:
	//		-X, -x: hydrogen fraction by mass
	//		-Y, -y: helium fraction by mass
	//		-Z, -z: metals fractions by mass
    //		-I, -i: path to .net file containing the list of isotopes that should be included
    //		-O, -o: path for output
    //		--dump: dump isotopes that aren't in the net into the heaviest isotope
    //		--best: dump isotopes that aren't in the net into the nearest element or isotope
    //		if --dump and --best are NOT selected, the default behavior is to renormalize the metal abundances
    //		to achieve the desired X, Y, and Z
    
    for (int iIdx = 1; iIdx < i_iArgCount; iIdx++)
    {
    	if (i_lppArgValues[iIdx][0] == '-') // indicates flag
    	{
    		if (i_lppArgValues[iIdx][1] == 'X' || i_lppArgValues[iIdx][1] == 'x')
    		{
    			dX = atof(&i_lppArgValues[iIdx][3]);
    		}
    		else if (i_lppArgValues[iIdx][1] == 'Y' || i_lppArgValues[iIdx][1] == 'y')
    		{
    			dY = atof(&i_lppArgValues[iIdx][3]);
    		}
    		else if (i_lppArgValues[iIdx][1] == 'Z' || i_lppArgValues[iIdx][1] == 'z')
    		{
    			dZ = atof(&i_lppArgValues[iIdx][3]);
    		}
    		else if (i_lppArgValues[iIdx][1] == 'I' || i_lppArgValues[iIdx][1] == 'i')
    		{
    		    if (i_lppArgValues[iIdx][3] == '\"' || i_lppArgValues[iIdx][3] == '\'')
	    			sNetfile.copy(i_lppArgValues[iIdx],strlen(i_lppArgValues[iIdx]) - 5,4);
	    		else
	    			sNetfile.copy(i_lppArgValues[iIdx],strlen(i_lppArgValues[iIdx]) - 3,3);
    		}
    		else if (i_lppArgValues[iIdx][1] == 'O' || i_lppArgValues[iIdx][1] == 'o')
    		{
    		    if (i_lppArgValues[iIdx][3] == '\"' || i_lppArgValues[iIdx][3] == '\'')
	    			sOutFile.copy(i_lppArgValues[iIdx],strlen(i_lppArgValues[iIdx]) - 5,4);
	    		else
	    			sOutFile.copy(i_lppArgValues[iIdx],strlen(i_lppArgValues[iIdx]) - 3,3);
    		}
    		else if (strcmp(i_lppArgValues[iIdx],"--dump") == 0)
    		{
    			bDump = true;
    		}
    		else if (strcmp(i_lppArgValues[iIdx],"--best") == 0)
    		{
    			bBestGuess = true;
    		}
    	}
    	else
    	{
		    if (i_lppArgValues[iIdx][0] == '\"' || i_lppArgValues[iIdx][0] == '\'')
    			sReadFile.copy(i_lppArgValues[iIdx],strlen(i_lppArgValues[iIdx]) - 2,1);
    		else
    			sReadFile.copy(i_lppArgValues[iIdx],strlen(i_lppArgValues[iIdx]),0);
    	}
    }
    
    if (dX != -1 && dY != -1 && dZ != -1)
    {
		double dSum = dX + dY + dZ; // check and 
		if (fabs(dSum - 1.0) > 0.0001)
		{
			std::cerr << "X + Y + Z does not add up to 1" << std::endl;
			return 1;
		}
	}
	
    try {
        cAbundancereader.readFile(sReadFile);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
	if (!sNetfile.empty()) // use network specified isotopes instead of the list in the abundance file
	{
		try {
		    cNetReader.readFile(sNetfile);
		} catch (const std::exception& ex) {
		    std::cerr << "Error: " << ex.what() << std::endl;
		    return 1;
		}
		
		std::vector<NetIsotope> cNetIsotopes = cNetReader.getIsotopes()
		// parse the netfile and get abundances for entries specified in the net file
		for (auto netIter = cNetIsotopes.cbegin(); netIter != cNetIsotopes.cend(); netIter++)
		{
			IsotopeEntry	cEntry;
			IsotopeEntry	cAbundanceEntry;
			cEntry.element = netIter->element;
			cEntry.atomicNumber = netIter->atomicNumber;
			cEntry.atomicWeight = netIter->atomicWeight;
			cEntry.isotopeName = netIter->isotopeName;
			
			IsotopeEntry pcAbundanceEntry = cAbundancereader.find(netIter->isotopeName);
			if (pcAbundanceEntry != nullptr)
				cEntry.abundance = pcAbundanceEntry.abundance;
			else
				cEntry.abundance = 0.0;
				
			cOutput.setEntry(cEntry);
		}
		// now go through the original abundance file and deal with entries that aren't in the network file.
		//@@TODO
	}
	else
	{
		// copy the abundance file to the output file
		//@@TODO
	}
	
	// renormalize and adjust metallicity
	//@@TODO
	
	// output results
	//@@TODO

	return 0;
}
