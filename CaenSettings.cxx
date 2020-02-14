#include "CaenSettings.hh"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <curses.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iterator>

#include "midas.h"

#ifdef USE_TENV
#include "TEnv.h"
#endif

#include "CaenOdb.h"

// this assumes that the right hand side is the template!!!
//PSD settings
bool operator==(const CAEN_DGTZ_DPP_PSD_Params_t& lh, const CAEN_DGTZ_DPP_PSD_Params_t& rh)
{
	for(int i = 0; i < MAX_DPP_PSD_CHANNEL_SIZE; ++i) {
		if(lh.thr[i] != rh.thr[0]) return false;
		if(lh.selft[i] != rh.selft[0]) return false;
		if(lh.csens[i] != rh.csens[0]) return false;
		if(lh.sgate[i] != rh.sgate[0]) return false;
		if(lh.lgate[i] != rh.lgate[0]) return false;
		if(lh.pgate[i] != rh.pgate[0]) return false;
		if(lh.tvaw[i] != rh.tvaw[0]) return false;
		if(lh.nsbl[i] != rh.nsbl[0]) return false;
	}
	return (lh.trgho  == rh.trgho &&
	        lh.purh   == rh.purh  &&
	        lh.purgap == rh.purgap);
}

bool operator!=(const CAEN_DGTZ_DPP_PSD_Params_t& lh, const CAEN_DGTZ_DPP_PSD_Params_t& rh)
{
	return !(lh == rh);
}

// this assumes that the right hand side is the template!!!
//PHA settings
bool operator==(const CAEN_DGTZ_DPP_PHA_Params_t& lh, const CAEN_DGTZ_DPP_PHA_Params_t& rh)
{
	for(int i = 0; i < MAX_DPP_PHA_CHANNEL_SIZE; ++i) {
		if(lh.thr[i] != rh.thr[0]) return false;
		if(lh.trgho[i] != rh.trgho[0]) return false;
		if(lh.nsbl[i] != rh.nsbl[0]) return false;
		if(lh.M[i] != rh.M[0]) return false;
		if(lh.m[i] != rh.m[0]) return false;
		if(lh.k[i] != rh.k[0]) return false;
		if(lh.ftd[i] != rh.ftd[0]) return false;
		if(lh.a[i] != rh.a[0]) return false;
		if(lh.b[i] != rh.b[0]) return false;
		if(lh.nspk[i] != rh.nspk[0]) return false;
		if(lh.pkho[i] != rh.pkho[0]) return false;
		if(lh.blho[i] != rh.blho[0]) return false;
		if(lh.twwdt[i] != rh.twwdt[0]) return false;
		if(lh.trgwin[i] != rh.trgwin[0]) return false;
		if(lh.dgain[i] != rh.dgain[0]) return false;
		if(lh.enf[i] != rh.enf[0]) return false;
		if(lh.decimation[i] != rh.decimation[0]) return false;
		if(lh.M[i] != rh.M[0]) return false;
		if(lh.M[i] != rh.M[0]) return false;

		//int enskim      [MAX_DPP_PHA_CHANNEL_SIZE]; // Enable energy skimming
		//int eskimlld    [MAX_DPP_PHA_CHANNEL_SIZE]; // LLD    energy skimming
		//int eskimuld    [MAX_DPP_PHA_CHANNEL_SIZE]; // ULD    energy skimming
		//int blrclip     [MAX_DPP_PHA_CHANNEL_SIZE]; // Enable baseline restorer clipping
		//int dcomp       [MAX_DPP_PHA_CHANNEL_SIZE]; // tt_filter compensation
		//int trapbsl     [MAX_DPP_PHA_CHANNEL_SIZE]; // trapezoid baseline adjuster
	}
	return true;
}

bool operator!=(const CAEN_DGTZ_DPP_PHA_Params_t& lh, const CAEN_DGTZ_DPP_PHA_Params_t& rh)
{
	return !(lh == rh);
}

ChannelSettings::ChannelSettings(const V1730_TEMPLATE& templateSettings)
{
	fRecordLength          = templateSettings.record_length;
	fDCOffset              = templateSettings.dc_offset;
	fPreTrigger            = templateSettings.pre_trigger;
	fPulsePolarity         = static_cast<CAEN_DGTZ_PulsePolarity_t>(templateSettings.pulse_polarity);
	fEnableCfd             = templateSettings.enable_cfd;
	fCfdParameters         = templateSettings.cfd_delay & 0xff;
	fCfdParameters        |= (templateSettings.cfd_fraction & 0x3) << 8;
	fCfdParameters        |= (templateSettings.cfd_interpolation_points & 0x3) << 10;
	fEnableCoinc           = templateSettings.enable_coinc;
	fEnableBaseline        = templateSettings.enable_baseline;
	fCoincWindow           = templateSettings.coinc_window;
	fCoincLatency          = templateSettings.coinc_latency;
	fInputRange            = templateSettings.input_range;
	fEnableZeroSuppression = templateSettings.enable_zs;
	fChargeThreshold       = templateSettings.charge_threshold;
}

#ifdef USE_TENV
ChannelSettings::ChannelSettings(const int& boardNumber, const int& channelNumber, TEnv*& settings)
{
	fRecordLength          = settings->GetValue(Form("Board.%d.Channel.%d.RecordLength", boardNumber, channelNumber), 192);
	fDCOffset              = settings->GetValue(Form("Board.%d.Channel.%d.DCOffset", boardNumber, channelNumber), 0x8000);
	fPreTrigger            = settings->GetValue(Form("Board.%d.Channel.%d.RunSync", boardNumber, channelNumber), 80);
	fPulsePolarity         = static_cast<CAEN_DGTZ_PulsePolarity_t>(settings->GetValue(Form("Board.%d.Channel.%d.PulsePolarity", boardNumber, channelNumber), CAEN_DGTZ_PulsePolarityNegative));//1
	fEnableCfd             = settings->GetValue(Form("Board.%d.Channel.%d.EnableCfd", boardNumber, channelNumber), true);
	fCfdParameters         = (settings->GetValue(Form("Board.%d.Channel.%d.CfdDelay", boardNumber, channelNumber), 5) & 0xff);
	fCfdParameters        |= (settings->GetValue(Form("Board.%d.Channel.%d.CfdFraction", boardNumber, channelNumber), 0) & 0x3) << 8;
	fCfdParameters        |= (settings->GetValue(Form("Board.%d.Channel.%d.CfdInterpolationPoints", boardNumber, channelNumber), 0) & 0x3) << 10;
	fEnableCoinc           = settings->GetValue(Form("Board.%d.Channel.%d.EnableCoinc", boardNumber, channelNumber), false);
	fEnableBaseline        = settings->GetValue(Form("Board.%d.Channel.%d.EnableBaseline", boardNumber, channelNumber), false);
	fCoincWindow           = settings->GetValue(Form("Board.%d.Channel.%d.CoincWindow", boardNumber, channelNumber), 5);
	fCoincLatency          = settings->GetValue(Form("Board.%d.Channel.%d.CoincLatency", boardNumber, channelNumber), 2);
	fInputRange            = settings->GetValue(Form("Board.%d.Channel.%d.InputRange", boardNumber, channelNumber), 2);
	fEnableZeroSuppression = settings->GetValue(Form("Board.%d.Channel.%d.EnableZeroSuppression", boardNumber, channelNumber), 2);
	fChargeThreshold       = settings->GetValue(Form("Board.%d.Channel.%d.ChargeThreshold", boardNumber, channelNumber), 2);
}
#endif

void ChannelSettings::ReadCustomSettings(const HNDLE& hDb, const HNDLE& hKey)
{
	// loop over sub-keys
	HNDLE hSubKey;
	KEY key;
	int size;
	for(int i = 0; ; ++i) {
		db_enum_key(hDb, hKey, i, &hSubKey);
		if(!hSubKey) break; // end of list reached
		db_get_key(hDb, hSubKey, &key);
		if(strcmp(key.name, "Record length") == 0 && key.num_values == 1) {
			size = sizeof(fRecordLength);
			db_get_data(hDb, hSubKey, &fRecordLength, &size, TID_WORD);
		} else if(strcmp(key.name, "DC offset") == 0 && key.num_values == 1) {
			size = sizeof(fDCOffset);
			db_get_data(hDb, hSubKey, &fDCOffset, &size, TID_WORD);
		} else if(strcmp(key.name, "Pre trigger") == 0 && key.num_values == 1) {
			size = sizeof(fPreTrigger);
			db_get_data(hDb, hSubKey, &fPreTrigger, &size, TID_WORD);
		} else if(strcmp(key.name, "Pulse polarity") == 0 && key.num_values == 1) {
			size = sizeof(fPulsePolarity);
			db_get_data(hDb, hSubKey, &fPulsePolarity, &size, TID_WORD);
			//fPulsePolarity = static_cast<CAEN_DGTZ_PulsePolarity_t>(*reinterpret_cast<WORD*>(key.data));
		} else if(strcmp(key.name, "Enable CFD") == 0 && key.num_values == 1) {
			size = sizeof(fEnableCfd);
			db_get_data(hDb, hSubKey, &fEnableCfd, &size, TID_BOOL);
		} else if(strcmp(key.name, "CFD delay") == 0 && key.num_values == 1) {
			int tmp;
			size = sizeof(tmp);
			db_get_data(hDb, hSubKey, &tmp, &size, TID_WORD);
			fCfdParameters &= ~0xff;
			fCfdParameters |= (tmp & 0xff);
		} else if(strcmp(key.name, "CFD fraction") == 0 && key.num_values == 1) {
			int tmp;
			size = sizeof(tmp);
			db_get_data(hDb, hSubKey, &tmp, &size, TID_WORD);
			fCfdParameters &= ~0x300;
			fCfdParameters |= (tmp & 0x3) << 8;
		} else if(strcmp(key.name, "CFD interpolation points") == 0 && key.num_values == 1) {
			int tmp;
			size = sizeof(tmp);
			db_get_data(hDb, hSubKey, &tmp, &size, TID_WORD);
			fCfdParameters &= ~0xc00;
			fCfdParameters |= (tmp & 0x3) << 10;
		} else if(strcmp(key.name, "Enable coincidence") == 0 && key.num_values == 1) {
			size = sizeof(fEnableCoinc);
			db_get_data(hDb, hSubKey, &fEnableCoinc, &size, TID_BOOL);
		} else if(strcmp(key.name, "Enable baseline") == 0 && key.num_values == 1) {
			size = sizeof(fEnableBaseline);
			db_get_data(hDb, hSubKey, &fEnableBaseline, &size, TID_BOOL);
		} else if(strcmp(key.name, "Coincidence window") == 0 && key.num_values == 1) {
			size = sizeof(fCoincWindow);
			db_get_data(hDb, hSubKey, &fCoincWindow, &size, TID_WORD);
		} else if(strcmp(key.name, "Coincidence latency") == 0 && key.num_values == 1) {
			size = sizeof(fCoincLatency);
			db_get_data(hDb, hSubKey, &fCoincLatency, &size, TID_WORD);
		} else if(strcmp(key.name, "Input range") == 0 && key.num_values == 1) {
			size = sizeof(fInputRange);
			db_get_data(hDb, hSubKey, &fInputRange, &size, TID_BOOL);
		} else if(strcmp(key.name, "Enable zero suppression") == 0 && key.num_values == 1) {
			size = sizeof(fEnableZeroSuppression);
			db_get_data(hDb, hSubKey, &fEnableZeroSuppression, &size, TID_BOOL);
		} else if(strcmp(key.name, "Charge threshold") == 0 && key.num_values == 1) {
			size = sizeof(fChargeThreshold);
			db_get_data(hDb, hSubKey, &fChargeThreshold, &size, TID_WORD);
		} else {
			// we keep both channel and channelparameter (which are "per board") settings
			// in the "Channel x" directory, so there might be unrecognized entries
		}
	}
}

ChannelSettings::~ChannelSettings()
{
}

void ChannelSettings::Print(const ChannelSettings& templateSettings)
{
	if(fRecordLength != templateSettings.RecordLength()) {
		std::cout<<"      record length "<<fRecordLength<<std::endl;
	}
	if(fDCOffset != templateSettings.DCOffset()) {
		std::cout<<"      DC offset 0x"<<std::hex<<fDCOffset<<std::dec<<std::endl;
	}
	if(fPreTrigger != templateSettings.PreTrigger()) {
		std::cout<<"      pre trigger "<<fPreTrigger<<std::endl;
	}
	if(fPulsePolarity != templateSettings.PulsePolarity()) {
		std::cout<<"      pulse polarity ";
		switch(fPulsePolarity) {
			case CAEN_DGTZ_PulsePolarityPositive:
				std::cout<<"positive"<<std::endl;
				break;
			case CAEN_DGTZ_PulsePolarityNegative:
				std::cout<<"negative"<<std::endl;
				break;
			default:
				std::cout<<"unknown"<<std::endl;
				break;
		}
	}
	if(fEnableCfd != templateSettings.EnableCfd()) {
		if(fEnableCfd) {
			std::cout<<"      cfd enabled"<<std::endl;
		} else {
			std::cout<<"      cfd disabled"<<std::endl;
		}
	}
	if(fCfdParameters != templateSettings.CfdParameters()) {
		std::cout<<"      cfd parameters 0x"<<std::hex<<fCfdParameters<<std::dec<<std::endl;
	}
	if(fEnableCoinc != templateSettings.EnableCoinc()) {
		std::cout<<"      coincidence "<<(fEnableCoinc?"enabled":"disabled")<<std::endl;
	}
	if(fEnableBaseline != templateSettings.EnableBaseline()) {
		std::cout<<"      baseline "<<(fEnableBaseline?"enabled":"disabled")<<std::endl;
	}
	if(fCoincWindow != templateSettings.CoincWindow()) {
		std::cout<<"      coincidence window "<<fCoincWindow<<std::endl;
	}
	if(fCoincLatency != templateSettings.CoincLatency()) {
		std::cout<<"      coincidence latency "<<fCoincLatency<<std::endl;
	}
	if(fInputRange != templateSettings.InputRange()) {
		std::cout<<"      input range "<<fInputRange<<std::endl;
	}
	if(fEnableZeroSuppression != templateSettings.EnableZeroSuppression()) {
		std::cout<<"      enable zero suppression "<<fEnableZeroSuppression<<std::endl;
	}
	if(fChargeThreshold != templateSettings.ChargeThreshold()) {
		std::cout<<"      charge threshold "<<fChargeThreshold<<std::endl;
	}
}

void ChannelSettings::Print()
{
	std::cout<<"      record length "<<fRecordLength<<std::endl;
	std::cout<<"      DC offset 0x"<<std::hex<<fDCOffset<<std::dec<<std::endl;
	std::cout<<"      pre trigger "<<fPreTrigger<<std::endl;
	std::cout<<"      pulse polarity ";
	switch(fPulsePolarity) {
		case CAEN_DGTZ_PulsePolarityPositive:
			std::cout<<"positive"<<std::endl;
			break;
		case CAEN_DGTZ_PulsePolarityNegative:
			std::cout<<"negative"<<std::endl;
			break;
		default:
			std::cout<<"unknown"<<std::endl;
			break;
	}
	if(fEnableCfd) {
		std::cout<<"      cfd enabled"<<std::endl;
		std::cout<<"      cfd parameters 0x"<<std::hex<<fCfdParameters<<std::dec<<std::endl;
	} else {
		std::cout<<"      cfd disabled"<<std::endl;
	}
	std::cout<<"      coincidence "<<(fEnableCoinc?"enabled":"disabled")<<std::endl
		<<"      baseline "<<(fEnableBaseline?"enabled":"disabled")<<std::endl
		<<"      coincidence window "<<fCoincWindow<<std::endl
		<<"      coincidence latency "<<fCoincLatency<<std::endl
		<<"      input range "<<fInputRange<<std::endl
		<<"      enable zero suppression "<<fEnableZeroSuppression<<std::endl
		<<"      charge threshold "<<fChargeThreshold<<std::endl;
}

bool operator==(const ChannelSettings& lh, const ChannelSettings& rh)
{
	return (lh.fRecordLength          == rh.fRecordLength &&
			  lh.fDCOffset              == rh.fDCOffset &&
			  lh.fPreTrigger            == rh.fPreTrigger &&
			  lh.fPulsePolarity         == rh.fPulsePolarity &&
			  lh.fEnableCfd             == rh.fEnableCfd &&
			  lh.fCfdParameters         == rh.fCfdParameters &&
			  lh.fEnableCoinc           == rh.fEnableCoinc &&
			  lh.fEnableBaseline        == rh.fEnableBaseline &&
			  lh.fCoincWindow           == rh.fCoincWindow &&
			  lh.fCoincLatency          == rh.fCoincLatency &&
			  lh.fInputRange            == rh.fInputRange &&
			  lh.fEnableZeroSuppression == rh.fEnableZeroSuppression &&
			  lh.fChargeThreshold       == rh.fChargeThreshold);
}

bool operator!=(const ChannelSettings& lh, const ChannelSettings& rh)
{
	return !(lh == rh);
}

BoardSettings::BoardSettings(const int& nofChannels, const V1730_TEMPLATE& templateSettings)
{
	fLinkType         = static_cast<CAEN_DGTZ_ConnectionType>(templateSettings.link_type);
	fBoardType        = static_cast<EBoardType>(templateSettings.board_type);
	fVmeBaseAddress   = templateSettings.vme_base_address;
	fPortNumber       = 0;
	fDeviceNumber     = 0;
	fAcquisitionMode  = static_cast<CAEN_DGTZ_DPP_AcqMode_t>(templateSettings.acquisition_mode);
	fIOLevel          = static_cast<CAEN_DGTZ_IOLevel_t>(templateSettings.io_level);
	fChannelMask      = templateSettings.channel_mask;
	fRunSync          = static_cast<CAEN_DGTZ_RunSyncMode_t>(templateSettings.runsync_mode);
	fEventAggregation = templateSettings.event_aggregation;
	fTriggerMode      = static_cast<CAEN_DGTZ_TriggerMode_t>(templateSettings.trigger_mode);
	fChannelSettings.resize(nofChannels, ChannelSettings(templateSettings));
	fChannelPsdParameter.purh   = static_cast<CAEN_DGTZ_DPP_PUR_t>(templateSettings.pile_up_rejection_mode);
	fChannelPsdParameter.purgap = templateSettings.pile_up_gap;
	fChannelPsdParameter.trgho = templateSettings.trigger_holdoff;
	for(int ch = 0; ch < nofChannels; ++ch) {
		fChannelPsdParameter.thr[ch]   = templateSettings.threshold;
		fChannelPsdParameter.nsbl[ch]  = templateSettings.baseline_samples;
		fChannelPsdParameter.lgate[ch] = templateSettings.long_gate;
		fChannelPsdParameter.sgate[ch] = templateSettings.short_gate;
		fChannelPsdParameter.pgate[ch] = templateSettings.pre_gate;
		fChannelPsdParameter.selft[ch] = templateSettings.self_trigger;
		fChannelPsdParameter.trgc[ch]  = static_cast<CAEN_DGTZ_DPP_TriggerConfig_t>(1); //deprecated, must be 1
		fChannelPsdParameter.tvaw[ch]  = templateSettings.trigger_validation_window;
		fChannelPsdParameter.csens[ch] = templateSettings.charge_sensitivity;

		fChannelPhaParameter.thr[ch]        = templateSettings.threshold;
		fChannelPhaParameter.trgho[ch]      = templateSettings.trigger_holdoff;
		fChannelPhaParameter.nsbl[ch]       = templateSettings.baseline_samples;
		fChannelPhaParameter.trgwin[ch]     = templateSettings.trigger_validation_window;
		fChannelPhaParameter.M[ch]          = templateSettings.trap_decay_time;
		fChannelPhaParameter.m[ch]          = templateSettings.trap_flat_top;
		fChannelPhaParameter.k[ch]          = templateSettings.trap_rise_time;
		fChannelPhaParameter.ftd[ch]        = templateSettings.peaking_time;
		fChannelPhaParameter.a[ch]          = templateSettings.smoothing_factor;
		fChannelPhaParameter.b[ch]          = templateSettings.input_rise_time;
		fChannelPhaParameter.nspk[ch]       = templateSettings.peak_samples;
		fChannelPhaParameter.pkho[ch]       = templateSettings.peak_holdoff;
		fChannelPhaParameter.blho[ch]       = templateSettings.baseline_holdoff;
		fChannelPhaParameter.twwdt[ch]      = templateSettings.rise_time_validation_window;
		fChannelPhaParameter.dgain[ch]      = templateSettings.digital_gain;
		fChannelPhaParameter.enf[ch]        = templateSettings.energy_normalization;
		fChannelPhaParameter.decimation[ch] = templateSettings.decimation;
	}
}

#ifdef USE_TENV
BoardSettings::BoardSettings(const int& boardNumber, const int& nofChannels, TEnv*& settings)
{
	fLinkType         = static_cast<CAEN_DGTZ_ConnectionType>(settings->GetValue(Form("Board.%d.LinkType", boardNumber), CAEN_DGTZ_USB));//0
	fBoardType        = static_cast<EBoardType>(settings->GetValue(Form("Board.%d.BoardType", boardNumber), 2));//EBoardType::kVME
	fVmeBaseAddress   = 0;
	fPortNumber       = boardNumber;
	fDeviceNumber     = 0;
	fAcquisitionMode  = static_cast<CAEN_DGTZ_DPP_AcqMode_t>(settings->GetValue(Form("Board.%d.AcquisitionMode", boardNumber), CAEN_DGTZ_DPP_ACQ_MODE_Mixed));//2
	fIOLevel          = static_cast<CAEN_DGTZ_IOLevel_t>(settings->GetValue(Form("Board.%d.IOlevel", boardNumber), CAEN_DGTZ_IOLevel_NIM));//0
	fTriggerMode      = static_cast<CAEN_DGTZ_TriggerMode_t>(settings->GetValue(Form("Board.%d.TriggerMode", boardNumber), CAEN_DGTZ_TRGMODE_ACQ_ONLY));//1
	fChannelMask      = settings->GetValue(Form("Board.%d.ChannelMask", boardNumber), 0xff);
	fRunSync          = static_cast<CAEN_DGTZ_RunSyncMode_t>(settings->GetValue(Form("Board.%d.RunSync", boardNumber), CAEN_DGTZ_RUN_SYNC_Disabled));//0
	fEventAggregation = settings->GetValue(Form("Board.%d.EventAggregate", boardNumber), 0);

	fChannelSettings.resize(nofChannels);
	fChannelPsdParameter.purh   = static_cast<CAEN_DGTZ_DPP_PUR_t>(settings->GetValue(Form("Board.%d.PileUpRejection", boardNumber), CAEN_DGTZ_DPP_PSD_PUR_DetectOnly));//0
	fChannelPsdParameter.purgap = settings->GetValue(Form("Board.%d.PurityGap", boardNumber), 100);
	fChannelPsdParameter.blthr  = settings->GetValue(Form("Board.%d.BaseLine.Threshold", boardNumber), 3);
	fChannelPsdParameter.bltmo  = settings->GetValue(Form("Board.%d.BaseLine.Timeout", boardNumber), 100);
	fChannelPsdParameter.trgho  = settings->GetValue(Form("Board.%d.TriggerHoldOff", boardNumber), 8);
	for(int ch = 0; ch < nofChannels; ++ch) {
		fChannelSettings[ch] = ChannelSettings(boardNumber, ch, settings);
		fChannelPsdParameter.thr[ch]   = settings->GetValue(Form("Board.%d.Channel.%d.Threshold", boardNumber, ch), 50);
		fChannelPsdParameter.nsbl[ch]  = settings->GetValue(Form("Board.%d.Channel.%d.BaselineSamples", boardNumber, ch), 4);
		fChannelPsdParameter.lgate[ch] = settings->GetValue(Form("Board.%d.Channel.%d.LongGate", boardNumber, ch), 32);
		fChannelPsdParameter.sgate[ch] = settings->GetValue(Form("Board.%d.Channel.%d.ShortGate", boardNumber, ch), 24);
		fChannelPsdParameter.pgate[ch] = settings->GetValue(Form("Board.%d.Channel.%d.PreGate", boardNumber, ch), 8);
		fChannelPsdParameter.selft[ch] = settings->GetValue(Form("Board.%d.Channel.%d.SelfTrigger", boardNumber, ch), 1);
		fChannelPsdParameter.trgc[ch]  = static_cast<CAEN_DGTZ_DPP_TriggerConfig_t>(1);//deprecated, must be 1
		fChannelPsdParameter.tvaw[ch]  = settings->GetValue(Form("Board.%d.Channel.%d.TriggerValidationAcquisitionWindow", boardNumber, ch), 50);
		fChannelPsdParameter.csens[ch] = settings->GetValue(Form("Board.%d.Channel.%d.ChargeSensitivity", boardNumber, ch), 0);

		fChannelPhaParameter.thr[ch]        = settings->GetValue(Form("Board.%d.Channel.%d.Threshold", boardNumber, ch), 50);
		fChannelPhaParameter.trgho[ch]      = settings->GetValue(Form("Board.%d.Channel.%d.TriggerHoldoff", boardNumber, ch), 8);
		fChannelPhaParameter.nsbl[ch]       = settings->GetValue(Form("Board.%d.Channel.%d.BaselineSamples", boardNumber, ch), 4);
		fChannelPhaParameter.trgwin[ch]     = settings->GetValue(Form("Board.%d.Channel.%d.TriggerValidationAcquisitionWindow", boardNumber, ch), 50);
		fChannelPhaParameter.M[ch]          = settings->GetValue(Form("Board.%d.Channel.%d.TrapezoidDecayTime", boardNumber, ch), 50000);
		fChannelPhaParameter.m[ch]          = settings->GetValue(Form("Board.%d.Channel.%d.TrapezoidFlatTop", boardNumber, ch), 1200);
		fChannelPhaParameter.k[ch]          = settings->GetValue(Form("Board.%d.Channel.%d.TrapezoidRiseTime", boardNumber, ch), 6000);
		fChannelPhaParameter.ftd[ch]        = settings->GetValue(Form("Board.%d.Channel.%d.PeakingTime", boardNumber, ch), 1000);
		fChannelPhaParameter.a[ch]          = settings->GetValue(Form("Board.%d.Channel.%d.SmoothingFactor", boardNumber, ch), 0);
		fChannelPhaParameter.b[ch]          = settings->GetValue(Form("Board.%d.Channel.%d.InputRiseTime", boardNumber, ch), 400);
		fChannelPhaParameter.nspk[ch]       = settings->GetValue(Form("Board.%d.Channel.%d.PeakSamples", boardNumber, ch), 0);
		fChannelPhaParameter.pkho[ch]       = settings->GetValue(Form("Board.%d.Channel.%d.PeakHoldoff", boardNumber, ch), 6000);
		fChannelPhaParameter.blho[ch]       = settings->GetValue(Form("Board.%d.Channel.%d.BaselineHoldoff", boardNumber, ch), 6);
		fChannelPhaParameter.twwdt[ch]      = settings->GetValue(Form("Board.%d.Channel.%d.RiseTimeValidationWindow", boardNumber, ch), 0);
		fChannelPhaParameter.dgain[ch]      = settings->GetValue(Form("Board.%d.Channel.%d.DigitalGain", boardNumber, ch), 0);
		fChannelPhaParameter.enf[ch]        = settings->GetValue(Form("Board.%d.Channel.%d.EnergyNormalization", boardNumber, ch), 1.0);
		fChannelPhaParameter.decimation[ch] = settings->GetValue(Form("Board.%d.Channel.%d.Decimation", boardNumber, ch), 0);
	}
}
#endif

void BoardSettings::ReadCustomSettings(const HNDLE& hDb, const HNDLE& hKey)
{
	// loop over sub-keys
	HNDLE hSubKey;
	KEY key;
	int size;
	for(int i = 0; ; ++i) {
		db_enum_key(hDb, hKey, i, &hSubKey);
		if(!hSubKey) break; // end of list reached
		db_get_key(hDb, hSubKey, &key);
		if(strcmp(key.name, "Link Type") == 0 && key.num_values == 1) {
			size = sizeof(fLinkType);
			db_get_data(hDb, hSubKey, &fLinkType, &size, TID_WORD);
		} else if(strcmp(key.name, "Board Type") == 0 && key.num_values == 1) {
			size = sizeof(fBoardType);
			db_get_data(hDb, hSubKey, &fBoardType, &size, TID_WORD);
		} else if(strcmp(key.name, "VME base address") == 0 && key.num_values == 1) {
			size = sizeof(fVmeBaseAddress);
			db_get_data(hDb, hSubKey, &fVmeBaseAddress, &size, TID_WORD);
		} else if(strcmp(key.name, "Port number") == 0 && key.num_values == 1) {
			size = sizeof(fPortNumber);
			db_get_data(hDb, hSubKey, &fPortNumber, &size, TID_WORD);
		} else if(strcmp(key.name, "Device number") == 0 && key.num_values == 1) {
			size = sizeof(fDeviceNumber);
			db_get_data(hDb, hSubKey, &fDeviceNumber, &size, TID_WORD);
		} else if(strcmp(key.name, "Acquisition Mode") == 0 && key.num_values == 1) {
			size = sizeof(fAcquisitionMode);
			db_get_data(hDb, hSubKey, &fAcquisitionMode, &size, TID_WORD);
		} else if(strcmp(key.name, "IO Level") == 0 && key.num_values == 1) {
			size = sizeof(fIOLevel);
			db_get_data(hDb, hSubKey, &fIOLevel, &size, TID_WORD);
		} else if(strcmp(key.name, "Trigger Mode") == 0 && key.num_values == 1) {
			size = sizeof(fTriggerMode);
			db_get_data(hDb, hSubKey, &fTriggerMode, &size, TID_WORD);
		} else if(strcmp(key.name, "Channel Mask") == 0 && key.num_values == 1) {
			size = sizeof(fChannelMask);
			db_get_data(hDb, hSubKey, &fChannelMask, &size, TID_WORD);
		} else if(strcmp(key.name, "RunSync mode") == 0 && key.num_values == 1) {
			size = sizeof(fRunSync);
			db_get_data(hDb, hSubKey, &fRunSync, &size, TID_WORD);
		} else if(strcmp(key.name, "Event aggregation") == 0 && key.num_values == 1) {
			size = sizeof(fEventAggregation);
			db_get_data(hDb, hSubKey, &fEventAggregation, &size, TID_WORD);
		} else if(strcmp(key.name, "Pile up rejection mode") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.purh);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.purh, &size, TID_WORD);
		} else if(strcmp(key.name, "Pile up gap") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.purgap);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.purgap, &size, TID_WORD);
		} else if(strncmp(key.name, "Channel ", 8) != 0) {
			std::cout<<"unrecognized custom entry "<<key.name<<std::endl;
		}
	}
}

void BoardSettings::ReadCustomChannelSettings(const int& channel, const HNDLE& hDb, const HNDLE& hKey)
{
	fChannelSettings.at(channel).ReadCustomSettings(hDb, hKey);
	// loop over sub-keys
	HNDLE hSubKey;
	KEY key;
	int size;
	for(int i = 0; ; ++i) {
		db_enum_key(hDb, hKey, i, &hSubKey);
		if(!hSubKey) break; // end of list reached
		db_get_key(hDb, hSubKey, &key);
		if(strcmp(key.name, "Threshold") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.thr[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.thr[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Baseline samples") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.nsbl[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.nsbl[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Long gate") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.lgate[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.lgate[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Short gate") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.sgate[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.sgate[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Pre gate") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.pgate[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.pgate[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Self trigger") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.selft[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.selft[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Trigger validation window") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.tvaw[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.tvaw[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Charge sensitivity") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.csens[channel]);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.csens[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Trigger holdoff") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPsdParameter.trgho);
			db_get_data(hDb, hSubKey, &fChannelPsdParameter.trgho, &size, TID_WORD);
		// PHA channel parameters
		} else if(strcmp(key.name, "Threshold") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.thr[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.thr[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Baseline samples") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.nsbl[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.nsbl[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Trigger holdoff") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.trgho[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.trgho[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Trigger validation window") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.trgwin[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.trgwin[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Trapezoid decay time") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.M[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.M[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Trapezoid flat top") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.m[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.m[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Trapezoid rise time") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.k[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.k[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Peaking time") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.ftd[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.ftd[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Smoothing factor") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.a[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.a[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Input rise time") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.b[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.b[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Peak samples") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.nspk[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.nspk[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Peak holdoff") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.pkho[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.pkho[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Baseline holdoff") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.blho[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.blho[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Rise time validation window") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.twwdt[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.twwdt[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Digital gain") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.dgain[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.dgain[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Energy normalization") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.enf[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.enf[channel], &size, TID_WORD);
		} else if(strcmp(key.name, "Decimation") == 0 && key.num_values == 1) {
			size = sizeof(fChannelPhaParameter.decimation[channel]);
			db_get_data(hDb, hSubKey, &fChannelPhaParameter.decimation[channel], &size, TID_WORD);
		} else {
			// we keep both channel and channelparameter (which are "per board") settings
			// in the "Channel x" directory, so there might be unrecognized entries
		}
	}
}

BoardSettings::~BoardSettings()
{
}

void BoardSettings::Print(const BoardSettings& templateSettings)
{
	if(fLinkType != templateSettings.LinkType()) {
		std::cout<<"  link type "<<fLinkType<<" = ";
		switch(fLinkType) {
			case CAEN_DGTZ_USB:
				std::cout<<"USB"<<std::endl;
				break;
			case CAEN_DGTZ_OpticalLink:
				std::cout<<"Optical Link"<<std::endl;
				break;
			default:
				std::cout<<"unknown"<<std::endl;
				break;
		}
	}
	if(fBoardType != templateSettings.BoardType()) {
		std::cout<<"  board type "<<static_cast<int>(fBoardType)<<" = ";
		switch(fBoardType) {
			case EBoardType::kDesktop:
				std::cout<<"Desktop"<<std::endl;
				break;
			case EBoardType::kNIM:
				std::cout<<"NIM"<<std::endl;
				break;
			case EBoardType::kVME:
				std::cout<<"VME"<<std::endl;
				break;
			default:
				std::cout<<"unknown"<<std::endl;
				break;
		}
	}
	if(fVmeBaseAddress != templateSettings.VmeBaseAddress()) {
		std::cout<<"   VME base address 0x"<<std::hex<<fVmeBaseAddress<<std::dec<<std::endl;
	}
	// always print port number and device number (these HAVE to be different for all boards)
	std::cout<<"   Port number "<<fPortNumber<<std::endl;
	std::cout<<"   Device number "<<fDeviceNumber<<std::endl;

	if(fAcquisitionMode != templateSettings.AcquisitionMode()) {
		std::cout<<"   acquisition mode "<<fAcquisitionMode<<" = ";
		switch(fAcquisitionMode) {
			case CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope:
				std::cout<<"oscilloscope"<<std::endl;
				break;
			case CAEN_DGTZ_DPP_ACQ_MODE_List:
				std::cout<<"list mode"<<std::endl;
				break;
			case CAEN_DGTZ_DPP_ACQ_MODE_Mixed:
				std::cout<<"mixed"<<std::endl;
				break;
				//case CAEN_DGTZ_SW_CONTROLLED:
				//	std::cout<<"software controlled"<<std::endl;
				//	break;
				//case CAEN_DGTZ_S_IN_CONTROLLED:
				//	std::cout<<"external signal controlled"<<std::endl;
				//	break;
				//case CAEN_DGTZ_FIRST_TRG_CONTROLLED:
				//	std::cout<<"first trigger controlled"<<std::endl;
				//	break;
			default:
				std::cout<<"unknown"<<std::endl;
				break;
		}
	}
	if(fIOLevel != templateSettings.IOLevel()) {
		std::cout<<"   IO level "<<fIOLevel<<" = ";
		switch(fIOLevel) {
			case CAEN_DGTZ_IOLevel_NIM:
				std::cout<<"NIM"<<std::endl;
				break;
			case CAEN_DGTZ_IOLevel_TTL:
				std::cout<<"TTL"<<std::endl;
				break;
			default:
				std::cout<<"unknown"<<std::endl;
				break;
		}
	}
	if(fChannelMask != templateSettings.ChannelMask()) {
		std::cout<<"   channel mask 0x"<<std::hex<<fChannelMask<<std::dec<<std::endl;
	}
	if(fRunSync != templateSettings.RunSync()) {
		std::cout<<"   run sync "<<fRunSync<<" = ";
		switch(fRunSync) {
			case CAEN_DGTZ_RUN_SYNC_Disabled:
				std::cout<<"disabled"<<std::endl;
				break;
			case CAEN_DGTZ_RUN_SYNC_TrgOutTrgInDaisyChain:
				std::cout<<"trigger out/trigger in chain"<<std::endl;
				break;
			case CAEN_DGTZ_RUN_SYNC_TrgOutSinDaisyChain:
				std::cout<<"trigger out/s in chain"<<std::endl;
				break;
			case CAEN_DGTZ_RUN_SYNC_SinFanout:
				std::cout<<"s in fanout"<<std::endl;
				break;
			case CAEN_DGTZ_RUN_SYNC_GpioGpioDaisyChain:
				std::cout<<"gpio chain"<<std::endl;
				break;
			default:
				std::cout<<"unknown"<<std::endl;
				break;
		}
	}
	if(fEventAggregation != templateSettings.EventAggregation()) {
		std::cout<<"   event aggregation "<<fEventAggregation<<std::endl;
	}
	if(fTriggerMode != templateSettings.TriggerMode()) {
		std::cout<<"   trigger mode "<<fTriggerMode<<" = ";
		switch(fTriggerMode) {
			case CAEN_DGTZ_TRGMODE_DISABLED:
				std::cout<<"disabled"<<std::endl;
				break;
			case CAEN_DGTZ_TRGMODE_EXTOUT_ONLY:
				std::cout<<"ext out only"<<std::endl;
				break;
			case CAEN_DGTZ_TRGMODE_ACQ_ONLY:
				std::cout<<"acq only"<<std::endl;
				break;
			case CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT:
				std::cout<<"acq and ext out"<<std::endl;
				break;
		}
	}

	if(fChannelPsdParameter.purh != templateSettings.ChannelPsdParameter()->purh) {
		std::cout<<"   pile-up rejection mode "<<fChannelPsdParameter.purh<<" = ";
		switch(fChannelPsdParameter.purh) {
			case CAEN_DGTZ_DPP_PSD_PUR_DetectOnly:
				std::cout<<"detection only"<<std::endl;
				break;
			case CAEN_DGTZ_DPP_PSD_PUR_Enabled:
				std::cout<<"enabled"<<std::endl;
				break;
			default:
				std::cout<<"unknown"<<std::endl;
				break;
		}
	}
	if(fChannelPsdParameter.purgap != templateSettings.ChannelPsdParameter()->purgap) {
		std::cout<<"   pile-up gap "<<fChannelPsdParameter.purgap<<std::endl;
	}
	if(fChannelPsdParameter.trgho != templateSettings.ChannelPsdParameter()->trgho) {
		std::cout<<"   trigger holdoff "<<fChannelPsdParameter.trgho<<std::endl;
	}
	for(int ch = 0; ch < static_cast<int>(fChannelSettings.size()); ++ch) {
		if(fChannelSettings[ch] == templateSettings.ChannelSettingsVector().at(0) &&
			fChannelPsdParameter == *(templateSettings.ChannelPsdParameter())) {
			continue;
		}
		std::cout<<"   Channel #"<<ch<<" custom settings:"<<std::endl;
		fChannelSettings[ch].Print(templateSettings.ChannelSettingsVector().at(0));
		if(fChannelPsdParameter.thr[ch] != templateSettings.ChannelPsdParameter()->thr[0]) {
			std::cout<<"      threshold "<<fChannelPsdParameter.thr[ch]<<std::endl;
		}
		if(fChannelPsdParameter.nsbl[ch] != templateSettings.ChannelPsdParameter()->nsbl[0]) {
			std::cout<<"      baseline samples "<<fChannelPsdParameter.nsbl[ch]<<std::endl;
		}
		if(fChannelPsdParameter.lgate[ch] != templateSettings.ChannelPsdParameter()->lgate[0]) {
			std::cout<<"      long gate "<<fChannelPsdParameter.lgate[ch]<<std::endl;
		}
		if(fChannelPsdParameter.sgate[ch] != templateSettings.ChannelPsdParameter()->sgate[0]) {
			std::cout<<"      short gate "<<fChannelPsdParameter.sgate[ch]<<std::endl;
		}
		if(fChannelPsdParameter.pgate[ch] != templateSettings.ChannelPsdParameter()->pgate[0]) {
			std::cout<<"      pre-gate "<<fChannelPsdParameter.pgate[ch]<<std::endl;
		}
		if(fChannelPsdParameter.selft[ch] != templateSettings.ChannelPsdParameter()->selft[0]) {
			std::cout<<"      self trigger "<<fChannelPsdParameter.selft[ch]<<std::endl;
		}
		if(fChannelPsdParameter.tvaw[ch] != templateSettings.ChannelPsdParameter()->tvaw[0]) {
			std::cout<<"      trigger val. window "<<fChannelPsdParameter.tvaw[ch]<<std::endl;
		}
		if(fChannelPsdParameter.csens[ch] != templateSettings.ChannelPsdParameter()->csens[0]) {
			std::cout<<"      charge sensitivity "<<fChannelPsdParameter.csens[ch]<<std::endl;
		}

		if(fChannelPhaParameter.thr[ch]        != templateSettings.ChannelPhaParameter()->thr[0]) {
			std::cout<<"      threshold "<<fChannelPhaParameter.thr[ch]<<std::endl;
		}
		if(fChannelPhaParameter.trgho[ch] != templateSettings.ChannelPhaParameter()->trgho[ch]) {
			std::cout<<"   trigger holdoff "<<fChannelPhaParameter.trgho[ch]<<std::endl;
		}
		if(fChannelPhaParameter.nsbl[ch]       != templateSettings.ChannelPhaParameter()->nsbl[0]) {
			std::cout<<"      baseline samples "<<fChannelPhaParameter.nsbl[ch]<<std::endl;
		}
		if(fChannelPhaParameter.trgwin[ch]     != templateSettings.ChannelPhaParameter()->trgwin[0]) {
			std::cout<<"      trigger window "<<fChannelPhaParameter.trgwin[ch]<<std::endl;
		}
		if(fChannelPhaParameter.M[ch]          != templateSettings.ChannelPhaParameter()->M[0]) {
			std::cout<<"      trapezoid decay time "<<fChannelPhaParameter.M[ch]<<std::endl;
		}
		if(fChannelPhaParameter.m[ch]          != templateSettings.ChannelPhaParameter()->m[0]) {
			std::cout<<"      trapezoid flat top "<<fChannelPhaParameter.m[ch]<<std::endl;
		}
		if(fChannelPhaParameter.k[ch]          != templateSettings.ChannelPhaParameter()->k[0]) {
			std::cout<<"      trapezoid rise time "<<fChannelPhaParameter.k[ch]<<std::endl;
		}
		if(fChannelPhaParameter.ftd[ch]        != templateSettings.ChannelPhaParameter()->ftd[0]) {
			std::cout<<"      peaking time "<<fChannelPhaParameter.ftd[ch]<<std::endl;
		}
		if(fChannelPhaParameter.a[ch]          != templateSettings.ChannelPhaParameter()->a[0]) {
			std::cout<<"      smoothing factor "<<fChannelPhaParameter.a[ch]<<std::endl;
		}
		if(fChannelPhaParameter.b[ch]          != templateSettings.ChannelPhaParameter()->b[0]) {
			std::cout<<"      input rise time "<<fChannelPhaParameter.b[ch]<<std::endl;
		}
		if(fChannelPhaParameter.nspk[ch]       != templateSettings.ChannelPhaParameter()->nspk[0]) {
			std::cout<<"      peak samples "<<fChannelPhaParameter.nspk[ch]<<std::endl;
		}
		if(fChannelPhaParameter.pkho[ch]       != templateSettings.ChannelPhaParameter()->pkho[0]) {
			std::cout<<"      peak holdoff "<<fChannelPhaParameter.pkho[ch]<<std::endl;
		}
		if(fChannelPhaParameter.blho[ch]       != templateSettings.ChannelPhaParameter()->blho[0]) {
			std::cout<<"      baseline holdoff "<<fChannelPhaParameter.blho[ch]<<std::endl;
		}
		if(fChannelPhaParameter.twwdt[ch]      != templateSettings.ChannelPhaParameter()->twwdt[0]) {
			std::cout<<"      trigger validation window "<<fChannelPhaParameter.twwdt[ch]<<std::endl;
		}
		if(fChannelPhaParameter.dgain[ch]      != templateSettings.ChannelPhaParameter()->dgain[0]) {
			std::cout<<"      digitakl gain "<<fChannelPhaParameter.dgain[ch]<<std::endl;
		}
		if(fChannelPhaParameter.enf[ch]        != templateSettings.ChannelPhaParameter()->enf[0]) {
			std::cout<<"      energy normalization "<<fChannelPhaParameter.enf[ch]<<std::endl;
		}
		if(fChannelPhaParameter.decimation[ch] != templateSettings.ChannelPhaParameter()->decimation[0]) {
			std::cout<<"      decimation "<<fChannelPhaParameter.decimation[ch]<<std::endl;
		}
	}
}

void BoardSettings::Print()
{
	std::cout<<"  link type "<<fLinkType<<" = ";
	switch(fLinkType) {
		case CAEN_DGTZ_USB:
			std::cout<<"USB"<<std::endl;
			break;
		case CAEN_DGTZ_OpticalLink:
			std::cout<<"Optical Link"<<std::endl;
			break;
		default:
			std::cout<<"unknown"<<std::endl;
			break;
	}
	std::cout<<"  board type "<<static_cast<int>(fBoardType)<<" = ";
	switch(fBoardType) {
		case EBoardType::kDesktop:
			std::cout<<"Desktop"<<std::endl;
			break;
		case EBoardType::kNIM:
			std::cout<<"NIM"<<std::endl;
			break;
		case EBoardType::kVME:
			std::cout<<"VME"<<std::endl;
			break;
		default:
			std::cout<<"unknown"<<std::endl;
			break;
	}
	std::cout<<"   VME base address 0x"<<std::hex<<fVmeBaseAddress<<std::dec<<std::endl;
	std::cout<<"   Port number "<<fPortNumber<<std::endl;
	std::cout<<"   Device number "<<fDeviceNumber<<std::endl;
	std::cout<<"   acquisition mode "<<fAcquisitionMode<<" = ";
	switch(fAcquisitionMode) {
		case CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope:
			std::cout<<"oscilloscope"<<std::endl;
			break;
		case CAEN_DGTZ_DPP_ACQ_MODE_List:
			std::cout<<"list mode"<<std::endl;
			break;
		case CAEN_DGTZ_DPP_ACQ_MODE_Mixed:
			std::cout<<"mixed"<<std::endl;
			break;
			//case CAEN_DGTZ_SW_CONTROLLED:
			//	std::cout<<"software controlled"<<std::endl;
			//	break;
			//case CAEN_DGTZ_S_IN_CONTROLLED:
			//	std::cout<<"external signal controlled"<<std::endl;
			//	break;
			//case CAEN_DGTZ_FIRST_TRG_CONTROLLED:
			//	std::cout<<"first trigger controlled"<<std::endl;
			//	break;
		default:
			std::cout<<"unknown"<<std::endl;
			break;
	}
	std::cout<<"   IO level "<<fIOLevel<<" = ";
	switch(fIOLevel) {
		case CAEN_DGTZ_IOLevel_NIM:
			std::cout<<"NIM"<<std::endl;
			break;
		case CAEN_DGTZ_IOLevel_TTL:
			std::cout<<"TTL"<<std::endl;
			break;
		default:
			std::cout<<"unknown"<<std::endl;
			break;
	}
	std::cout<<"   channel mask 0x"<<std::hex<<fChannelMask<<std::dec<<std::endl;
	std::cout<<"   run sync "<<fRunSync<<" = ";
	switch(fRunSync) {
		case CAEN_DGTZ_RUN_SYNC_Disabled:
			std::cout<<"disabled"<<std::endl;
			break;
		case CAEN_DGTZ_RUN_SYNC_TrgOutTrgInDaisyChain:
			std::cout<<"trigger out/trigger in chain"<<std::endl;
			break;
		case CAEN_DGTZ_RUN_SYNC_TrgOutSinDaisyChain:
			std::cout<<"trigger out/s in chain"<<std::endl;
			break;
		case CAEN_DGTZ_RUN_SYNC_SinFanout:
			std::cout<<"s in fanout"<<std::endl;
			break;
		case CAEN_DGTZ_RUN_SYNC_GpioGpioDaisyChain:
			std::cout<<"gpio chain"<<std::endl;
			break;
		default:
			std::cout<<"unknown"<<std::endl;
			break;
	}
	std::cout<<"   event aggregation "<<fEventAggregation<<std::endl;
	std::cout<<"   trigger mode "<<fTriggerMode<<" = ";
	switch(fTriggerMode) {
		case CAEN_DGTZ_TRGMODE_DISABLED:
			std::cout<<"disabled"<<std::endl;
			break;
		case CAEN_DGTZ_TRGMODE_EXTOUT_ONLY:
			std::cout<<"ext out only"<<std::endl;
			break;
		case CAEN_DGTZ_TRGMODE_ACQ_ONLY:
			std::cout<<"acq only"<<std::endl;
			break;
		case CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT:
			std::cout<<"acq and ext out"<<std::endl;
			break;
	}

	std::cout<<"   pile-up rejection mode "<<fChannelPsdParameter.purh<<" = ";
	switch(fChannelPsdParameter.purh) {
		case CAEN_DGTZ_DPP_PSD_PUR_DetectOnly:
			std::cout<<"detection only"<<std::endl;
			break;
		case CAEN_DGTZ_DPP_PSD_PUR_Enabled:
			std::cout<<"enabled"<<std::endl;
			break;
		default:
			std::cout<<"unknown"<<std::endl;
			break;
	}
	std::cout<<"   pile-up gap "<<fChannelPsdParameter.purgap<<std::endl;
	std::cout<<"   trigger holdoff "<<fChannelPsdParameter.trgho<<std::endl;
	for(int ch = 0; ch < static_cast<int>(fChannelSettings.size()); ++ch) {
		std::cout<<"   Channel #"<<ch<<":"<<std::endl;
		fChannelSettings[ch].Print();
		std::cout<<"      threshold "<<fChannelPsdParameter.thr[ch]<<std::endl;
		std::cout<<"      baseline samples "<<fChannelPsdParameter.nsbl[ch]<<std::endl;
		std::cout<<"      long gate "<<fChannelPsdParameter.lgate[ch]<<std::endl;
		std::cout<<"      short gate "<<fChannelPsdParameter.sgate[ch]<<std::endl;
		std::cout<<"      pre-gate "<<fChannelPsdParameter.pgate[ch]<<std::endl;
		std::cout<<"      self trigger "<<fChannelPsdParameter.selft[ch]<<std::endl;
		std::cout<<"      trigger val. window "<<fChannelPsdParameter.tvaw[ch]<<std::endl;
		std::cout<<"      charge sensitivity "<<fChannelPsdParameter.csens[ch]<<std::endl;

		std::cout<<"      threshold "<<fChannelPhaParameter.thr[ch]<<std::endl;
		std::cout<<"      baseline samples "<<fChannelPhaParameter.nsbl[ch]<<std::endl;
		std::cout<<"      trigger holdoff "<<fChannelPhaParameter.trgho[ch]<<std::endl;
		std::cout<<"      trigger window "<<fChannelPhaParameter.trgwin[ch]<<std::endl;
		std::cout<<"      trapezoid decay time "<<fChannelPhaParameter.M[ch]<<std::endl;
		std::cout<<"      trapezoid flat top "<<fChannelPhaParameter.m[ch]<<std::endl;
		std::cout<<"      trapezoid rise time "<<fChannelPhaParameter.k[ch]<<std::endl;
		std::cout<<"      peaking time "<<fChannelPhaParameter.ftd[ch]<<std::endl;
		std::cout<<"      smoothing factor "<<fChannelPhaParameter.a[ch]<<std::endl;
		std::cout<<"      input rise time "<<fChannelPhaParameter.b[ch]<<std::endl;
		std::cout<<"      peak samples "<<fChannelPhaParameter.nspk[ch]<<std::endl;
		std::cout<<"      peak holdoff "<<fChannelPhaParameter.pkho[ch]<<std::endl;
		std::cout<<"      baseline holdoff "<<fChannelPhaParameter.blho[ch]<<std::endl;
		std::cout<<"      trigger validation window "<<fChannelPhaParameter.twwdt[ch]<<std::endl;
		std::cout<<"      digital gain "<<fChannelPhaParameter.dgain[ch]<<std::endl;
		std::cout<<"      energy normalization "<<fChannelPhaParameter.enf[ch]<<std::endl;
		std::cout<<"      decimation "<<fChannelPhaParameter.decimation[ch]<<std::endl;
	}
}

bool operator==(const BoardSettings& lh, const BoardSettings& rh)
{
	if(lh.fChannelSettings.size() != rh.fChannelSettings.size()) {
		return false;
	}
	for(size_t ch = 0; ch < lh.fChannelSettings.size(); ++ch) {
		if(lh.fChannelSettings[ch] != rh.fChannelSettings[ch]) {
			return false;
		}
	}

	return (lh.fLinkType            == rh.fLinkType &&
			  lh.fBoardType           == rh.fBoardType &&
			  lh.fVmeBaseAddress      == rh.fVmeBaseAddress &&
			  lh.fPortNumber          == rh.fPortNumber &&
			  lh.fDeviceNumber        == rh.fDeviceNumber &&
			  lh.fAcquisitionMode     == rh.fAcquisitionMode &&
			  lh.fIOLevel             == rh.fIOLevel &&
			  lh.fChannelMask         == rh.fChannelMask &&
			  lh.fRunSync             == rh.fRunSync &&
			  lh.fEventAggregation    == rh.fEventAggregation &&
			  lh.fTriggerMode         == rh.fTriggerMode &&
			  lh.fChannelPsdParameter == rh.fChannelPsdParameter &&
			  lh.fChannelPhaParameter == rh.fChannelPhaParameter);
}

bool operator!=(const BoardSettings& lh, const BoardSettings& rh)
{
	return !(lh == rh);
}

CaenSettings::CaenSettings(bool debug)
{
	fNumberOfBoards = 0;
 	fNumberOfChannels = 0;
	fUseExternalClock = false;

	fBufferSize = 0;

	fRawOutput = false;

	fDebug = debug;
}

CaenSettings::~CaenSettings()
{
}

bool CaenSettings::ReadOdb(HNDLE hDB)
{
	if(hDB == 0) {
		std::cerr<<"Can't read settings from ODB, handle provided is "<<hDB<<std::endl;
		return false;
	}
	// read template from ODB
	HNDLE hSet;
	V1730_TEMPLATE templateSettings;
	V1730_TEMPLATE_STR(v1730TemplateStr);
	if(db_create_record(hDB, 0, "/DAQ/params/VX1730/template", strcomb(v1730TemplateStr)) != DB_SUCCESS) {
		std::cout<<"failed to create record"<<std::endl;
		throw;
	}

	db_find_key(hDB, 0, "/DAQ/params/VX1730/template", &hSet);
	int size = sizeof(templateSettings);
	if(db_get_record(hDB, hSet, &templateSettings, &size, 0) != DB_SUCCESS) {
		std::cout<<"Error occured trying to read \"/DAQ/params/VX1730/template\""<<std::endl;
		throw;
	}

	fNumberOfBoards = templateSettings.number_of_digitizers;
	if(fNumberOfBoards < 1) {
		std::cout<<fNumberOfBoards<<" boards is not possible!"<<std::endl;
		throw;
	}
	fNumberOfChannels = templateSettings.channels_per_digitizer;
	if(fNumberOfChannels < 1) {
		std::cout<<fNumberOfChannels<<" maximum channels is not possible!"<<std::endl;
		throw;
	}
	fUseExternalClock = templateSettings.use_external_clock;
	fRawOutput = templateSettings.raw_output;
	fBufferSize = 100000;

	if(fDebug) {
		// print template settings
		std::cout<<"template settings:"<<std::endl
			<<"number_of_digitizer "<<templateSettings.number_of_digitizers<<std::endl
			<<"channels_per_digitizer "<<templateSettings.channels_per_digitizer<<std::endl
			<<"raw_output "<<templateSettings.raw_output<<std::endl
			<<"link_type "<<templateSettings.link_type<<std::endl
			<<"vme_base_address "<<templateSettings.vme_base_address<<std::endl
			<<"acquisition_mode "<<templateSettings.acquisition_mode<<std::endl
			<<"io_level "<<templateSettings.io_level<<std::endl
			<<"trigger_mode "<<templateSettings.trigger_mode<<std::endl
			<<"channel_mask "<<templateSettings.channel_mask<<std::endl
			<<"runsync_mode "<<templateSettings.runsync_mode<<std::endl
			<<"event_aggregation "<<templateSettings.event_aggregation<<std::endl
			<<"record_length "<<templateSettings.record_length<<std::endl
			<<"dc_offset "<<templateSettings.dc_offset<<std::endl
			<<"pre_trigger "<<templateSettings.pre_trigger<<std::endl
			<<"pulse_polarity "<<templateSettings.pulse_polarity<<std::endl
			<<"enable_cfd "<<templateSettings.enable_cfd<<std::endl
			<<"cfd_delay "<<templateSettings.cfd_delay<<std::endl
			<<"cfd_fraction "<<templateSettings.cfd_fraction<<std::endl
			<<"cfd_interpolation_points "<<templateSettings.cfd_interpolation_points<<std::endl
			<<"enable_coinc "<<templateSettings.enable_coinc<<std::endl
			<<"enable_baseline "<<templateSettings.enable_baseline<<std::endl
			<<"coinc_window "<<templateSettings.coinc_window<<std::endl
			<<"coinc_latency "<<templateSettings.coinc_latency<<std::endl
			<<"input range "<<templateSettings.input_range<<std::endl
			<<"enable zero suppression "<<templateSettings.enable_zs<<std::endl
			<<"charge threshold "<<templateSettings.charge_threshold<<std::endl
			<<"pile_up_rejection_mode "<<templateSettings.pile_up_rejection_mode<<std::endl
			<<"pile_up_gap "<<templateSettings.pile_up_gap<<std::endl
			<<"trigger_holdoff "<<templateSettings.trigger_holdoff<<std::endl
			<<"threshold "<<templateSettings.threshold<<std::endl
			<<"baseline_samples "<<templateSettings.baseline_samples<<std::endl
			<<std::endl;
	}
	fTemplateSettings = BoardSettings(1, templateSettings);

	fBoardSettings.resize(fNumberOfBoards);
	for(int i = 0; i < fNumberOfBoards; ++i) {
		fBoardSettings[i] = BoardSettings(fNumberOfChannels, templateSettings);
		fBoardSettings[i].PortNumber(i); //this might get overwritten by custom settings
	}

	// search for custom information
	HNDLE hBoard;
	HNDLE hChannel;
	std::ostringstream oss;
	if(db_find_key(hDB, 0, "/DAQ/params/VX1730/custom", &hSet) == DB_SUCCESS) {
		for(int board = 0; board < fNumberOfBoards; ++board) {
			oss.str("");
			oss.clear();
			oss<<"Board "<<board;
			if(db_find_key(hDB, hSet, oss.str().c_str(), &hBoard) == DB_SUCCESS) {
				// found a set of custom settings for this board so we pass them along
				fBoardSettings[board].ReadCustomSettings(hDB, hBoard);
				for(int channel = 0; channel < fNumberOfChannels; ++channel) {
					oss.str("");
					oss.clear();
					oss<<"Channel "<<channel;
					//std::cout<<oss.str().c_str()<<std::endl;
					if(db_find_key(hDB, hBoard, oss.str().c_str(), &hChannel) == DB_SUCCESS) {
						//std::cout<<"found"<<std::endl;
						// found a set of custom settings for this channel so we pass them along
						fBoardSettings[board].ReadCustomChannelSettings(channel, hDB, hChannel);
					}
				}
			}
		}
	} else {
		db_create_key(hDB, 0, "/DAQ/params/VX1730/custom", TID_KEY);
	}

	Print();

	return true;
}

#ifdef USE_TENV
bool CaenSettings::ReadSettingsFile(const std::string& filename)
{
	auto settings = new TEnv(filename.c_str());
	if(settings == NULL || settings->ReadFile(filename.c_str(), kEnvLocal) != 0) {
		return false;
	}

	fNumberOfBoards = settings->GetValue("NumberOfBoards", 1);
	if(fNumberOfBoards < 1) {
		std::cout<<fNumberOfBoards<<" boards is not possible!"<<std::endl;
		return false;
	}
	fNumberOfChannels = settings->GetValue("NumberOfChannels", 8);
	if(fNumberOfChannels < 1) {
		std::cout<<fNumberOfChannels<<" maximum channels is not possible!"<<std::endl;
		return false;
	}
	fUseExternalClock = settings->GetValue("UseExternalClocl", false);
	fBufferSize = settings->GetValue("BufferSize", 100000);

	fBoardSettings.resize(fNumberOfBoards);
	for(int i = 0; i < fNumberOfBoards; ++i) {
		fBoardSettings[i] = BoardSettings(i, fNumberOfChannels, settings);
	}

	return true;
}
#endif

bool CaenSettings::WriteOdb()
{
	// for some reason using the handle doesn't work, cm_get_experiment_database(&hDB, NULL) gives hDB = 0
	// which then fails when used
	// so instead we create a script which uses odbedit and run it

	// for some reason WriteOdb does not work, so we create a script 'writeSettings.sh' and run it
	std::ofstream script("writeSettings.sh", std::ios::out);

	script<<"#!/bin/bash"<<std::endl
		<<"#"<<std::endl
		<<"# script to update ODB settings, automatically created by WriteOdb"<<std::endl
		<<std::endl;

	// template
	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Number of digitizers\\\" "<<fNumberOfBoards<<"\""<<std::endl;
	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Channels per digitizer\\\" "<<fNumberOfChannels<<"\""<<std::endl;
	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Use external clock\\\" "<<fUseExternalClock<<"\""<<std::endl;
	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Raw output\\\" "<<fRawOutput<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Link Type\\\" "<<fLinkType<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/VME base address\\\" "<<fVmeBaseAddress<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Acquisition Mode\\\" "<<fAcquisitionMode<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/IO Level\\\" "<<fIOLevel<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Trigger Mode\\\" "<<fTriggerMode<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Channel Mask\\\" "<<fChannelMask<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/RunSync Mode\\\" "<<fRunSync<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Event aggregation\\\" "<<fEventAggregation<<"\""<<std::endl;

	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Record length\\\" "<<fRecordLength<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/DC offset\\\" "<<fDCOffset<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Pre trigger\\\" "<<fPreTrigger<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Pulse polarity\\\" "<<fPulsePolarity<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Enable CFD\\\" "<<fEnableCfd<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/CFD delay\\\" "<<(fCfdParameters & 0xff)<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/CFD fraction\\\" "<<((fCfdParameters>>8) & 0x3)<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/CFD interpolation points\\\" "<<((fCfdParameters>>10) & 0x3)<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Enable Coincidence\\\" "<<fEnableCoinc<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Enable Baseline\\\" "<<fEnableBaseline<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Coincidence Window\\\" "<<fCoincWindow <<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Coincidence Latency\\\" "<<fCoincLatency <<"\""<<std::endl;

	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Pile up rejection mode\\\" "<<fChannelPsdParameter.purh<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Pile up gap\\\" "<<fChannelPsdParameter.purgap<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Baseline threshold\\\" "<<fChannelPsdParameter.blthr<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Baseline timeout\\\" "<<fChannelPsdParameter.bltmo<<"\""<<std::endl;
	//script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Trigger holdoff\\\" "<<fChannelPsdParameter.trgho<<"\""<<std::endl;
	//for(int ch = 0; ch < fNumberOfChannels; ++ch) {
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Threshold["<<ch<<"]\\\" "<<fChannelPsdParameter.thr[ch]<<"\""<<std::endl;
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Baseline samples["<<ch<<"]\\\" "<<fChannelPsdParameter.nsbl[ch]<<"\""<<std::endl;
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Long gate["<<ch<<"]\\\" "<<fChannelPsdParameter.lgate[ch]<<"\""<<std::endl;
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Short gate["<<ch<<"]\\\" "<<fChannelPsdParameter.sgate[ch]<<"\""<<std::endl;
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Pre gate["<<ch<<"]\\\" "<<fChannelPsdParameter.pgate[ch]<<"\""<<std::endl;
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Self trigger["<<ch<<"]\\\" "<<fChannelPsdParameter.selft[ch]<<"\""<<std::endl;
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Trigger validation window["<<ch<<"]\\\" "<<fChannelPsdParameter.tvaw[ch]<<"\""<<std::endl;
	//	script<<"odbedit -c \"set \\\"/DAQ/params/VX1730/custom/Charge sensitivity["<<ch<<"]\\\" "<<fChannelPsdParameter.csens[ch]<<"\""<<std::endl;
	//}
	script<<std::endl;

	script.close();

	std::system("chmod +x writeSettings.sh; ./writeSettings.sh; rm writeSettings.sh");

	return true;

	// write ODB settings
	HNDLE hDB;
	HNDLE hSet;
	V1730_TEMPLATE settings;

	std::cout<<"here ..."<<std::endl;
	// copy settings to V1730_TEMPLATE struct
	if(fNumberOfBoards > MAX_BOARDS) {
		std::cerr<<"Too many boards specified in settings: "<<fNumberOfBoards<<" > "<<MAX_BOARDS<<std::endl;
		return false;
	}
	if(fNumberOfChannels*fNumberOfBoards > MAX_CHANNELS) {
		std::cerr<<"Too many channels specified in settings: "<<fNumberOfChannels*fNumberOfBoards<<" > "<<MAX_CHANNELS<<std::endl;
		return false;
	}
	settings.number_of_digitizers = fNumberOfBoards;
	settings.channels_per_digitizer = fNumberOfChannels;
	settings.use_external_clock = fUseExternalClock;
	//settings.link_type = fLinkType;
	//settings.vme_base_address = fVmeBaseAddress;
	//settings.acquisition_mode = fAcquisitionMode;
	//settings.io_level = fIOLevel;
	//settings.trigger_mode = fTriggerMode;
	//settings.channel_mask = fChannelMask;
	//settings.runsync_mode = fRunSync;
	//settings.event_aggregation = fEventAggregation;

	//settings.record_length = fRecordLength;
	//settings.dc_offset = fDCOffset;
	//settings.pre_trigger = fPreTrigger;
	//settings.pulse_polarity = fPulsePolarity;
	//settings.enable_cfd = fEnableCfd;
	//settings.cfd_delay = fCfdParameters & 0xff;
	//settings.cfd_fraction = (fCfdParameters>>8) & 0x3;
	//settings.cfd_interpolation_points = (fCfdParameters>>10) & 0x3;

	//settings.enable_coinc = fEnableCoinc;
	//settings.enable_baseline = fEnableBaseline;
	//settings.coinc_window = fCoincWindow;
	//settings.coinc_latency = fCoincLatency;

	//settings.pile_up_rejection_mode = fChannelPsdParameter.purh;
	//settings.pile_up_gap = fChannelPsdParameter.purgap;
	//settings.trigger_holdoff = fChannelPsdParameter.trgho;
	//for(int ch = 0; ch < fNumberOfChannels; ++ch) {
	//	settings.threshold[ch] = fChannelPsdParameter.thr[ch];
	//	settings.baseline_samples[ch] = fChannelPsdParameter.nsbl[ch];
	//	settings.long_gate[ch] = fChannelPsdParameter.lgate[ch];
	//	settings.short_gate[ch] = fChannelPsdParameter.sgate[ch];
	//	settings.pre_gate[ch] = fChannelPsdParameter.pgate[ch];
	//	settings.self_trigger[ch] = fChannelPsdParameter.selft[ch];
	//	settings.trigger_validation_window[ch] = fChannelPsdParameter.tvaw[ch];
	//	settings.charge_sensitivity[ch] = fChannelPsdParameter.csens[ch];
	//}
	settings.raw_output = fRawOutput;
	std::cout<<"connecting to database ..."<<std::endl;
	// connect to ODB
	if(cm_get_experiment_database(&hDB, NULL) != CM_SUCCESS) {
		std::cout<<"failed to get database handle "<<hDB<<std::endl;
		return false;
	}

	std::cout<<"creating record using handle "<<hDB<<" ..."<<std::endl;
	// create record (just in case it didn't exist already)
	V1730_TEMPLATE_STR(v1730TemplateStr);
	if(db_create_record(hDB, 0, "/DAQ/params/VX1730/template", strcomb(v1730TemplateStr)) != DB_SUCCESS) {
		std::cout<<"failed to create record"<<std::endl;
		return false;
	}

	std::cout<<"writing ..."<<std::endl;
	// write settings to ODB
	db_find_key(hDB, 0, "/DAQ/params/VX1730/template", &hSet);
	int size = sizeof(settings);
	if(db_set_record(hDB, hSet, &settings, size, 0) != DB_SUCCESS) {
		std::cout<<"Error occured trying to write \"/DAQ/params/VX1730/template\""<<std::endl;
		return false;
	}

	return true;
}

void CaenSettings::Print()
{
	std::cout<<(fUseExternalClock?"Using ":"Not using ")<<" external clock for "<<fNumberOfBoards<<" board(s) with "<<fNumberOfChannels<<" channels each:"<<std::endl;
	std::cout<<"template settings:"<<std::endl;
	fTemplateSettings.Print();
	for(size_t i = 0; i < fBoardSettings.size(); ++i) {
		std::cout<<"Board #"<<i<<" custom settings:"<<std::endl;
		fBoardSettings[i].Print(fTemplateSettings);
	}
}

