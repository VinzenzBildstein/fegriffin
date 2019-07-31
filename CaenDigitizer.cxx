#include "CaenDigitizer.hh"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <string>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <time.h>

std::string format(const std::string& format, ...)
{
	va_list args;
	va_start (args, format);
	size_t len = std::vsnprintf(NULL, 0, format.c_str(), args);
	va_end (args);
	std::vector<char> vec(len + 1);
	va_start (args, format);
	std::vsnprintf(&vec[0], len + 1, format.c_str(), args);
	va_end (args);
	return &vec[0];
}

CaenDigitizer::CaenDigitizer(HNDLE hDB, bool debug)
	: fSettings(new CaenSettings(debug)), fDebug(debug), fSetupDone(false)
{
	if(fDebug) std::cout<<"Using ODB handle "<<hDB<<std::endl;
	// only setup digitizers now if we were successful in reading the settings from the ODB
	if(fSettings->ReadOdb(hDB)) Setup();
}

void CaenDigitizer::Setup()
{
	if(fDebug) std::cout<<"setting up digitizer"<<std::endl;
	CAEN_DGTZ_ErrorCode errorCode;
	CAEN_DGTZ_BoardInfo_t boardInfo;

	if(static_cast<int>(fHandle.size()) < fSettings->NumberOfBoards()) {
		// we have more boards now than before, so we need to initialize the additional boards
		try {
			fHandle.resize(fSettings->NumberOfBoards(), -1);
			fPort.resize(fSettings->NumberOfBoards(), -1);
			fDevice.resize(fSettings->NumberOfBoards(), -1);
			fFirmwareVersion.resize(fSettings->NumberOfBoards(), -1);
			fFirmwareType.resize(fSettings->NumberOfBoards(), EFirmware::kPSD);
			fBuffer.resize(fSettings->NumberOfBoards(), NULL);
			fBufferSize.resize(fSettings->NumberOfBoards(), 0);
			fWaveforms.resize(fSettings->NumberOfBoards(), NULL);
			fNofEvents.resize(fSettings->NumberOfBoards(), 0);
			fLastNofEvents.resize(fSettings->NumberOfBoards(), 0);
			fReadError.resize(fSettings->NumberOfBoards(), 0);
		} catch(std::exception e) {
			std::cerr<<"Failed to resize vectors for "<<fSettings->NumberOfBoards()<<" boards, and "<<fSettings->NumberOfChannels()<<" channels: "<<e.what()<<std::endl;
			throw e;
		}
	}
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		if(fHandle[b] == -1 || fPort[b] != fSettings->PortNumber(b) || fDevice[b] != fSettings->DeviceNumber(b)) {
			if(fDebug) std::cout<<"setting up board "<<b<<std::endl;
			// open digitizers
			errorCode = CAEN_DGTZ_OpenDigitizer(fSettings->LinkType(b), fSettings->PortNumber(b), fSettings->DeviceNumber(b), fSettings->VmeBaseAddress(b), &fHandle[b]);
			fPort[b] = fSettings->PortNumber(b);
			fDevice[b] = fSettings->DeviceNumber(b);
			if(errorCode != 0) {
				throw std::runtime_error(format("Error %d when opening %d. digitizer using port %d, device %d", errorCode, b, fPort[b], fDevice[b]));
			}
			if(fDebug) std::cout<<"got handle "<<fHandle[b]<<" for board "<<b<<std::endl;
			// get digitizer info
			errorCode = CAEN_DGTZ_GetInfo(fHandle[b], &boardInfo);
			if(errorCode != 0) {
				CAEN_DGTZ_CloseDigitizer(fHandle[b]);
				throw std::runtime_error(format("Error %d when reading digitizer info", errorCode));
			}
			std::cout<<std::endl<<"Connected to CAEN Digitizer Model "<<boardInfo.ModelName<<" serial number "<<boardInfo.SerialNumber<<" as "<<b<<". board, using port "<<fPort[b]<<", device "<<fDevice[b]<<std::endl;
			std::cout<<std::endl<<"Firmware is ROC "<<boardInfo.ROC_FirmwareRel<<", AMC "<<boardInfo.AMC_FirmwareRel<<std::endl;

			std::stringstream str(boardInfo.AMC_FirmwareRel);
			str>>fFirmwareVersion[b];
			if(fFirmwareVersion[b] == 131 || fFirmwareVersion[b] == 132 || fFirmwareVersion[b] == 136) {
				std::cout<<"Digitizer "<<b<<" has DPP-PSD firmware"<<std::endl;
				fFirmwareType[b] = EFirmware::kPSD;
			} else {
				std::cout<<"Digitizer "<<b<<" has DPP-PHA firmware"<<std::endl;
				fFirmwareType[b] = EFirmware::kPHA;
			}
		} else {// if(fHandle[b] == -1)
			std::cout<<"Re-using handle for port "<<fPort[b]<<"/"<<fSettings->PortNumber(b)<<", device "<<fDevice[b]<<"/"<<fSettings->DeviceNumber(b)<<std::endl;
		}
	}

	// we always re-program the digitizer in case settings have been changed
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		if(fFirmwareType[b] == EFirmware::kPSD) {
			ProgramPsdDigitizer(b);
		} else if(fFirmwareType[b] == EFirmware::kPHA) {
			ProgramPhaDigitizer(b);
		}

		// we don't really need to know how many bytes have been allocated, so we use fBufferSize here
		free(fBuffer[b]);
		//fBuffer[b] = static_cast<char*>(malloc(100*6504464));
		//1638416 bytes are allocated by CAEN_DGTZ_MallocReadoutBuffer (2 channels, 192 samples each)
		//changing this to 8 channels changed the number to 6504464
		if(fDebug) std::cout<<fHandle[b]<<"/"<<fBuffer.size()<<": trying to allocate memory for readout buffer "<<static_cast<void*>(fBuffer[b])<<std::endl;
		errorCode = CAEN_DGTZ_MallocReadoutBuffer(fHandle[b], &fBuffer[b], &fBufferSize[b]);
		if(errorCode != 0) {
			CAEN_DGTZ_CloseDigitizer(fHandle[b]);
			throw std::runtime_error(format("Error %d when allocating readout buffer", errorCode));
		}
		if(fDebug) std::cout<<"allocated "<<fBufferSize[0]<<" bytes of buffer for board "<<b<<std::endl;
#ifdef USE_WAVEFORMS
		// allocate waveforms, again not caring how many bytes have been allocated
		uint32_t size;
		free(fWaveforms[b]);
		errorCode = CAEN_DGTZ_MallocDPPWaveforms(fHandle[b], reinterpret_cast<void**>(&(fWaveforms[b])), &size);
		if(errorCode != 0) {
			CAEN_DGTZ_CloseDigitizer(fHandle[b]);
			throw std::runtime_error(format("Error %d when allocating DPP waveforms", errorCode));
		}
#endif
		if(fDebug) std::cout<<"done with board "<<b<<std::endl;
	} // for(int b = 0; b < fSettings->NumberOfBoards(); ++b)
	fSetupDone = true;
	std::cout<<"Setup of VX1730 finished!"<<std::endl;
}

CaenDigitizer::~CaenDigitizer()
{
	// only need to free or close anything if we did a setup already
	for(int b = 0; b < fSettings->NumberOfBoards() && fSetupDone; ++b) {
		CAEN_DGTZ_FreeReadoutBuffer(&fBuffer[b]);
#ifdef USE_WAVEFORMS
		CAEN_DGTZ_FreeDPPWaveforms(fHandle[b], reinterpret_cast<void*>(fWaveforms[b]));
#endif
		CAEN_DGTZ_CloseDigitizer(fHandle[b]);
	}
}

void CaenDigitizer::StartAcquisition(HNDLE hDB)
{
	// re-load settings from ODB and set digitzer up (again)
	fSettings->ReadOdb(hDB);
	Setup();
	Calibrate();

	// reset status variables
	fIteration = 0;
	for(size_t i = 0; i < fNofEvents.size(); ++i) {
		fNofEvents[i] = 0;
		fLastNofEvents[i] = 0;
		fReadError[i] = 0;
	}
	clock_gettime(CLOCK_REALTIME, &fLastUpdate);
	fLastTotalNofEvents = 0;

	int i;
	for(i = 0; i < 100 && !CalibrationDone(); ++i) {
		CalibrationStatus();
	}
	if(i == 100) {
		std::cout<<"gave up waiting for calibration to be done after "<<100<<" tries"<<std::endl;
	} else {
		std::cout<<"Calibration is done!"<<std::endl;
	}

	if(fSettings->RawOutput()) {
		// open raw output file
		fRawOutput.open("raw.dat");
	}
	// don't need to start acquisition, this is done by the s-in/gpi signal
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		CAEN_DGTZ_SWStartAcquisition(fHandle[b]);
	}
}

void CaenDigitizer::StopAcquisition()
{
	// stop acquisition
	for(int b = 0; b < fSettings->NumberOfBoards() && fSetupDone; ++b) {
		CAEN_DGTZ_SWStopAcquisition(fHandle[b]);
	}
	if(fRawOutput.is_open()) {
		fRawOutput.close();
	}
	uint32_t totalNofEvents = 0;
	std::cout<<std::endl;
	for(size_t i = 0; i < fNofEvents.size(); ++i) {
		std::cout<<"board "<<i<<": "<<std::setw(12)<<fNofEvents[i]<<" events  | ";
		totalNofEvents += fNofEvents[i];
	}
	std::cout<<"total: "<<std::setw(12)<<totalNofEvents<<" events    "<<std::endl;
}

void CaenDigitizer::Calibrate()
{
	// calibrate all digitizers
	int errorCode = 0;
	// start calibration for each board
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		errorCode = CAEN_DGTZ_Calibrate(fHandle[b]);
		if(errorCode != 0) {
			std::cerr<<"Error "<<errorCode<<" when trying to calibrate board "<<b<<", handle "<<fHandle[b]<<std::endl;
		}
	}
}

void CaenDigitizer::CalibrationStatus()
{
	// check calibration status for each board
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		std::cout<<"board "<<b<<": ";
		for(int ch = 0; ch < fSettings->NumberOfChannels(); ++ch) {
			if((fSettings->ChannelMask(b) & (1<<ch)) != 0) {
				uint32_t	address = 0x1088 + 0x100*ch;
				uint32_t data = 0;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				if((data&0x8) != 0x8) {
					std::cout<<"-";
				} else {
					std::cout<<"+";
				}
			}
		}
		std::cout<<std::endl;
	}
}

bool CaenDigitizer::CalibrationDone()
{
	// check if calibration is done for each board
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		for(int ch = 0; ch < fSettings->NumberOfChannels(); ++ch) {
			if((fSettings->ChannelMask(b) & (1<<ch)) != 0) {
				uint32_t	address = 0x1088 + 0x100*ch;
				uint32_t data = 0;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				if((data&0x8) != 0x8) {
					return false;
				}
			}
		}
	}
	return true;
}

INT CaenDigitizer::DataReady()
{
	// if the setup hasn't been done we can't get any data
	if(!fSetupDone) {
		return FALSE;
	}

	int errorCode = 0;

	// read data
	bool gotData = false;
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		errorCode = CAEN_DGTZ_ReadData(fHandle[b], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, fBuffer[b], &fBufferSize[b]);
		if(errorCode != 0) {
			++fReadError[b];
			if(fReadError[b]%100 == 0) {
				cm_msg(MERROR, "DataReady", "Error %d when reading data from board %d (%d. error)", errorCode, b, fReadError[b]);
			}
			// sleep for a microsecond
			nanosleep((const struct timespec[]){{0, 1000}}, (struct timespec*) NULL);
			return FALSE;
		}
		if(fBufferSize[b] > 0) {
			if(fDebug) {
				std::cout<<"Read "<<fBufferSize[b]<<" bytes from board "<<b<<std::endl;
			}
			gotData = true;
		}
	}

	// this is necessary because TRUE and FALSE are not booleans ...
	if(gotData) return TRUE;
	return FALSE;
}

bool CaenDigitizer::ReadData(char* event, const int& maxSize, uint32_t& eventsRead)
{
	// creates bank at <event> and copies up to maxSize worth of data from fBuffer to it
	// sets the number of events read from the buffer via eventsRead and returns if there are any
	// events left in the buffer.
	//
	// no checks for valid events done, nor any identification of board/channel???
	DWORD* data;
	//check if we have any data
	int totalSize = 0;
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		if(fBufferSize[b] >= 0) {
			totalSize += fBufferSize[b];
		} else {
			std::cerr<<"buffer size of board "<<b<<" is negative: "<<fBufferSize[b]<<std::endl;
			fBufferSize[b] = 0;
		}
	}
	if(totalSize == 0) {
		std::cout<<__PRETTY_FUNCTION__<<": no data"<<std::endl;
		return false; // had no data, so there is none left either
	}
	const char* bankName;
	//check if we can copy all events from fBuffer to data
	int sizeRead = 0;
	eventsRead = 0;
	int b;
	for(b = 0; b < fSettings->NumberOfBoards(); ++b) {
		if(fBufferSize[b] == 0) continue;
		if(fBufferSize[b] > static_cast<size_t>(maxSize)) {
			std::cout<<"Can't fit board buffer of "<<fBufferSize[b]<<" bytes into maximum size of "<<maxSize<<" bytes, discarding this buffer!"<<std::endl;
			fBufferSize[b] = 0; // prevent us from trying/warning about this again
			continue;
		}
		if(fBufferSize[b] + sizeRead > static_cast<size_t>(maxSize)) {
			break;
		}
		//create bank - returns pointer to data area of bank
		if(fFirmwareType[b] == EFirmware::kPSD) {
			bankName = "CAEN";
		} else if(fFirmwareType[b] == EFirmware::kPHA) {
			bankName = "CPHA";
		}
		bk_create(event, bankName, TID_DWORD, reinterpret_cast<void**>(&data));
		// at this stage we know that this boards buffer will fit in the rest of the event buffer
		//copy buffer of this board
		std::memcpy(data, fBuffer[b], fBufferSize[b]);
		data += fBufferSize[b]/sizeof(DWORD);
		if(fRawOutput.is_open()) {
			fRawOutput.write(fBuffer[b], fBufferSize[b]);
		}

		uint32_t nofEvents = GetNumberOfEvents(fBuffer[b], fBufferSize[b]);
		fNofEvents[b] += nofEvents;
		eventsRead += nofEvents;

		// we're done reading this boards buffer, so we set it's size to zero
		// to prevent copying it again if this function fails to write all board
		// buffers into a single midas event
		fBufferSize[b] = 0;
		
		//close bank
		bk_close(event, data);
	}

	// we only have events left in the buffers if the above loop
	// was broken out of

	return (b != fSettings->NumberOfBoards());
}

uint32_t CaenDigitizer::GetNumberOfEvents(char* buffer, uint32_t bufferSize)
{
	uint32_t numEvents = 0;
	uint32_t w = 0;
	uint32_t* data = reinterpret_cast<uint32_t*>(buffer);
	while(w < bufferSize) {
		//read board aggregate header
		if(data[w]>>28 != 0xa) {
			// wrong format, let's just return zero for now
			if(fDebug) { 
				std::cout<<w<<": wrong aggregate header word 0x"<<std::hex<<data[w]<<std::dec<<std::endl;
				for(w=0;w<bufferSize;++w) {
					std::cout<<"0x"<<std::hex<<std::setfill('0')<<std::setw(8)<<data[w]<<std::dec<<" ";
					if(w%8 == 7) std::cout<<std::endl;
				}
			}
			return numEvents;
		}
		int32_t numWordsBoard = data[w]&0xfffffff;
		if(w + numWordsBoard > bufferSize) {
			// we're missing words, return zero for now
			if(fDebug) std::cout<<w<<": missing words, should get "<<numWordsBoard<<" from size "<<bufferSize<<std::endl;
			return numEvents;
		}
		if(fDebug) std::cout<<w<<": got "<<numWordsBoard<<" words in board"<<std::endl;

		//read the channel mask
		++w;
		uint8_t channelMask = data[w]&0xff;

		if(fDebug) std::cout<<w<<": got channel mask 0x"<<std::hex<<static_cast<int>(channelMask)<<std::dec<<std::endl;
		//skip the next two words
		w += 2;
		// loop over all active channels and read their channel aggregate header
		for(uint8_t channel = 0; channel < 16 && w < bufferSize; channel += 2) {
			if(((channelMask>>(channel/2)) & 0x1) == 0x0) continue;
			++w;
			if(w >= bufferSize) { std::cout<<channel<<": "<<w<<" >= "<<bufferSize<<std::endl; return numEvents; } 
			if(data[w]>>31 != 0x1) {
				//failed to find the right channel header
				if(fDebug) std::cout<<w<<": wrong channel header word 0x"<<std::hex<<data[w]<<std::dec<<std::endl;
				return numEvents;
			}
			int32_t numWordsChannel = data[w]&0x3fffff;
			if(fDebug) std::cout<<static_cast<int>(channel)<<", "<<w<<": got numWordsChannel "<<numWordsChannel<<std::endl;

			//read the format and number of waveform samples
			++w;
			if(w >= bufferSize) { std::cout<<channel<<": "<<w<<" >= "<<bufferSize<<std::endl; return numEvents; } 
			if(((data[w]>>29) & 0x3) != 0x3) {
				//bits 29&30 should be set
				if(fDebug) std::cout<<w<<": wrong format word 0x"<<std::hex<<data[w]<<std::dec<<std::endl;
				return numEvents;
			}
			//this is nofSample/8, 2 samples/word => *4
			//+2 for trigger time and charge words
			int eventSize = 4*(data[w]&0xffff) + 2;
			//extra word enabled?
			if(((data[w]>>28) & 0x1) == 0x1) ++eventSize;
			//we should have (numWordsChannel-2)/eventSize hits from this board
			//-2 for the 2 header words of the channel aggregate
			if(fDebug) std::cout<<static_cast<int>(channel)<<", "<<w<<": got "<<numWordsChannel-2<<"/"<<eventSize<<"; "<<numEvents<<", "<<w<<" => ";
			numEvents += (numWordsChannel-2)/eventSize;
			//skip these words to get to the next channel aggregate header (minus the channel header we've already read)
			w += numWordsChannel-1;
			if(fDebug) std::cout<<numEvents<<", "<<w<<std::endl;
		}
	}
	if(fDebug) std::cout<<w<<": got "<<numEvents<<" events from buffer of size "<<bufferSize<<std::endl;
	return numEvents;
}

void CaenDigitizer::ProgramPsdDigitizer(int b)
{
	uint32_t address;
	uint32_t data;

	if(fDebug) std::cout<<"programming digitizer "<<b<<std::endl;
	CAEN_DGTZ_ErrorCode errorCode;

   // not sure if this is needed?
	errorCode = CAEN_DGTZ_Reset(fHandle[b]);

	if(errorCode != 0) {
	//	throw std::runtime_error(format("Error %d when resetting digitizer", errorCode));
		cm_msg(MINFO, "ProgramDigitizer", "Error %d when resetting digitizer %d", errorCode, b);
		errorCode = CAEN_DGTZ_ErrorCode(0);
	}

	errorCode = CAEN_DGTZ_SetDPPAcquisitionMode(fHandle[b], fSettings->AcquisitionMode(b), CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
	//CAEN_DGTZ_DPP_AcqMode_t mode;
	//CAEN_DGTZ_DPP_SaveParam_t param;
	//std::cout<<"acquisition mode "<<CAEN_DGTZ_GetDPPAcquisitionMode(fHandle[b], &mode, &param)<<": mode "<<mode<<", param "<<param<<std::endl;

	if(errorCode != 0) {
		//throw std::runtime_error(format("Error %d when setting DPP acquisition mode", errorCode));
		cm_msg(MINFO, "ProgramDigitizer", "Error %d when setting DPP acquisition mode for digitizer %d", errorCode, b);
		errorCode = CAEN_DGTZ_ErrorCode(0);
	}

	// CAEN_DGTZ_SetAcquisitionMode gets overwritten later by CAEN_DGTZ_SetRunSynchronizationMode, so we don't bother with it

	errorCode = CAEN_DGTZ_SetIOLevel(fHandle[b], fSettings->IOLevel(b));

	if(errorCode != 0) {
		//throw std::runtime_error(format("Error %d when setting IO level", errorCode));
		cm_msg(MINFO, "ProgramDigitizer", "Error %d when setting IO level for digitizer %d", errorCode, b);
		errorCode = CAEN_DGTZ_ErrorCode(0);
	}

	errorCode = CAEN_DGTZ_SetExtTriggerInputMode(fHandle[b], fSettings->TriggerMode(b));

	if(errorCode != 0) {
		//throw std::runtime_error(format("Error %d when setting external trigger DPP events", errorCode));
		cm_msg(MINFO, "ProgramDigitizer", "Error %d when setting external trigger DPP events for digitizer %d", errorCode, b);
		errorCode = CAEN_DGTZ_ErrorCode(0);
	}

	errorCode = CAEN_DGTZ_SetChannelEnableMask(fHandle[b], fSettings->ChannelMask(b));

	if(errorCode != 0) {
		//throw std::runtime_error(format("Error %d when setting channel mask", errorCode));
		cm_msg(MINFO, "ProgramDigitizer", "Error %d when setting channel mask for digitizer %d", errorCode, b);
		errorCode = CAEN_DGTZ_ErrorCode(0);
	}

	// disabled turns acquisition mode back to SW controlled
	// both GpioGpioDaisyChain and SinFanout turn it to S_IN controlled
	// according to rev18 manual GpioGpioDaisyChain is not used!
	errorCode = CAEN_DGTZ_SetRunSynchronizationMode(fHandle[b], fSettings->RunSync(b)); // change to settings (was CAEN_DGTZ_RUN_SYNC_Disabled)

	if(errorCode != 0) {
		//throw std::runtime_error(format("Error %d when setting run synchronization", errorCode));
		cm_msg(MINFO, "ProgramDigitizer", "Error %d when setting run synchronization for digitizer %d", errorCode, b);
		errorCode = CAEN_DGTZ_ErrorCode(0);
	}

	errorCode = CAEN_DGTZ_SetDPPParameters(fHandle[b], fSettings->ChannelMask(b), const_cast<void*>(static_cast<const void*>(fSettings->ChannelParameter(b))));

	if(errorCode != 0) {
		//throw std::runtime_error(format("Error %d when setting dpp parameters", errorCode));
		cm_msg(MINFO, "ProgramDigitizer", "Error %d when setting dpp parameters for digitizer %d", errorCode, b);
		errorCode = CAEN_DGTZ_ErrorCode(0);
	}

	// write some special registers directly
	// enable EXTRA word
	address = 0x8000;
	CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
	data |= 0x20000; // no mask necessary, we just set one bit
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	for(int ch = 0; ch < fSettings->NumberOfChannels(); ++ch) {
		if((fSettings->ChannelMask(b) & (1<<ch)) != 0) {
			if(fDebug) std::cout<<"programming channel "<<ch<<std::endl;
			if(ch%2 == 0) {
				errorCode = CAEN_DGTZ_SetRecordLength(fHandle[b], fSettings->RecordLength(b, ch), ch);
			}

			errorCode = CAEN_DGTZ_SetChannelDCOffset(fHandle[b], ch, fSettings->DCOffset(b, ch));

			errorCode = CAEN_DGTZ_SetDPPPreTriggerSize(fHandle[b], ch, fSettings->PreTrigger(b, ch));

			errorCode = CAEN_DGTZ_SetChannelPulsePolarity(fHandle[b], ch, fSettings->PulsePolarity(b, ch));

			if(fSettings->EnableCfd(b, ch)) {
				if(fDebug) std::cout<<"enabling CFD on channel "<<ch<<std::endl;
				// enable CFD mode
				address = 0x1080 + ch*0x100;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				//std::cout<<"cfd - 0x"<<std::hex<<address<<" read 0x"<<data<<std::dec<<std::endl;
				data |= 0x40; // no mask necessary, we just set one bit
				CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
				//std::cout<<"cfd - 0x"<<std::hex<<address<<" wrote 0x"<<data<<std::dec<<std::endl;

				// set CFD parameters
				address = 0x103c + ch*0x100;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				data = (data & ~0xfff) | (fSettings->CfdParameters(b, ch) & 0xfff);
				CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
			}
			// write extended TS, flags, and fine TS (from CFD) to extra word
			address = 0x1084 + ch*0x100;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data = (data & ~0x700) | 0x200;
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			// if input range flag is set we use 0.5 Vpp instead of 2 Vpp range
			if(fSettings->InputRange(b, ch)) {
				address = 0x1028 + ch*0x100;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				data = (data & ~0x1) | 0x1;
				CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
			} else {
				address = 0x1028 + ch*0x100;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				data = (data & ~0x1) | 0x0;
				CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
			}

			if(fSettings->EnableZeroSuppression(b, ch)) {
				// enable charge zero suppression
				address = 0x1080 + ch*0x100;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				//std::cout<<"zs - 0x"<<std::hex<<address<<" read 0x"<<data<<std::dec<<std::endl;
				data = (data & ~0x2000000) | 0x2000000;
				CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
				//std::cout<<"zs - 0x"<<std::hex<<address<<" wrote 0x"<<data<<std::dec<<std::endl;

				// set charge zero suppression threshold
				address = 0x1044 + ch*0x100;
				CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
				//std::cout<<"zs thres. - 0x"<<std::hex<<address<<" read 0x"<<data<<std::dec<<std::endl;
				data = (data & ~0xffff) | (fSettings->ChargeThreshold(b, ch) & 0xffff);
				CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
				//std::cout<<"zs thres. - 0x"<<std::hex<<address<<" wrote 0x"<<data<<std::dec<<std::endl;
			}
		}
	}

	errorCode = CAEN_DGTZ_SetDPPEventAggregation(fHandle[b], fSettings->EventAggregation(b), 0);

	// doesn't work??? we set it now by hand below
	errorCode = CAEN_DGTZ_SetDPP_VirtualProbe(fHandle[b], ANALOG_TRACE_2,  CAEN_DGTZ_DPP_VIRTUALPROBE_CFD);

	// this has been confirmed to work
	errorCode = CAEN_DGTZ_SetDPP_VirtualProbe(fHandle[b], DIGITAL_TRACE_1, CAEN_DGTZ_DPP_DIGITALPROBE_Gate);

	errorCode = CAEN_DGTZ_SetDPP_VirtualProbe(fHandle[b], DIGITAL_TRACE_2, CAEN_DGTZ_DPP_DIGITALPROBE_GateShort);

	// manually set analog traces to input and cfd
	address = 0x8000;
	CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
	data = (data & ~0x3000) | 0x2000;
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	// use external clock - this seems to be safer if done at the end of setting all parameters ???
	if(fSettings->UseExternalClock()) {
		if(fSettings->BoardType(b) == EBoardType::kVME) {
			// VME boards have an onboard switch to select the clock
			// so we check if it's enabled and give a warning otherwise
			address = 0x8104;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			if((data&0x20) != 0x20) {
				cm_msg(MERROR, "ProgramDigitizer", "Requested external clock for VME module, this has to be set by onboard switch S3 (0x%x)", data);
			}
		} else {
			address = 0x8100;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data |= 0x40; // no mask necessary, we just set one bit
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
		}
	}

	if(fDebug) std::cout<<"done with digitizer "<<b<<std::endl;
}

void CaenDigitizer::ProgramPhaDigitizer(int b)
{
	uint32_t address;
	uint32_t data;

	if(fDebug) std::cout<<"programming pha digitizer "<<b<<std::endl;
	CAEN_DGTZ_ErrorCode errorCode;

	errorCode = CAEN_DGTZ_Reset(fHandle[b]);

	if(errorCode != 0) {
		throw std::runtime_error(format("Error %d when resetting digitizer", errorCode));
	}

	errorCode = CAEN_DGTZ_SetDPPAcquisitionMode(fHandle[b], fSettings->AcquisitionMode(b), CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);

	if(errorCode != 0) {
		throw std::runtime_error(format("Error %d when setting DPP acquisition mode", errorCode));
	}

	errorCode = CAEN_DGTZ_SetIOLevel(fHandle[b], fSettings->IOLevel(b));

	if(errorCode != 0) {
		throw std::runtime_error(format("Error %d when setting IO level", errorCode));
	}

	errorCode = CAEN_DGTZ_SetExtTriggerInputMode(fHandle[b], fSettings->TriggerMode(b));

	if(errorCode != 0) {
		throw std::runtime_error(format("Error %d when setting external trigger DPP events", errorCode));
	}

	errorCode = CAEN_DGTZ_SetChannelEnableMask(fHandle[b], fSettings->ChannelMask(b));

	if(errorCode != 0) {
		throw std::runtime_error(format("Error %d when setting channel mask", errorCode));
	}

	// disabled turns acquisition mode back to SW controlled
	// both GpioGpioDaisyChain and SinFanout turn it to S_IN controlled
	// according to rev18 manual GpioGpioDaisyChain is not used!
	errorCode = CAEN_DGTZ_SetRunSynchronizationMode(fHandle[b], CAEN_DGTZ_RUN_SYNC_Disabled); // change to settings

	if(errorCode != 0) {
		throw std::runtime_error(format("Error %d when setting run sychronization", errorCode));
	}

	// for dpp only
	//  errorCode = CAEN_DGTZ_SetDPPParameters(fHandle[b], fSettings->ChannelMask(b), static_cast<void*>(fSettings->ChannelParameter(b)));

	if(errorCode != 0) {
		throw std::runtime_error(format("Error %d when setting dpp parameters", errorCode));
	}


	//HPGE Registers - hard coded for channel 0. Registers are not read, just written.
	address = 0x1020;
	data = 0x00000271; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	//address = 0x1028;
	//data = 0x00000000; 
	//CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	//address = 0x1034;
	//data = 0x000001FF; 
	//CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1038;
	data = 0x0000007D; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	// address = 0x1040;
	// data = 0x00000000; 
	//CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1054;
	data = 0x00000004; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1058;
	data = 0x00000006; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x105C;
	data = 0x00000138; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1060;
	data = 0x0000003E; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1064;
	data = 0x00000032; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1068;
	data = 0x00000C35; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x106C;
	data = 0x00000064; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1070;
	data = 0x00000000; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1074;
	data = 0x0000001E; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1078;
	data = 0x0000003E; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1080;
	data = 0x0C310013; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1084;
	data = 0x00000006; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x1098;
	data = 0x00003333; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x10A0;
	data = 0x00000000; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x10D4;
	data = 0x0000000A; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0xEF00;
	data = 0x00000010; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
	/*
	   address = 0xEF20;
	   data = 0xAAAAAAAA; 
	   CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	   address = 0xEF1C;
	   data = 0x00000040; 
	   CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	   address = 0xEF18;
	   data = 0x00000000; 
	   CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	   address = 0xEF14;
	   data = 0x000000DD; 
	   CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	   address = 0xEF10;
	   data = 0x00000000; 
	   CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	   address = 0xEF0C;
	   data = 0x000000AA; 
	   CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
	   */

	address = 0x811C;
	data = 0x00008000; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	address = 0x8120;
	data = 0x000000FF; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
	/*
	   address = 0x8168;
	   data = 0x00000001; 
	   CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
	   */
	address = 0x81A0;
	data = 0x00000000; 
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);


	// configure register event size independently
	//address = 0x800C;
	//CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
	//std::cout << "Buffer size is currently " << std::hex << data <<std::dec << std::endl;
	//data = (data & ~0xf ) | 0x0000;
	//CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
	//std::cout << " Add, data " << std::hex << address << " " << data << std::dec << std::endl;

	// write some special registers directly
	// enable EXTRA word
	//address = 0x8000;
	//CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
	//data |= 0x20000; // no mask necessary, we just set one bit
	//CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	for(int ch = 0; ch < fSettings->NumberOfChannels(); ++ch) {
		if((fSettings->ChannelMask(b) & (1<<ch)) != 0) {
			if(fDebug) std::cout<<"programming channel "<<ch<<std::endl;
			if(ch%2 == 0) {
				errorCode = CAEN_DGTZ_SetRecordLength(fHandle[b], fSettings->RecordLength(b, ch), ch);
			}

			errorCode = CAEN_DGTZ_SetChannelDCOffset(fHandle[b], ch, fSettings->DCOffset(b, ch));

			errorCode = CAEN_DGTZ_SetDPPPreTriggerSize(fHandle[b], ch, fSettings->PreTrigger(b, ch));

			errorCode = CAEN_DGTZ_SetChannelPulsePolarity(fHandle[b], ch, fSettings->PulsePolarity(b, ch));

			//if(fSettings->EnableCfd(b, ch)) {
			//  if(fDebug) std::cout<<"enabling CFD on channel "<<ch<<std::endl;
			//  // enable CFD mode
			//  address = 0x1080 + ch*0x100;
			//  CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			//  data |= 0x40; // no mask necessary, we just set one bit
			//  CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			//  // set CFD parameters
			//  address = 0x103c + ch*0x100;
			//  CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			//  data = (data & ~0xfff) | fSettings->CfdParameters(b, ch);
			//  CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			//}
			// write extended TS, flags, and fine TS (from CFD) to extra word
			//address = 0x1084 + ch*0x100;
			//CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			//data = (data & ~0x700) | 0x200;
			//CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
			/*

			   if(fSettings->EnableCoinc(b, ch)){
			   std::cout << "Coincidences enabled" << std::endl;
			// enable EXTRA word with 0x20000 and enable coincidence trigger with 0x4 in register 8000     
			address = 0x8000;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data |= 0x4; // no mask necessary, we just set one bit                                         
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			// set Coinc window parameters                                                                 
			address = 0x1070 + ch*0x100;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data |= 0x1; //A corresponds to 10 clock cycles (ie 80ns)                                      
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			//Set Coincidence Latency parameters                                                           
			address = 0x106c + ch*0x100;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data |= 0x2;
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			//Set Coincidence setting for each channel                                                     
			address = 0x1080 + ch*0x100;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data = (data & ~0xc0000 ) | 0x40000;
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			}
			if(fSettings->EnableCoincTrig(b,ch)){
			std::cout <<  "Coincidence trigger enabled " << std::endl;
			//For coincidence trigger, we must also set address 8184 to the couple Ch2/3                 
			address = 0x8184;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data = (data & ~0xffffffff) | 0x1;
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			// write extended TS, flags, and fine TS (from CFD) to extra word and Coinciudence tyrn on TRG_VAL                                                                                                                   
			address = 0x1084;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data = (data & ~0xff) | 0x64;
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

			std::cout <<  "Print baselines for channels 1284 and 1384 " << std::endl;
			address = 0x1284;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			data = (data & ~0xff) | 0x57;
			CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
			}
			*/

		}
	}

	//errorCode = CAEN_DGTZ_SetDPPEventAggregation(fHandle[b], fSettings->EventAggregation(b), 0);

	//// doesn't work??? we set it now by hand below
	//errorCode = CAEN_DGTZ_SetDPP_VirtualProbe(fHandle[b], ANALOG_TRACE_2,  CAEN_DGTZ_DPP_VIRTUALPROBE_CFD);

	//// this has been confirmed to work
	//errorCode = CAEN_DGTZ_SetDPP_VirtualProbe(fHandle[b], DIGITAL_TRACE_1, CAEN_DGTZ_DPP_DIGITALPROBE_Gate);

	//errorCode = CAEN_DGTZ_SetDPP_VirtualProbe(fHandle[b], DIGITAL_TRACE_2, CAEN_DGTZ_DPP_DIGITALPROBE_GateShort);

	//// manually set analog traces to input and cfd
	//address = 0x8000;
	//CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
	//data = (data & ~0x3000) | 0x2000;
	//CAEN_DGTZ_WriteRegister(fHandle[b], address, data);

	/*
	// use external clock - this seems to be safer if done at the end of setting all parameters ???
	address = 0x8100;
	CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
	data |= 0x40; // no mask necessary, we just set one bit
	CAEN_DGTZ_WriteRegister(fHandle[b], address, data);
	*/


	if(fDebug) std::cout<<"done with digitizer "<<b<<std::endl;
}

void CaenDigitizer::PrintAggregatesPerBlt()
{
	uint32_t address = 0xef1c;
	uint32_t data;

	std::cout<<"Aggregates per BLT: ";
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
		std::cout<<b<<" - "<<(data&0x3ff)<<"; ";
	}
	std::cout<<std::endl;
}

void CaenDigitizer::PrintEventsPerAggregate()
{
	uint32_t address;
	uint32_t data;

	std::cout<<"Events per aggregate:"<<std::endl;
	for(int b = 0; b < fSettings->NumberOfBoards(); ++b) {
		std::cout<<b<<": ";
		for(int ch = 0; ch < fSettings->NumberOfChannels(); ch += 2) {
			address = 0x1034 + ch*0x100;
			CAEN_DGTZ_ReadRegister(fHandle[b], address, &data);
			std::cout<<ch<<" - "<<(data&0x3ff)<<"; ";
		}
		std::cout<<std::endl;
	}
}

void CaenDigitizer::Status()
{
	// print current rate
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	// only print about once every second
	if(now.tv_sec != fLastUpdate.tv_sec && fLastUpdate.tv_sec != 0) {
		double timeDiff = (now.tv_sec + now.tv_nsec/1e9) - (fLastUpdate.tv_sec + fLastUpdate.tv_nsec/1e9);
		uint32_t totalNofEvents = 0;
		for(size_t i = 0; i < fNofEvents.size(); ++i) {
			std::cout<<"board "<<i<<": "<<std::setw(9)<<static_cast<int>((fNofEvents[i] - fLastNofEvents[i])/timeDiff)<<"/s | ";
			totalNofEvents += fNofEvents[i];
		}
		std::cout<<"total: "<<std::setw(9)<<static_cast<int>((totalNofEvents - fLastTotalNofEvents)/timeDiff)<<"/s   ";
		std::cout<<"\r"<<std::flush;
		if(fIteration%300 == 0) std::cout<<std::endl; // preserve last output rougly every 5 minutes
		fLastUpdate = now;
		fLastNofEvents = fNofEvents;
		fLastTotalNofEvents = totalNofEvents;
		++fIteration;
	}
}
