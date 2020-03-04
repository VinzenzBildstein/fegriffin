/********************************************************************\

  Name:         CaenOdb.h
  Created by:   ODBedit program

  Contents:     
                This file contains the "Settings" subtree for
                all items listed under "/Equipment" as well as their
                event definition.

                It can be used by the frontend and analyzer to work
                with these information.

                All C structures are accompanied with a string represen-
                tation which can be used in the db_create_record function
                to setup an ODB structure which matches the C structure.

  Created on:   Tue Jan  9 10:50:52 2018

\********************************************************************/
#ifndef CAENODB_H
#define CAENODB_H
#ifndef EXCL_CAEN

#define CAEN_SETTINGS_DEFINED

#define MAX_BOARDS 5
#define MAX_CHANNELS 16

typedef struct {
  char      format[80];
  WORD      number_of_digitizers;
  WORD      channels_per_digitizer;
  BOOL		use_external_clock;
  BOOL      raw_output;
  WORD      link_type;
  WORD		board_type;
  DWORD     vme_base_address;
  WORD      acquisition_mode;
  WORD      io_level;
  WORD      board_trigger_mode;
  WORD      channel_mask;
  WORD		runsync_mode;
  WORD		event_aggregation;
  BOOL		trigger_propagation;
  BOOL		dual_trace;
  WORD		analog_probe1;
  WORD		analog_probe2;
  WORD		digital_probe;
  // per channel parameters
  WORD		record_length;
  WORD		dc_offset;
  WORD		pre_trigger;
  WORD		pulse_polarity;
  BOOL		enable_cfd;
  WORD		cfd_delay;
  WORD		cfd_fraction;
  WORD		cfd_interpolation_points;
  BOOL      enable_coinc;
  BOOL      enable_baseline;
  WORD		coinc_window;
  WORD		coinc_latency;
  BOOL		input_range;
  BOOL		enable_zs;
  WORD		charge_threshold;
  WORD		trigger_width;
  WORD		trigger_mode;
  WORD		trigger_mask;
  WORD		coincidence_mode;

  // for channel PSD and PHA parameters structure
  WORD		threshold;
  WORD		baseline_samples;
  WORD      trigger_holdoff;
  WORD		trigger_validation_window;

  // for channel PSD parameters structure
  WORD      pile_up_rejection_mode;
  WORD      pile_up_gap;
  WORD		long_gate;
  WORD		short_gate;
  WORD		pre_gate;
  WORD		self_trigger;
  WORD		charge_sensitivity;

  // for channel PHA parameters structure
  WORD		trap_decay_time;
  WORD		trap_flat_top;
  WORD		trap_rise_time;
  WORD		peaking_time;
  WORD		smoothing_factor;
  WORD		input_rise_time;
  WORD		peak_samples;
  WORD		peak_holdoff;
  WORD		baseline_holdoff;
  WORD		rise_time_validation_window;
  WORD		digital_gain;
  WORD		energy_normalization;
  WORD		decimation;
} V1730_TEMPLATE;

#define V1730_TEMPLATE_STR(_name) const char *_name[] = {\
	"[.]",\
	"Format = STRING : [80] ",\
	"Number of digitizers = WORD : 1",\
	"Channels per digitizer = WORD : 8",\
	"Use external clock = BOOL : 0",\
	"Raw output = BOOL : 0",\
	"Link type = WORD : 1",\
	"Board type = WORD : 2",\
	"VME base address = DWORD : 0",\
	"Acquisition mode = WORD : 1",\
	"IO level = WORD : 0",\
	"Board trigger Mode = WORD : 1",\
	"Channel mask = WORD : 0xff",\
	"RunSync mode = WORD : 0",\
	"Event aggregation = WORD : 0",\
	"Trigger propagation = BOOL : 0",\
	"Dual trace = BOOL : 0",\
	"Analog probe 1 = WORD : 0",\
	"Analog probe 2 = WORD : 2",\
	"Digital probe = WORD : 0",\
	"Record length = WORD : 192",\
	"DC offset = WORD : 0x8000",\
	"Pre trigger = WORD : 80",\
	"Pulse polarity = WORD : 1",\
	"Enable CFD = BOOL : 1",\
	"CFD delay = WORD : 2",\
	"CFD fraction = WORD : 0",\
	"CFD interpolation points = WORD : 0",\
	"Enable baseline = BOOL : 0",\
	"Input range = BOOL : 0",\
	"Enable zero suppression = BOOL : 0",\
	"Charge threshold = WORD : 0",\
	"Trigger width = WORD : 50",\
	"Trigger mode = WORD : 0",\
	"Trigger mask = WORD : 0",\
	"Coincidence mode = WORD : 0",\
	"Threshold = WORD : 50",\
	"Baseline samples = WORD : 4",\
	"Trigger holdoff = WORD : 8",\
	"Trigger validation window = WORD : 50",\
	"Pile up rejection mode = WORD : 0",\
	"Pile up gap = WORD : 100",\
	"Long gate = WORD : 50",\
	"Short gate = WORD : 24",\
	"Pre gate = WORD : 8",\
	"Self trigger = WORD : 1",\
	"Charge sensitivity = WORD : 0",\
	"Trapezoid decay time = WORD : 50000",\
	"Trapezoid flat top = WORD : 1200",\
	"Trapezoid rise time = WORD : 6000",\
	"Peaking time = WORD : 1000",\
	"Smoothing factor = WORD : 0",\
	"Input rise time = WORD : 400",\
	"Peak samples = WORD : 0",\
	"Peak holdoff = WORD : 6000",\
	"Baseline holdoff = WORD : 6",\
	"Rise time validation window = WORD : 0",\
	"Digital gain = WORD : 0",\
	"Energy normalization = WORD : 0",\
	"Decimation = WORD : 0",\
	"",\
	NULL }

// DT5730 - old style
typedef struct {
  char      format[80];
  WORD      number_of_digitizers;
  WORD      channels_per_digitizer;
  BOOL		use_external_clock;
  BOOL      raw_output;
  WORD      link_type[MAX_BOARDS];
  WORD      board_type[MAX_BOARDS];
  DWORD     vme_base_address[MAX_BOARDS];
  WORD      acquisition_mode[MAX_BOARDS];
  WORD      io_level[MAX_BOARDS];
  WORD      trigger_mode[MAX_BOARDS];
  WORD      channel_mask[MAX_BOARDS];
  WORD		runsync_mode[MAX_BOARDS];
  WORD		event_aggregation[MAX_BOARDS];
  WORD		record_length[MAX_CHANNELS];
  WORD		dc_offset[MAX_CHANNELS];
  WORD		pre_trigger[MAX_CHANNELS];
  WORD		pulse_polarity[MAX_CHANNELS];
  WORD		enable_cfd[MAX_CHANNELS];
  WORD		cfd_delay[MAX_CHANNELS];
  WORD		cfd_fraction[MAX_CHANNELS];
  WORD		cfd_interpolation_points[MAX_CHANNELS];

  //channel parameters
  WORD      pile_up_rejection_mode[MAX_BOARDS];
  WORD      pile_up_gap[MAX_BOARDS];
  WORD      baseline_threshold[MAX_BOARDS];
  WORD      baseline_timeout[MAX_BOARDS];
  WORD      trigger_holdoff[MAX_BOARDS];
  WORD		threshold[MAX_CHANNELS];
  WORD		baseline_samples[MAX_CHANNELS];
  WORD		long_gate[MAX_CHANNELS];
  WORD		short_gate[MAX_CHANNELS];
  WORD		pre_gate[MAX_CHANNELS];
  WORD		self_trigger[MAX_CHANNELS];
  WORD		trigger_configuration[MAX_CHANNELS];
  WORD		trigger_validation_window[MAX_CHANNELS];
  WORD		charge_sensitivity[MAX_CHANNELS];
} DT5730_SETTINGS;

#define DT5730_SETTINGS_STR(_name) const char *_name[] = {\
	"[.]",\
	"Format = STRING : [80] ",\
	"Number of digitizers = WORD : 1",\
	"Channels per digitizer = WORD : 8",\
	"Use external clock = BOOL : 0",\
	"Raw output = BOOL : 0",\
	"Link Type = WORD[2] :",\
	"[0] 0",\
	"[1] 0",\
	"Board Type = WORD[2] :",\
	"[0] 0",\
	"[1] 0",\
	"VME base address = DWORD[2] :",\
	"[0] 0",\
	"[1] 0",\
	"Acquisition Mode = WORD[2] :",\
	"[0] 1",\
	"[1] 1",\
	"IO Level = WORD[2] :",\
	"[0] 0",\
	"[1] 0",\
	"Trigger Mode = WORD[2] :",\
	"[0] 1",\
	"[1] 1",\
	"Channel Mask = WORD[2] :",\
	"[0] 0xff",\
	"[1] 0xff",\
	"RunSync mode = WORD[2] :",\
	"[0] 0",\
	"[1] 0",\
	"Event aggregation = WORD[2] :",\
	"[0] 0",\
	"[1] 0",\
	"Record length = WORD[8] :",\
	"[0]  192",\
	"[1]  192",\
	"[2]  192",\
	"[3]  192",\
	"[4]  192",\
	"[5]  192",\
	"[6]  192",\
	"[7]  192",\
	"DC offset = WORD[8] :",\
	"[0]  0x8000",\
	"[1]  0x8000",\
	"[2]  0x8000",\
	"[3]  0x8000",\
	"[4]  0x8000",\
	"[5]  0x8000",\
	"[6]  0x8000",\
	"[7]  0x8000",\
	"Pre trigger = WORD[8] :",\
	"[0]  80",\
	"[1]  80",\
	"[2]  80",\
	"[3]  80",\
	"[4]  80",\
	"[5]  80",\
	"[6]  80",\
	"[7]  80",\
	"Pulse polarity = WORD[8] :",\
	"[0]  1",\
	"[1]  1",\
	"[2]  1",\
	"[3]  1",\
	"[4]  1",\
	"[5]  1",\
	"[6]  1",\
	"[7]  1",\
	"Enable CFD = WORD[8] :",\
	"[0]  1",\
	"[1]  1",\
	"[2]  1",\
	"[3]  1",\
	"[4]  1",\
	"[5]  1",\
	"[6]  1",\
	"[7]  1",\
	"CFD delay = WORD[8] :",\
	"[0]  2",\
	"[1]  2",\
	"[2]  2",\
	"[3]  2",\
	"[4]  2",\
	"[5]  2",\
	"[6]  2",\
	"[7]  2",\
	"CFD fraction = WORD[8] :",\
	"[0]  0",\
	"[1]  0",\
	"[2]  0",\
	"[3]  0",\
	"[4]  0",\
	"[5]  0",\
	"[6]  0",\
	"[7]  0",\
	"CFD interpolation points = WORD[8] :",\
	"[0]  0",\
	"[1]  0",\
	"[2]  0",\
	"[3]  0",\
	"[4]  0",\
	"[5]  0",\
	"[6]  0",\
	"[7]  0",\
	"Pile up rejection mode = WORD[2] :",\
	"[0] 0",\
	"[1] 0",\
	"Pile up gap = WORD[2] :",\
	"[0] 100",\
	"[1] 100",\
	"Baseline threshold = WORD[2] :",\
	"[0] 3",\
	"[1] 3",\
	"Baseline timeout = WORD[2] :",\
	"[0] 100",\
	"[1] 100",\
	"Trigger holdoff = WORD[2] :",\
	"[0] 8",\
	"[1] 8",\
	"Threshold = WORD[8] :",\
	"[0]  50",\
	"[1]  50",\
	"[2]  50",\
	"[3]  50",\
	"[4]  50",\
	"[5]  50",\
	"[6]  50",\
	"[7]  50",\
	"Baseline samples = WORD[8] :",\
	"[0]  4",\
	"[1]  4",\
	"[2]  4",\
	"[3]  4",\
	"[4]  4",\
	"[5]  4",\
	"[6]  4",\
	"[7]  4",\
	"Long gate = WORD[8] :",\
	"[0]  50",\
	"[1]  50",\
	"[2]  50",\
	"[3]  50",\
	"[4]  50",\
	"[5]  50",\
	"[6]  50",\
	"[7]  50",\
	"Short gate = WORD[8] :",\
	"[0]  24",\
	"[1]  24",\
	"[2]  24",\
	"[3]  24",\
	"[4]  24",\
	"[5]  24",\
	"[6]  24",\
	"[7]  24",\
	"Pre gate = WORD[8] :",\
	"[0]  8",\
	"[1]  8",\
	"[2]  8",\
	"[3]  8",\
	"[4]  8",\
	"[5]  8",\
	"[6]  8",\
	"[7]  8",\
	"Self trigger = WORD[8] :",\
	"[0]  1",\
	"[1]  1",\
	"[2]  1",\
	"[3]  1",\
	"[4]  1",\
	"[5]  1",\
	"[6]  1",\
	"[7]  1",\
	"Trigger configuration = WORD[8] :",\
	"[0]  1",\
	"[1]  1",\
	"[2]  1",\
	"[3]  1",\
	"[4]  1",\
	"[5]  1",\
	"[6]  1",\
	"[7]  1",\
	"Trigger validation window = WORD[8] :",\
	"[0]  50",\
	"[1]  50",\
	"[2]  50",\
	"[3]  50",\
	"[4]  50",\
	"[5]  50",\
	"[6]  50",\
	"[7]  50",\
	"Charge sensitivity = WORD[8] :",\
	"[0]  0",\
	"[1]  0",\
	"[2]  0",\
	"[3]  0",\
	"[4]  0",\
	"[5]  0",\
	"[6]  0",\
	"[7]  0",\
	"",\
	NULL }

#endif
#endif


