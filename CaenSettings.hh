#ifndef CAENSETTINGS_HH
#define CAENSETTINGS_HH
#include <iostream>
#include <vector>
#include <string>

#include "midas.h"

#include "CAENDigitizer.h"

#ifdef USE_TENV
#include "TEnv.h"
#endif

#include "CaenOdb.h"

enum class EBoardType : char { kDesktop, kNIM, kVME };

class ChannelSettings {
public:
	ChannelSettings() {}
	ChannelSettings(const V1730_TEMPLATE& templateSettings);
#ifdef USE_TENV
	ChannelSettings(const int& boardNumber, const int& channelNumber, TEnv*& settings);
#endif
	~ChannelSettings();

	void ReadCustomSettings(const HNDLE& db, const HNDLE& key);

	void Print(const ChannelSettings& templateSettings);
	void Print();

	//setters
	void RecordLength(const uint32_t& val) { fRecordLength = val; }
	void DCOffset(const uint32_t& val) { fDCOffset = val; }
	void PreTrigger(const uint32_t& val) { fPreTrigger = val; }
	void PulsePolarity(const CAEN_DGTZ_PulsePolarity_t& val) { fPulsePolarity = val; }
	void EnableCfd(const bool& val) { fEnableCfd = val; }
	void CfdParameters(const uint32_t& val) { fCfdParameters = val; }
	void EnableBaseline(const bool& val) { fEnableBaseline = val; }
	void InputRange(const bool& val) { fInputRange = val; }
	void EnableZeroSuppression(const bool& val) { fEnableZeroSuppression = val; }
	void ChargeThreshold(const uint32_t& val) { fChargeThreshold = val; }
	void TriggerWidth(const uint16_t& val) { fTriggerWidth = val; }
	void TriggerMode(const uint8_t& val) { fTriggerMode = val; }
	void TriggerMask(const uint16_t& val) { fTriggerMask = val; }
	void CoincidenceMode(const uint8_t& val) { fCoincidenceMode = val; }

	//getters
	uint32_t RecordLength() const { return fRecordLength; }
	uint32_t DCOffset() const { return fDCOffset; }
	uint32_t PreTrigger() const { return fPreTrigger; }
	CAEN_DGTZ_PulsePolarity_t PulsePolarity() const { return fPulsePolarity; }
	bool EnableCfd() const { return fEnableCfd; }
	uint32_t CfdParameters() const { return fCfdParameters; }
	bool EnableBaseline() const { return fEnableBaseline; }
	bool InputRange() const { return fInputRange; }
	bool EnableZeroSuppression() const { return fEnableZeroSuppression; }
	uint32_t ChargeThreshold() const { return fChargeThreshold; }
	uint16_t TriggerWidth() const { return fTriggerWidth; }
	uint8_t TriggerMode() const { return fTriggerMode; }
	uint16_t TriggerMask() const { return fTriggerMask; }
	uint8_t CoincidenceMode() const { return fCoincidenceMode; }

	friend bool operator==(const ChannelSettings& lh, const ChannelSettings& rh);
	friend bool operator!=(const ChannelSettings& lh, const ChannelSettings& rh);

private:
	uint32_t fRecordLength;
	uint32_t fDCOffset;
	uint32_t fPreTrigger;
	CAEN_DGTZ_PulsePolarity_t fPulsePolarity; //enum
	bool fEnableCfd;
	uint16_t fCfdParameters;
  	bool fEnableBaseline;
	bool fInputRange;
	bool fEnableZeroSuppression;
	uint32_t fChargeThreshold;
	uint16_t fTriggerWidth;
	uint8_t fTriggerMode;
	uint16_t fTriggerMask;
	uint8_t fCoincidenceMode;
};

class BoardSettings {
public:
	BoardSettings() { }
	BoardSettings(const int& nofChannels, const V1730_TEMPLATE& templateSettings);
#ifdef USE_TENV
	BoardSettings(const int& boardNumber, const int& nofChannels, TEnv*& settings);
#endif
	~BoardSettings();

	void NumberOfChannels(const int& nofChannels) { fChannelSettings.resize(nofChannels); }
	void ReadCustomSettings(const HNDLE& db, const HNDLE& key);
	void ReadCustomChannelSettings(const int& channel, const HNDLE& db, const HNDLE& key);

	void Print(const BoardSettings& templateSettings);
	void Print();

	void FamilyType(CAEN_DGTZ_BoardFamilyCode_t code) { fFamilyType = code; }

	//setters
	void LinkType(const CAEN_DGTZ_ConnectionType& val) { fLinkType = val; }
	void BoardType(const EBoardType& val) { fBoardType = val; }
	void VmeBaseAddress(const uint32_t& val) { fVmeBaseAddress = val; }
	void AcquisitionMode(const CAEN_DGTZ_DPP_AcqMode_t& val) { fAcquisitionMode = val; }
	void IOLevel(const CAEN_DGTZ_IOLevel_t& val) { fIOLevel = val; }
	void ChannelMask(const uint32_t& val) { fChannelMask = val; }
	void RunSync(const CAEN_DGTZ_RunSyncMode_t& val) { fRunSync = val; }
	void EventAggregation(const int& val) { fEventAggregation = val; }
	void TriggerMode(const CAEN_DGTZ_TriggerMode_t& val) { fTriggerMode = val; }
	void ChannelPsdParameter(const CAEN_DGTZ_DPP_PSD_Params_t& val) { fChannelPsdParameter = val; }
	void ChannelPhaParameter(const CAEN_DGTZ_DPP_PHA_Params_t& val) { fChannelPhaParameter = val; }
	void PortNumber(const int& val) { fPortNumber = val; }
	void DeviceNumber(const int& val) { fDeviceNumber = val; }
	void TriggerPropagation(const bool& val) { fTriggerPropagation = val; }

	//getters
	std::vector<ChannelSettings> ChannelSettingsVector() const { return fChannelSettings; }
	CAEN_DGTZ_ConnectionType LinkType() const { return fLinkType; }
	EBoardType BoardType() const { return fBoardType; }
	uint32_t VmeBaseAddress() const { return fVmeBaseAddress; }
	CAEN_DGTZ_DPP_AcqMode_t AcquisitionMode() const { return fAcquisitionMode; }
	CAEN_DGTZ_IOLevel_t IOLevel() const { return fIOLevel; }
	uint32_t ChannelMask() const { return fChannelMask; }
	CAEN_DGTZ_RunSyncMode_t RunSync() const { return fRunSync; }
	int EventAggregation() const { return fEventAggregation; }
	CAEN_DGTZ_TriggerMode_t TriggerMode() const { return fTriggerMode; }
	const CAEN_DGTZ_DPP_PSD_Params_t* ChannelPsdParameter() const { return &fChannelPsdParameter; }
	const CAEN_DGTZ_DPP_PHA_Params_t* ChannelPhaParameter() const { return &fChannelPhaParameter; }
	int PortNumber() const { return fPortNumber; }
	int DeviceNumber() const { return fDeviceNumber; }
	bool TriggerPropagation() const { return fTriggerPropagation; }

	//channel setters
	void RecordLength(const int& i, const uint32_t& val) { fChannelSettings.at(i).RecordLength(val); }
	void DCOffset(const int& i, const uint32_t& val) { fChannelSettings.at(i).DCOffset(val); }
	void PreTrigger(const int& i, const uint32_t& val) { fChannelSettings.at(i).PreTrigger(val); }
	void PulsePolarity(const int& i, const CAEN_DGTZ_PulsePolarity_t& val) { fChannelSettings.at(i).PulsePolarity(val); }
	void EnableCfd(const int& i, const bool& val) { fChannelSettings.at(i).EnableCfd(val); }
	void CfdParameters(const int& i, const uint32_t& val) { fChannelSettings.at(i).CfdParameters(val); }
	void EnableBaseline(const int& i, const bool& val) { fChannelSettings.at(i).EnableBaseline(val); }
	void InputRange(const int& i, const bool& val) { fChannelSettings.at(i).InputRange(val); }
	void EnableZeroSuppression(const int& i, const bool& val) { fChannelSettings.at(i).EnableZeroSuppression(val); }
	void ChargeThreshold(const int& i, const uint32_t& val) { fChannelSettings.at(i).ChargeThreshold(val); }
	void TriggerWidth(const int& i, const uint16_t& val) { fChannelSettings.at(i).TriggerWidth(val); }
	void TriggerMode(const int& i, const uint8_t& val) { fChannelSettings.at(i).TriggerMode(val); }
	void TriggerMask(const int& i, const uint16_t& val) { fChannelSettings.at(i).TriggerMask(val); }
	void CoincidenceMode(const int& i, const uint8_t& val) { fChannelSettings.at(i).CoincidenceMode(val); }
	
	//channel getters
	uint32_t RecordLength(const int& i) const { return fChannelSettings.at(i).RecordLength(); }
	uint32_t DCOffset(const int& i) const { return fChannelSettings.at(i).DCOffset(); }
	uint32_t PreTrigger(const int& i) const { return fChannelSettings.at(i).PreTrigger(); }
	CAEN_DGTZ_PulsePolarity_t PulsePolarity(const int& i) const { return fChannelSettings.at(i).PulsePolarity(); }
	bool EnableCfd(const int& i) const { return fChannelSettings.at(i).EnableCfd(); }
	uint32_t CfdParameters(const int& i) const { return fChannelSettings.at(i).CfdParameters(); }
	bool EnableBaseline(const int& i) const { return fChannelSettings.at(i).EnableBaseline(); }
	bool InputRange(const int& i) const { return fChannelSettings.at(i).InputRange(); }
	bool EnableZeroSuppression(const int& i) const { return fChannelSettings.at(i).EnableZeroSuppression(); }
	uint32_t ChargeThreshold(const int& i) const { return fChannelSettings.at(i).ChargeThreshold(); }
	uint16_t TriggerWidth(const int& i) const { return fChannelSettings.at(i).TriggerWidth(); }
	uint8_t TriggerMode(const int& i) const { return fChannelSettings.at(i).TriggerMode(); }
	uint16_t TriggerMask(const int& i) const { return fChannelSettings.at(i).TriggerMask(); }
	uint8_t CoincidenceMode(const int& i) const { return fChannelSettings.at(i).CoincidenceMode(); }
	
	friend bool operator==(const BoardSettings& lh, const BoardSettings& rh);
	friend bool operator!=(const BoardSettings& lh, const BoardSettings& rh);

private:
	CAEN_DGTZ_BoardFamilyCode_t fFamilyType; //enum
	CAEN_DGTZ_ConnectionType fLinkType; //enum
	EBoardType fBoardType; // enum
	uint32_t fVmeBaseAddress;
	int fPortNumber;
	int fDeviceNumber;
	CAEN_DGTZ_DPP_AcqMode_t fAcquisitionMode; //enum
	CAEN_DGTZ_IOLevel_t fIOLevel; //enum
	uint32_t fChannelMask;
	CAEN_DGTZ_RunSyncMode_t fRunSync; //enum
	int fEventAggregation;
	CAEN_DGTZ_TriggerMode_t fTriggerMode; //enum
	bool fTriggerPropagation;
	CAEN_DGTZ_DPP_PSD_Params_t fChannelPsdParameter;
	CAEN_DGTZ_DPP_PHA_Params_t fChannelPhaParameter;
	std::vector<ChannelSettings> fChannelSettings;
};

class CaenSettings {
public:
	CaenSettings(bool debug = false);
	~CaenSettings();

	bool ReadOdb(HNDLE hDB);
#ifdef USE_TENV
	bool ReadSettingsFile(const std::string&);
#endif
	bool WriteOdb();
	void Print();

	int NumberOfBoards() const { return fNumberOfBoards; }
	int NumberOfChannels() const { return fNumberOfChannels; }
	bool UseExternalClock() const { return fUseExternalClock; }

	size_t NumberOfBoardSettings() const { return fBoardSettings.size(); }

	void FamilyType(int i, CAEN_DGTZ_BoardFamilyCode_t code) { fBoardSettings.at(i).FamilyType(code); }

	//board parameters
	CAEN_DGTZ_ConnectionType LinkType(int i) const { return fBoardSettings.at(i).LinkType(); }
	uint32_t VmeBaseAddress(int i) const { return fBoardSettings.at(i).VmeBaseAddress(); }
	int PortNumber(int i) const { return fBoardSettings.at(i).PortNumber(); }
	int DeviceNumber(int i) const { return fBoardSettings.at(i).DeviceNumber(); }
	CAEN_DGTZ_DPP_AcqMode_t AcquisitionMode(int i) const { return fBoardSettings.at(i).AcquisitionMode(); }
	CAEN_DGTZ_IOLevel_t IOLevel(int i) const { return fBoardSettings.at(i).IOLevel(); }
	uint32_t ChannelMask(int i) const { return fBoardSettings.at(i).ChannelMask(); }
	CAEN_DGTZ_RunSyncMode_t RunSync(int i) const { return fBoardSettings.at(i).RunSync(); }
	int EventAggregation(int i) const { return fBoardSettings.at(i).EventAggregation(); }
	CAEN_DGTZ_TriggerMode_t TriggerMode(int i) const { return fBoardSettings.at(i).TriggerMode(); }
	const CAEN_DGTZ_DPP_PSD_Params_t* ChannelPsdParameter(int i) const { return fBoardSettings.at(i).ChannelPsdParameter(); }
	const CAEN_DGTZ_DPP_PHA_Params_t* ChannelPhaParameter(int i) const { return fBoardSettings.at(i).ChannelPhaParameter(); }
	EBoardType BoardType(int i) const { return fBoardSettings.at(i).BoardType(); }
	bool TriggerPropagation(int i) const { return fBoardSettings.at(i).TriggerPropagation(); }

	//channel parameters
	uint32_t RecordLength(int i, int j) const { return fBoardSettings.at(i).RecordLength(j); }
	uint32_t DCOffset(int i, int j) const { return fBoardSettings.at(i).DCOffset(j); }
	uint32_t PreTrigger(int i, int j) const { return fBoardSettings.at(i).PreTrigger(j); }
	CAEN_DGTZ_PulsePolarity_t PulsePolarity(int i, int j) const { return fBoardSettings.at(i).PulsePolarity(j); }
	bool EnableCfd(int i, int j) const { return fBoardSettings.at(i).EnableCfd(j); }
	uint16_t CfdParameters(int i, int j) const { return fBoardSettings.at(i).CfdParameters(j); }
	bool EnableBaseline(int i, int j) const { return fBoardSettings.at(i).EnableBaseline(j); }
	bool InputRange(int i, int j) const { return fBoardSettings.at(i).InputRange(j); }
	bool EnableZeroSuppression(int i, int j) const { return fBoardSettings.at(i).EnableZeroSuppression(j); }
	uint32_t ChargeThreshold(int i, int j) const { return fBoardSettings.at(i).ChargeThreshold(j); }
	uint16_t TriggerWidth(int i, int j) const { return fBoardSettings.at(i).TriggerWidth(j); }
	uint8_t TriggerMode(int i, int j) const { return fBoardSettings.at(i).TriggerMode(j); }
	uint16_t TriggerMask(int i, int j) const { return fBoardSettings.at(i).TriggerMask(j); }
	uint8_t CoincidenceMode(int i, int j) const { return fBoardSettings.at(i).CoincidenceMode(j); }

	size_t BufferSize() const { return fBufferSize; }

	bool RawOutput() const { return fRawOutput; }

private:
	int fNumberOfBoards;
	int fNumberOfChannels;
	bool fUseExternalClock;

	BoardSettings fTemplateSettings;
	std::vector<BoardSettings> fBoardSettings;

	size_t fBufferSize;

	bool fRawOutput;

	bool fDebug;
};
#endif
