
#define MAX_GRIFC                    5
#define GRIFC_PORTS                 12

#define GRIF16_PAR      0x00FF // channel address of grif16 parameter block
#define GRIFC_SLAVEPAR  0x0FFF // channel address of slave-grifc parameter block
#define GRIFC_MSTPAR    0xFFFF // channel address of master parameter block
#define GRIFC_PPGPAR    0xFF7E // channel address of ppg parameter block
#define GRIFC_FILPAR    0xFF7F // channel address of filter parameter block

#define DEFAULT_VMEADDR 0x00100008 // default ppg vme address

// from param_io.v design file 
#define PARAM_NETSIZE        0x08
#define PARAM_CSR            0x3F // read/write whole register
#define PARAM_SSCSR          0x3E // selective set
#define PARAM_SCCSR          0x3D // selective clear
#define PPGPROG_OFFSET       0x80 // ppg program uses same numbers as filter
#define FILTPROG_OFFSET      0x80
#define PRESCALE_OFFSET      0xC0
#define PARAM_COINCWIN       0xC8 // filter coincwin param
#define PARAM_DETMASK        0xC9 // filter detector mask param

#define PPG_IDLEPAT             0x02
#define PPG_VMEADDR             0x0E

#define  CSR_RUN               0x100 /* CSR bit asignments ...*/
#define  CSR_SETMASTER         0x200
#define  CSR_ENABLE_FILTER     0x400
#define  CSR_PPG_EVENTS        0x800
#define  CSR_ENABLE_PPG        0x1000
#define  CSR_ENABLE_VME        0x2000
#define  CSR_PARAMRUN_ON       0x4000
#define  CSR_ESATARUN_OFF      0x8000
#define  CSR_ESATACLK_OFF      0x10000

#define  FILCSR_DISABLE_FILTER    0x100 /* Filter CSR bit asignments ...*/
#define  FILCSR_WRITE_FILTERED    0x200
#define  FILCSR_SKIP_UNFILTERED   0x400
#define  FILCSR_STRICT_FILTER     0x800
#define  FILCSR_MULTI_FILTCOUNT   0x1000

/* parameter Address format ... */

#define NUM_DIGITISER_TYPES       3

void grifc_FlushEvents   ( int collector_mask );
