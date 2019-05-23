#define  TIG64_CHANMASK_HIGH        (WORD) (1)
#define  TIG64_HIT_THRESHOLD        (WORD) (2)  //
#define  TIG64_HITDET_CLIP_DELAY    (WORD) (3)  // Global   30
#define  TIG64_PRE_TRIGGER          (WORD) (4)  // Global   20
#define  TIG64_SAMPLE_WINDOW_SIZE   (WORD) (5)  // Global  100
#define  TIG64_PARAM_K              (WORD) (6)  // Global  400
#define  TIG64_PARAM_L              (WORD) (7)  // Global  600
#define  TIG64_PARAM_M              (WORD) (8)  // Delta  4096
#define  TIG64_CHANMASK             (WORD) (9)  //
#define  TIG64_MODE_BITS            (WORD) (10) // Global    0
#define  TIG64_MODE_BITS_2          (WORD) (11) // Global    0
#define  TIG64_LATENCY              (WORD) (12) // 0
#define  TIG64_FIRMWARE_ID          (WORD) (13)
#define  TIG64_THRESHOLD_L          (WORD) (14) // CU 24 Jan 13
#define  TIG64_TRIG_THRESHOLD       (WORD) (15) //
#define  TIG64_THRESHOLD_COUNT      (WORD) (16)
#define  TIG64_THRESHOLD_K          (WORD) (17)
#define  TIG64_INTEG_DELAY          (WORD) (18)
#define  TIG64_THRESHOLD_MASK       0x1FFF
#define  TIG64_UNUSED               (WORD) (0xFFFF)

#define  TIG64COL_CHAN          0
#define  TIG64COL_PARAM         (1<<6)

#define  TIG64COL_FIRMWAREID_LO (TIG64COL_PARAM + 13)
#define  TIG64COL_FIRMWAREID_HI (TIG64COL_PARAM + 14)
