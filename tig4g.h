#include <time.h> // for time_t type used below ...

#define TIG4G_BASE_ADDR       0xB00000
#define TIG4G_ADDR_SIZE        0x10000
#define MAX_TIG4G                   13
#define TIG4G_PORTS                 12

#define TIG4G_CSR_REG_RW        0x0000
#define TIG4G_CSR               0x0008
#define TIG4G_SSET_CSR          0x0010
#define TIG4G_SCLR_CSR          0x0018
#define TIG4G_TEST_REG_RW       0x0028
#define TIG4G_SSET_TESTREG      0x002c 
#define TIG4G_SCLR_TESTREG      0x0030
#define TIG4G_FIRMWARE_R        0x0020
#define TIG4G_GENERAL_RESET     0x0080
#define TIG4G_NFRAME_R          0x0050 // old:1e0 New:50
#define TIG4G_NFRAME_MASK       0xFFFF
#define TIG4G_PARAM_DATA_RW     0x0040
#define TIG4G_PARAM_ID_R        0x0060
#define TIG4G_DATA_FIFO_R       0x0200

#define  TIG4G_CSR_RUNNING          0x00000001 /* CSR bit asignments ...*/
#define  TIG4G_CSR_PARAM_READY      0x00000004
#define  TIG4G_CSR_EVT_EMPTY        0x00000000
#define  TIG4G_CSR_FE_FULL          0x00000000
#define  TIG4G_CSR_SELECT_64BIT     0x00000008

/* parameter Address format ...
      Bits 15..12: Tig4gPort
      Bits 11.. 8: Tig10 Channel
      Bits      7: Read/Write (1=>Read)
      Bits      6: ?
      Bits  5.. 0: Parameter Id
*/

/* Data Header definitions */
#define TIG4G_DATA_HEADER_MASK     0xF0000000
#define TIG4G_DATA_TRAILER         0xE0000000
#define TIG4G_DATA_CHANNEL         0xC0000000
#define TIG4G_DATA_TIME_STAMP      0xA0000000
#define TIG4G_DATA_HEADER          0x80000000

#define TIG4G_DATA_DATA            0x00000000
#define TIG4G_DATA_FEATURE_TIME    0x40000000
#define TIG4G_DATA_FEATURE_CHARGE  0x50000000
#define TIG4G_DATA_FEATURE_MASK    0x70000000
#define TIG4G_DATA_TIMEOUT         0x10000000

#define TIG4G_DATA_OUT_OF_SYNC     0x88000000

int  tig4g_init          ( int * );
void tig4g_AcqStart      ( );
void tig4g_AcqStop       ( );
void tig4g_Reset         ( );
int  tig4g_vme_fifo_addr ( int tig4g_id );
int  tig4g_NFrameRead    ( int collector );
char *tig4g_firmware_date( time_t val );
void tig4g_set_64bit     ( int collector, int set_64bit_mode );
int  tig4g_ParamWrite    ( int col,int port,int chan, int ID, int val );
int  tig4g_ParamRead     ( int col,int port,int chan, int ID );
void tig4g_FlushEvents   ( int collector_mask );
void tig4g_SetTrigBox    ( int tig4g_id, int boxnum );
void tig4g_SetTrigPar    ( int tig4g_id, int par_addr, int value );
int tig4g_ReadTrigPar    ( int tig4g_id, int box, int par_addr );
int tig4g_ReadStatus     ( int tig4g_id, int addr );
int tig4g_ReadFirmwareId ( int tig4g_id );
void tig4g_ParamWriteNoCheck(int tig4g_id, int port, int chan, int parmID, int parm);
void tig4g_set_64bit_transfer(int tig4g_id, int value);

int tig_ParamReadStatus ();

void tig4g_set_trigger(int tig_id, int master_trigger_mode, int disabled_port_mask, int prescale_factor );

//int tig4g_readevent ( char *pevent, int collector, int frontend_event_assembly, int debug_interval, int suppress_error_messages, int usedma );

int tig4g_EventRead (int tig4g_id, int *ev_buffer, int *ev_bufpos);
void tig4g_EventRead64_init();
int tig4g_EventRead64(int tig4g_id, int *midas_buffer, int *midas_bufpos);
