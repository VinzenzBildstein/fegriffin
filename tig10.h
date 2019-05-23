#ifndef  __TIG10_INCLUDE_H__
#define  __TIG10_INCLUDE_H__
 
/* Parameter id ...
      1: PED                   X0000 (0)
      2: HitDetThresh          X0064 (100)  
      3: CFDDelay              X0005 (5)
      4: PRETRIG               X00E4 (228)
      5: SEGMENT_SIZE          X0100 (256)
      6: K (Integration time)  X0258 (600)
      7: L (Window size)       X02A8 (680)
      8: M (PZ constant)       X1388 (5000)
      9: Simulation            X0001
     10: Modebits              X3600 (11 0110 0000 0000, ge default)
     11: HitDetClipDelay       X001E (30)
     12: LATENCY               X008C (140)
     13: FIRMWAREID            X0311
     14: ATTENUATOR            X0258 (600)
     15: TriggerThreshold      X0064 (100) 

   Mode Parameter Bits ... -ve:2610 +ve:2018 +veLowGain:2818
     default: 0       0 : Enable simulation       
              0       1 : Suppress Raw Data
              0       2 : select deconvoluted raw data
              0       3 : Positive input
              ---------
             01     4-5 : Baseline Restore speed (1,8,64,256)
              0       6 : Freeze blr
              0       7 : Disable Trigger Request (was select clipped mode)
              ---------
              0       8 : Disable ADC
             00    9-10 : DC Offset (00:-veInput,11:+ve,01:central(bipolar)
              0      11 : Low Gain
              ---------
           0010   12-15 : Board Revision
*/

#define TIG10_MODEBITS2            1
#define TIG10_HIT_THRESHOLD        2
#define TIG10_CFD_DELAY            3
#define TIG10_PRE_TRIGGER          4
#define TIG10_SAMPLE_WINDOW_SIZE   5
#define TIG10_PARAM_K              6
#define TIG10_PARAM_L              7
#define TIG10_PARAM_M              8
#define TIG10_PULSER_BITS          9 // Using these to control the internal pulser - CU 30 July 2012
#define TIG10_MODE_BITS           10
#define TIG10_THRESHOLD_L         11
#define TIG10_LATENCY             12
#define TIG10_FIRMWARE_SCALARS    13
#define TIG10_THRESHOLD_K         14
#define TIG10_TRIG_THRESHOLD      15 
#define TIG10_INTEG_DELAY         16
#define TIG10_THRESHOLD_COUNT     17
#define TIG10_FIRMWAREID          18

#define  T10COL_CHAN               0xe // "channel#" for parameters in collectr

#define  T10COL_SETSTB              0
#define  T10COL_AlWAYSACTIVE       11
#define  T10COL_SERFLASH           13
#define  T10COL_FIRMWAREID_LO      14
#define  T10COL_FIRMWAREID_HI      15

char *tig10_firmware_date(time_t val);

#endif

