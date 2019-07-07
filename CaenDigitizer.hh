#ifndef CAENDIGITIZER_HH
#define CAENDIGITIZER_HH
#include <vector>
#include <string>
#include <functional>
#include <fstream>

#include "midas.h"

#include "CaenSettings.hh"

class CaenDigitizer {
public:
	CaenDigitizer(HNDLE hDB, bool debug = false);
	~CaenDigitizer();

	void StartAcquisition(HNDLE hDB);
	void StopAcquisition();
	INT  DataReady();
	bool ReadData(char* event, const char* bankName, const int& maxSize, uint32_t& eventsRead);
	void PrintAggregatesPerBlt();
	void PrintEventsPerAggregate();

private:
	void Setup();
	void ProgramDigitizer(int board);
	void Calibrate();
	void CalibrationStatus();
	bool CalibrationDone();
	bool BoardsReady();
	uint32_t GetNumberOfEvents(char* fBuffer, uint32_t fBufferSize);

	CaenSettings* fSettings;

	std::vector<int> fHandle;
	std::vector<int> fPort;
	std::vector<int> fDevice;
	// raw readout data
	char*            fBuffer; 
	uint32_t         fBufferSize;
	uint32_t         fMaxBufferSize;

	std::ofstream fRawOutput;

	bool fDebug;
	bool fSetupDone;
};
#endif
