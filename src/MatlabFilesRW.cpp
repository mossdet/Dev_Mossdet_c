// MatlabFilesRW.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <stdio.h>
#include <string.h> /* For strcmp() */
#include <stdlib.h> /* For EXIT_FAILURE, EXIT_SUCCESS */
#include <vector> /* For STL */
#include <direct.h>

#define BUFSIZE 256

#include "mat.h"
#include "MatlabFilesRW.h"


MatlabFilesRW::MatlabFilesRW(std::string filename, double samplingRate, std::string outputPath, bool verbose) {
	m_matFilename = filename;
	m_samplingRate = samplingRate;
	m_outputPath = outputPath;
	m_verbose = verbose;
}

long MatlabFilesRW::getSampleCount() {
	long sampleCount;

	const char *file = m_matFilename.c_str();
	MATFile *pmat;
	const char **dir;
	const char *name;
	int	  ndir;
	mxArray *pa;

	//Open file to get directory
	pmat = matOpen(file, "r");
	if (pmat == NULL) {
		if (m_verbose)
			printf("Error opening file %s\n", file);
		return(1);
	}

	//get directory of MAT-file
	dir = (const char **)matGetDir(pmat, &ndir);
	if (dir == NULL) {
		if (m_verbose)
			printf("Error reading directory of file %s\n", file);
		return(0);
	}
	mxFree(dir);
	/* In order to use matGetNextXXX correctly, reopen file to read in headers. */
	if (matClose(pmat) != 0) {
		return(0);
	}
	pmat = matOpen(file, "r");
	if (pmat == NULL) {
		return(0);
	}

	//for (int i = 0; i < ndir; i++) {
		pa = matGetNextVariableInfo(pmat, &name);
		if (pa == NULL) {
			//printf("Error reading in file %s\n", file);
			return(1);
		}
		sampleCount = mxGetNumberOfElements(pa);
		if(m_verbose)
			printf("Sample Count: %d\n", sampleCount);
		mxDestroyArray(pa);
	//}
	
	return sampleCount;
}


int MatlabFilesRW::writeMatlabSamples(std::vector<EOI_Event> &allDetections) {

	MATFile *pmat;
	mxArray *pa1;
	long nrDetections = allDetections.size();

	std::vector<double> detectionsDatToWrite;;
	for (long detIdx = 0; detIdx < allDetections.size(); detIdx++) {
		int detType = 0;
		
		std::string label = allDetections[detIdx].description;
		bool foundFastRipple = label.find("ast") != std::string::npos;
		bool foundRipple = ((label.find("ipple") != std::string::npos) || (label.find("HFO") != std::string::npos)) && !foundFastRipple;
		bool foundSpike = label.find("ike") != std::string::npos;
		/*bool foundFastRipple = label.find("ast") != std::string::npos;
		bool foundRipple = ((label.find("ipple") != std::string::npos) || (label.find("HFO") != std::string::npos)) && !foundFastRipple;
		bool foundRippleAndFastRipple = false;
		bool foundSpike = label.find("ike") != std::string::npos;
		bool foundSleepSpindle = label.find("indle") != std::string::npos;
		if (label.find("ipple") != std::string::npos) {
			label.erase(label.find("ipple"), 5);
			foundRippleAndFastRipple = label.find("ipple") != std::string::npos;
			if (foundRippleAndFastRipple) {
				foundRipple = false;
				foundFastRipple = false;
			}
		}*/

		if (foundRipple)//Ripple
			detType = 1;
		if (foundFastRipple)//FR
			detType = 2;
		if (foundSpike)//R+FR
			detType = 3;

		/*if (foundRipple && (!foundFastRipple && !foundRippleAndFastRipple && !foundSpike))//Ripple
			detType = 1;
		if (foundFastRipple && (!foundRipple && !foundRippleAndFastRipple && !foundSpike))//FR
			detType = 2;
		if (foundRippleAndFastRipple && (!foundRipple && !foundFastRipple && !foundSpike))//R+FR
			detType = 3;

		if ((foundSpike && foundRipple) && (!foundFastRipple && !foundRippleAndFastRipple))//IES+Ripple
			detType = 4;
		if ((foundSpike && foundFastRipple) && (!foundRipple && !foundRippleAndFastRipple))//IES+FR
			detType = 5;
		if ((foundSpike && foundRippleAndFastRipple) && (!foundRipple && !foundFastRipple))//IES+Ripple+FR
			detType = 6;

		if (foundSpike && (!foundRipple && !foundFastRipple && !foundRippleAndFastRipple))//IES
			detType = 7;*/

		detectionsDatToWrite.push_back(detType);
		detectionsDatToWrite.push_back(allDetections[detIdx].startTime);
		detectionsDatToWrite.push_back(allDetections[detIdx].endTime);
	}

	std::string outputDirectory = m_outputPath + "\\MOSSDET_Output\\";
	_mkdir(outputDirectory.c_str());
	std::string writeFilename = outputDirectory + "MOSSDET_Detections.mat";
	int status;
	
	if (m_verbose)
		printf("Creating file %s...\n\n", writeFilename.c_str());
	
	pmat = matOpen(writeFilename.c_str(), "w");
	if (pmat == NULL) {
		if (m_verbose) {
			printf("Error creating file %s\n", writeFilename.c_str());
			printf("(Do you have write permission in this directory?)\n");
		}
		return(EXIT_FAILURE);
	}

	pa1 = mxCreateDoubleMatrix(3, nrDetections, mxREAL);
	if (pa1 == NULL) {
		if (m_verbose) {
			printf("%s : Out of memory on line %d\n", __FILE__, __LINE__);
			printf("Unable to create mxArray.\n");
		}
		return(EXIT_FAILURE);
	}

	//status = matPutVariable(pmat, "LocalDouble", pa1);//local variable
	status = matPutVariableAsGlobal(pmat, "MOSSDET_Detections", pa1);
	if (status != 0) {
		if (m_verbose)
			printf("%s :  Error using matPutVariable on line %d\n", __FILE__, __LINE__);
		return(EXIT_FAILURE);
	}
	int sodd = sizeof(detectionsDatToWrite.data());
	memcpy((void *)(mxGetPr(pa1)), (void *)detectionsDatToWrite.data(), detectionsDatToWrite.size()*sizeof(double));
	status = matPutVariableAsGlobal(pmat, "MOSSDET_Detections", pa1);
	if (status != 0) {
		if (m_verbose)
			printf("%s :  Error using matPutVariable on line %d\n", __FILE__, __LINE__);
		return(EXIT_FAILURE);
	}

	/* clean up */
	mxDestroyArray(pa1);
	if (matClose(pmat) != 0) {
		if (m_verbose)
			printf("Error closing file %s\n", writeFilename.c_str());
		return(EXIT_FAILURE);
	}

	/*
	* Re-open file and verify its contents with matGetVariable
	*/
	pmat = matOpen(writeFilename.c_str(), "r");
	if (pmat == NULL) {
		if (m_verbose)
			printf("Error reopening file %s\n", writeFilename.c_str());
		return(EXIT_FAILURE);
	}

	/*
	* Read in each array we just wrote
	*/
	pa1 = matGetVariable(pmat, "MOSSDET_Detections");
	if (pa1 == NULL) {
		if (m_verbose)
			printf("Error reading existing matrix MOSSDET_Detections\n");
		return(EXIT_FAILURE);
	}
	if (mxGetNumberOfDimensions(pa1) != 2) {
		if (m_verbose)
			printf("Error saving matrix: result does not have two dimensions\n");
		return(EXIT_FAILURE);
	}
	if (!(mxIsFromGlobalWS(pa1))) {
		if (m_verbose)
			printf("Error saving global matrix: result is not global\n");
		return(EXIT_FAILURE);
	}

	/* clean up before exit */
	mxDestroyArray(pa1);

	if (matClose(pmat) != 0) {
		if (m_verbose)
			printf("Error closing file %s\n", writeFilename.c_str());
		return(EXIT_FAILURE);
	}
	printf("Done\n");
	return(EXIT_SUCCESS);
}

int MatlabFilesRW::readMatlabSamples(long startSample, long nrSamplesToRead, std::vector<std::vector<double>> &m_reformattedSignalSamples) {

	m_reformattedSignalSamples.resize(1);

	const char *file = m_matFilename.c_str();
	MATFile *pmat;
	const char **dir;
	const char *name;
	int	  ndir;
	mxArray *pa;
	if (m_verbose)
		printf("Reading file %s...\n", file);

	//Open file to get directory
	pmat = matOpen(file, "r");
	if (pmat == NULL) {
		if (m_verbose)
			printf("Error opening file %s\n", file);
		return(1);
	}

	//get directory of MAT-file
	dir = (const char **)matGetDir(pmat, &ndir);
	if (dir == NULL) {
		if (m_verbose)
			printf("Error reading directory of file %s\n", file);
		return(1);
	}
	else {
		if (m_verbose)
			printf("Directory of %s:\n", file);
		for (int i = 0; i < ndir; i++)
			printf("%s\n", dir[i]);
	}
	mxFree(dir);

	/* In order to use matGetNextXXX correctly, reopen file to read in headers. */
	if (matClose(pmat) != 0) {
		if (m_verbose)
			printf("Error closing file %s\n", file);
		return(1);
	}
	pmat = matOpen(file, "r");
	if (pmat == NULL) {
		if (m_verbose)
			printf("Error reopening file %s\n", file);
		return(1);
	}

	/* Get headers of all variables */
	if (m_verbose)
		printf("\nExamining the header for each variable:\n");
	for (int i = 0; i < ndir; i++) {
		pa = matGetNextVariableInfo(pmat, &name);

		if (pa == NULL) {
			if (m_verbose)
				printf("Error reading in file %s\n", file);
			return(1);
		}
		/* Diagnose header pa */
		if (m_verbose) {
			printf("According to its header, array %s has %d dimensions\n",
			name, mxGetNumberOfDimensions(pa));
			if (mxIsFromGlobalWS(pa))
				printf("  and was a global variable when saved\n");
			else
				printf("  and was a local variable when saved\n");
		}

		//std::cout << *mxGetPr(pa) << std::endl;
		//std::cout << mxGetCell(pa, 1) << std::endl;


		mxDestroyArray(pa);
	}

	/* Reopen file to read in actual arrays. */
	if (matClose(pmat) != 0) {
		if (m_verbose)
			printf("Error closing file %s\n", file);
		return(1);
	}
	pmat = matOpen(file, "r");
	if (pmat == NULL) {
		if (m_verbose)
			printf("Error reopening file %s\n", file);
		return(1);
	}

	/* Read in each array. */
	if (m_verbose)
		printf("\nReading in the actual array contents:\n");
	for (int i = 0; i<ndir; i++) {
		pa = matGetNextVariable(pmat, &name);
		if (pa == NULL) {
			if (m_verbose)
				printf("Error reading in file %s\n", file);
			return(1);
		}
		/*
		* Diagnose array pa
		*/
		if (m_verbose) {
			printf("According to its contents, array %s has %d dimensions\n",
				name, mxGetNumberOfDimensions(pa));
			if (mxIsFromGlobalWS(pa))
				printf("  and was a global variable when saved\n");
			else
				printf("  and was a local variable when saved\n");
		}

		double *real_data_ptr;
		real_data_ptr = (double *)mxGetPr(pa);
		for (int idx = 0; idx < mxGetNumberOfElements(pa); ++idx) {
			if (mxIsChar(pa)) {
				std::string readStr;
				readStr = mxArrayToString(pa);
				std::cout << readStr.c_str() << std::endl;
				break;
			}
			else {
				//std::cout << *real_data_ptr << std::endl;

				if ((idx >= startSample) && (idx < (startSample + nrSamplesToRead))) {
					m_reformattedSignalSamples[0].push_back(*real_data_ptr);
				}
				*real_data_ptr++;
			}
		}
		std::cout << std::endl;
		mxDestroyArray(pa);
	}
	if (matClose(pmat) != 0) {
		if (m_verbose)
			printf("Error closing file %s\n", file);
		return(1);
	}
	printf("Done\n");
	return 1;
}
