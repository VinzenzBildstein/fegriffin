#ifndef CAENDIGITIZER_HH
#define CAENDIGITIZER_HH
#include <vector>
#include <string>
#include <functional>
#include <fstream>
#include <time.h>

#include "midas.h"

#include "CaenSettings.hh"

class CaenDigitizer {
public:
	CaenDigitizer(HNDLE hDB, bool debug = false);
	~CaenDigitizer();

	void StartAcquisition(HNDLE hDB);
	void StopAcquisition();
	INT  DataReady();
	bool ReadData(char* event, const int& maxSize, uint32_t& eventsRead);
	void PrintAggregatesPerBlt();
	void PrintEventsPerAggregate();
	void Status();

private:
	void Setup();
	void ProgramPsdDigitizer(int board);
	void ProgramPhaDigitizer(int board);
	void Calibrate();
	void CalibrationStatus();
	bool CalibrationDone();
	bool BoardsReady();
	uint32_t GetNumberOfEvents(char* fBuffer, uint32_t fBufferSize);

	CaenSettings* fSettings;

	std::vector<int> fHandle;
	std::vector<int> fPort;
	std::vector<int> fDevice;
	std::vector<int> fFirmwareVersion;
	enum class EFirmware : char { kPSD, kPHA };
	std::vector<EFirmware> fFirmwareType;
	// raw readout data
	char*            fBuffer; 
	uint32_t         fBufferSize;
	uint32_t         fMaxBufferSize;

	std::ofstream fRawOutput;

	// variables for status output
	std::vector<uint32_t> fReadError;
	std::vector<uint32_t> fNofEvents;
	std::vector<uint32_t> fLastNofEvents;
	uint32_t fLastTotalNofEvents;
	int fIteration;
	struct timespec fLastUpdate;

	bool fDebug;
	bool fSetupDone;
};
#endif
