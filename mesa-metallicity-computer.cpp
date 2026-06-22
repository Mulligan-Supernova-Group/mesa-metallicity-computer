#include "IsotopeReader.h"
#include "NetworkReader.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cmath>


int main(int i_iArgCount, char ** i_lppArgValues)
{
    std::string sReadFile;
    std::string sOutFile = "results.dat";
    std::string sNetfile;
    double dX = -1;
    double dY = -1;
    double dZ = -1;
    
	IsotopeReader 	cAbundancereader;
	NetworkReader	cNetReader;
	IsotopeReader	cOutput;
	
	bool bDump = false;
	bool bBestGuess = false;
	const IsotopeEntry * pHeaviestIsotope = nullptr;
	
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
    		else if (i_lppArgValues[iIdx][1] == 'I' || i_lppArgValues[iIdx][1] == 'i' || i_lppArgValues[iIdx][1] == 'N' || i_lppArgValues[iIdx][1] == 'n')
    		{
    			sNetfile = &(i_lppArgValues[iIdx][3]);
    		}
    		else if (i_lppArgValues[iIdx][1] == 'O' || i_lppArgValues[iIdx][1] == 'o')
    		{
    			sOutFile = &(i_lppArgValues[iIdx][3]);
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
   			sReadFile = i_lppArgValues[iIdx];
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
		
		std::vector<NetIsotope> cNetIsotopes = cNetReader.getIsotopes();
		// parse the netfile and get abundances for entries specified in the net file
		for (auto netIter = cNetIsotopes.cbegin(); netIter != cNetIsotopes.cend(); netIter++)
		{
			IsotopeEntry	cEntry;
			IsotopeEntry	cAbundanceEntry;
			cEntry.element = netIter->element;
			cEntry.atomicNumber = netIter->atomicNumber;
			cEntry.atomicWeight = netIter->atomicWeight;
			cEntry.isotopeName = netIter->isotopeName;
			
			const IsotopeEntry * pcAbundanceEntry = cAbundancereader.find(netIter->isotopeName);
			if (pcAbundanceEntry != nullptr)
				cEntry.abundance = pcAbundanceEntry->abundance;
			else
				cEntry.abundance = 0.0;
				
			cOutput.setEntry(cEntry);
			
			if (pHeaviestIsotope == nullptr || netIter->atomicWeight > pHeaviestIsotope->atomicWeight)
			{
				pHeaviestIsotope = cOutput.find(netIter->isotopeName);
			}
		}
		// now go through the original abundance file and deal with entries that aren't in the network file.
		if (bDump || bBestGuess)
		{ // don't bother if we're just going to renormalize
		
			std::vector<IsotopeEntry> cIsotopes = cAbundancereader.getEntries();
			// copy the abundance file to the output file
			for (auto abdIter = cIsotopes.cbegin(); abdIter != cIsotopes.cend(); abdIter++)
			{
				const IsotopeEntry* pOutputEntry = cOutput.find(abdIter->isotopeName);
				if (pOutputEntry == nullptr) // not already included in the output abundance list
				{
					if (bDump)
					{
						cOutput.addAbundanceToEntry(pHeaviestIsotope->isotopeName,abdIter->abundance);
						std::cout << "Add abundance from " << abdIter->isotopeName << " to " << pHeaviestIsotope->isotopeName << std::endl;
					}
					else// if (bBestGuess)
					{
						std::vector<IsotopeEntry> cIsotopes = cOutput.getEntries(abdIter->atomicNumber);
						if (cIsotopes.size() == 1) // only one isotope of this element; add the abundance of this isotope to the same element
						{
							cOutput.addAbundanceToEntry(cIsotopes[0].isotopeName,abdIter->abundance);
						std::cout << "Add abundance from " << abdIter->isotopeName << " to " << cIsotopes[0].isotopeName << std::endl;
						}
						else if (cIsotopes.size() == 0)
						{ // no isotopes of this element; look for nearby elements
							bool bDone = false;
							int iOffset = 1;
							while (!bDone) {
								std::vector<IsotopeEntry> cIsotopesHigh = cOutput.getEntries(abdIter->atomicNumber + iOffset);
								if (abdIter->atomicNumber > iOffset)
									cIsotopes = cOutput.getEntries(abdIter->atomicNumber - iOffset);
								for (auto iterIso = cIsotopesHigh.begin(); iterIso < cIsotopesHigh.end(); iterIso++)
								{
									cIsotopes.push_back(*iterIso);
								}
								if (cIsotopes.size() > 0)
								{
									for (auto iterIso = cIsotopes.begin(); iterIso < cIsotopes.end(); iterIso++)
									{
										cOutput.addAbundanceToEntry(iterIso->isotopeName,abdIter->abundance / cIsotopes.size());
										std::cout << "Add a portion of abundance from " << abdIter->isotopeName << " to " << iterIso->isotopeName << std::endl;
									}
									bDone = true;
								}
								iOffset++;
							}
							
						}
						else
						{ // spread abundance equally between nearest isotopes
							for (auto iterIso = cIsotopes.begin(); iterIso < cIsotopes.end(); iterIso++)
							{
								cOutput.addAbundanceToEntry(iterIso->isotopeName,abdIter->abundance / cIsotopes.size());
							}
						}
						
					}
				}
			}
		}
	}
	else
	{
		std::vector<IsotopeEntry> cIsotopes = cAbundancereader.getEntries();
		// copy the abundance file to the output file
		for (auto abdIter = cIsotopes.cbegin(); abdIter != cIsotopes.cend(); abdIter++)
		{
			cOutput.setEntry(*abdIter);
		}
	}
	
	double dXcurr = cOutput.totalXAbundance();
	double dYcurr = cOutput.totalYAbundance();
	double dZcurr = cOutput.totalZAbundance();
	
	// adjust abundances as needed
	if ((dX != dXcurr && dX != -1) || (dY != dYcurr && dY != -1) || (dZ != dZcurr && dZ != -1))
	{
		if (dX == -1)
		{
			if (dY == -1)
			{
				dX = (-(dZ - dZcurr) / (dXcurr + dYcurr) + 1.0) * dXcurr;
				dY = (-(dZ - dZcurr) / (dXcurr + dYcurr) + 1.0) * dYcurr;
			}
			else
			{
				if (dZ == -1)
				{
					dX = (-(dY - dYcurr) / (dXcurr + dZcurr) + 1.0) * dXcurr;
					dZ = (-(dY - dYcurr) / (dXcurr + dZcurr) + 1.0) * dZcurr;
				}
				else
				{
					dX = 1.0 - dY - dZ;
				}
			}
		}
		else
		{
			if (dY == -1)
			{
				if (dZ == -1)
				{
					dY = (-(dX - dXcurr) / (dYcurr + dZcurr) + 1.0) * dYcurr;
					dZ = (-(dX - dXcurr) / (dYcurr + dZcurr) + 1.0) * dZcurr;
				}
				else
				{
					dY = 1.0 - dX - dZ;
				}
			}
			else // X and Y set; make Z the difference
			{
				dZ = 1.0 - dX - dY;
			}
		}
		
		double dXscale = dX / dXcurr;
		double dYscale = dY / dYcurr;
		double dZscale = dZ / dZcurr;
		std::vector<IsotopeEntry> cIsotopes = cOutput.getEntries();
		
		for (auto iterIso = cIsotopes.begin(); iterIso < cIsotopes.end(); iterIso++)
		{
			IsotopeEntry cRevisedEntry = *iterIso;
			if (iterIso->atomicNumber == 1)
				cRevisedEntry.abundance *= dXscale;
			else if (iterIso->atomicNumber == 2)
				cRevisedEntry.abundance *= dYscale;
			else
				cRevisedEntry.abundance *= dZscale;
			cOutput.setEntry(cRevisedEntry);
		}
	}

	if (!sNetfile.empty())
		std::cout << "netfile = " << sNetfile << std::endl;
	std::cout << "x = " << std::to_string(dX) << std::endl;
	std::cout << "y = " << std::to_string(dY) << std::endl;
	std::cout << "z = " << std::to_string(dZ) << std::endl;
	if (bDump && pHeaviestIsotope != nullptr)
		std::cout << "transferred all missing isotopes to " << pHeaviestIsotope->isotopeName << std::endl;
	else if (bBestGuess)
		std::cout << "Used best guess to fill in missing isotopes in net" << std::endl;
	else
		std::cout << "normalized missing metals" << std::endl;
	// output results
	cOutput.writeFile(sOutFile);

	return 0;
}
