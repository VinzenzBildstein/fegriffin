/* -------------- fegrifdev: for Grifc + attached digitisers ------------- */
/*    fegrifdev.c: midas frontend interface (init,start/stopRun,readEvent) */
/* odb_settings.c: functions to read and apply odb electronics settings    */

#include <stdio.h>
#include <netinet/in.h> /* sockaddr_in, IPPROTO_TCP, INADDR_ANY */
#include <netdb.h>      /* gethostbyname */
#include <stdlib.h>     /* exit() */
#include <string.h>     /* memset() */
#include <errno.h>      /* errno */
#include <unistd.h>     /* usleep */
#include <time.h>       /* select */
#include "midas.h"
#include "experim.h"
#include "tig4g.h"
#include "grifc.h"

#include "CaenDigitizer.hh"

#define STRING_LEN      256 // typical length for temporary strings
#define WORD_LEN         32 // typical length for short tmp strings
#define MAX_EVT_SIZE 500000 // was originally 500000
#define MSG_BUF_SIZE  10000 // for single packet messages
#define EVTBUFSIZ     65536 // max single event-fragment size is 2*this (ints)

#ifndef MAX_EVENT_SIZE                       /* value can be set via Makefile */
#define MAX_EVENT_SIZE         0x400000      /**< maximum event size 4MB      */
#endif

// grifc control register
// NOTE PARAMRUN_ON, ESATA_OFF is REQUIRED on the toplevel module
// (in either 1 or 2 or 3 level system) in order to have any run control
// also two level system with no ppg control will require digitisers to be
// changed from default esata-only run control, to get any data
// 1-level system will work ok as needs to be param-controlled
//    CSR_PPG_EVENTS | CSR_ENABLE_PPG | CSR_ENABLE_VME | CSR_ENABLE_FILTER
//    CSR_SETMASTER | CSR_PARAMRUN_ON | CSR_ESATARUN_OFF
#define MASTER_CONFIG CSR_ENABLE_VME | CSR_SETMASTER | \
                      CSR_PARAMRUN_ON | CSR_ESATARUN_OFF | \
       CSR_ENABLE_FILTER|  CSR_PPG_EVENTS | CSR_ENABLE_PPG
//     CSR_ENABLE_FILTER|  CSR_PPG_EVENTS | CSR_ENABLE_PPG
#define TWOLEVGRIFC_CONFIG CSR_PPG_EVENTS | CSR_ENABLE_PPG | CSR_ENABLE_VME | \
                                      CSR_PARAMRUN_ON | CSR_ESATARUN_OFF
#define SINGLELEVEL_CONFIG CSR_PARAMRUN_ON | CSR_ESATARUN_OFF | CSR_ESATACLK_OFF
// Filter control register
//    FILCSR_WRITE_FILTERED | FILCSR_SKIP_UNFILTERED | FILCSR_DISABLE_FILTER
#define FILTER_CONFIG FILCSR_WRITE_FILTERED | FILCSR_SKIP_UNFILTERED     
//#define FILTER_CONFIG          0   // Use this setting to disable the filter (write unfiltered only)
#define DEFAULT_IDLE_PATTERN 0x0   // ppg output when run not in progress
#define NET_PKT_SIZE        7000   // target size of event packets from grif
#define TARGET_EVENT_SIZE  10000   // target size of midas events to disk

#ifdef __cplusplus
extern "C" {
#endif

char fename[STRING_LEN]="fegriffin";

/////////////////////////  Midas Variables ////////////////////////////////////
char *frontend_name = fename;                        /* fe MIDAS client name */
const char *frontend_file_name = __FILE__;               /* The frontend file name */
BOOL frontend_call_loop = FALSE;   /* frontend_loop called periodically TRUE */
int display_period = 0;          /* status page displayed with this freq[ms] */
int max_event_size = MAX_EVT_SIZE;     /* max event size produced by this frontend */
int max_event_size_frag = 5 * 1024 * 1024;       /*max for fragmented events */
int event_buffer_size = 4 * 800000;            /* buffer size to hold events */
extern HNDLE hDB; // Odb Handle

int frontend_init();                  int frontend_exit();
int begin_of_run(int run, char *err); int end_of_run(int run, char *err);
int pause_run(int run, char *err);    int resume_run(int run, char *err);
int frontend_loop(){ return SUCCESS; }
int read_trigger_event(char *pevent, INT off);
int read_scalar_event(char *pevent, INT off);
int read_dt5730_event(char *pevent, INT off);

BANK_LIST trigger_bank_list[] = { /* online banks */
   {"GRF4", TID_DWORD, 16, NULL},
   {""},
};
EQUIPMENT equipment[] = {
   {"Trigger",                                             /* equipment name */
      {1, 0, "SYSTEM",                      /* event ID, trigger mask, Evbuf */
       EQ_POLLED, 0, "MIDAS",         /* equipment type, EventSource, format */
       TRUE, RO_RUNNING,                              /* enabled?, WhenRead? */
       50, 0, 0, 0,                 /* poll[ms], Evt Lim, SubEvtLim, LogHist */
       "", "", "",}, read_trigger_event,                  /* readout routine */
    /*NULL, NULL, trigger_bank_list*/                           /* bank list */
   },
   {"Scalar",                                              /* equipment name */
    {2, 0, "SYSTEM",                        /* event ID, trigger mask, Evbuf */

     EQ_PERIODIC, 0,                          /* equipment type, EventSource */
     "MIDAS", TRUE, RO_RUNNING | RO_ODB,      /* format, enabled?, WhenRead? */
     1000, 0, 0, 0,                 /* poll[ms], Evt Lim, SubEvtLim, LogHist */
     "", "", "",}, read_scalar_event,                     /* readout routine */
   },
   {"VX1730",                                             /* equipment name */
      {3, 0, "SYSTEM",                      /* event ID, trigger mask, Evbuf */
       EQ_POLLED, 0, "MIDAS",         /* equipment type, EventSource, format */
       TRUE, RO_RUNNING,                              /* enabled?, WhenRead? */
       50, 0, 0, 0,                 /* poll[ms], Evt Lim, SubEvtLim, LogHist */
       "", "", "",}, read_dt5730_event,                  /* readout routine */
    /*NULL, NULL, trigger_bank_list*/                           /* bank list */
   },

   {""}
};
////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

HNDLE hSet;
TRIGGER_SETTINGS ts;

#define DEFAULT_MASTER "grifip09" // master
//#define SERVER_HOSTNAME "grifip07" // slave
//#define SERVER_HOSTNAME "grifip13" // grif16d
//#define SERVER_HOSTNAME "grifip12" // grif16c
//#define SERVER_HOSTNAME "grifip11" // grif16b
//#define SERVER_HOSTNAME "grifip10" // grif16
//#define SERVER_HOSTNAME "grsmid00" // test

static int master_grifc, slave_grifc;

#define DATA_PORT 8800
#define CMD_PORT  8808

#define WRITE 1 // parameter encoding
#define READ  0

static int fe_verbose, debug_data, check_data;
static int addr_len = sizeof(struct sockaddr);
static struct sockaddr_in cmd_addr, data_addr;
static int data_socket, cmd_socket;
static char msgbuf[STRING_LEN];
static char replybuf[MSG_BUF_SIZE]; // was max_event_size
static int replybufbytes;
static float EPICS_Rates[20]; // Values from scalars and sent to ODB
//static float last_Rates[11];

CaenDigitizer* gDigitizer = nullptr;

void param_encode(char *buf, int par, int write, int chan, int val);
int  param_decode(unsigned char *buf, int *par, int *chan, int *val);
int  open_udp_socket(char *host, int port, struct sockaddr_in *addr);
int  open_tcp_socket(char *host, int port, struct sockaddr_in *addr);
int  sndmsg(int fd, struct sockaddr_in *addr, const char *msg, int len, char *reply);
int  testmsg(int socket, int timeout), readmsg(int socket);
void grifc_eventread_init();
int  grifc_eventread(int, int addr, int *dst, int *, int *, int *extra_info);
int  write_settings(HNDLE, HNDLE), dump_data_buffers(int);
int check_counts(unsigned *replybuf, int words), report_counts(int);

void seq_callback(INT hDB, INT hseq, void *info)
{ printf("odb ... trigger settings touched\n"); }

#ifdef __cplusplus
extern "C" {
#endif

int interrupt_configure(INT cmd, INT source, PTYPE adr){ return SUCCESS; }

int pause_run(INT run_number, char *error){ return SUCCESS; }
int resume_run(INT run_number, char *error){ return SUCCESS; }
int frontend_exit(){ delete gDigitizer; return SUCCESS; }
int frontend_init()
{
   char tmp_str[STRING_LEN], **argv;
   int i, par, status, size, argc;

   // to allow fe name to change, frontend_init() has been moved in mfe.c
   // and is now called before the frontend connects to the odb
   // i.e. calling any odb functions in frontend_init() will now fail

   //TRIGGER_SETTINGS_STR(trigger_settings_str); // Map odb Trigger/settings
   //sprintf(tmp_str, "/Equipment/Trigger/Settings");
   //status = db_create_record(hDB, 0, tmp_str, strcomb(trigger_settings_str));
   //status = db_find_key (hDB, 0, tmp_str, &hSet);
   //if( status != DB_SUCCESS ) cm_msg(MINFO,"FE","Key %s not found", tmp_str);
   //size = sizeof(TRIGGER_SETTINGS); // Enable hot-link on trigger/settings
   //if( (status = db_open_record(hDB, hSet, &ts, size, MODE_READ, seq_callback,
   //                              NULL)) != DB_SUCCESS){ return status; }

   mfe_get_args(&argc, &argv); // grab program arguments
   strcpy(tmp_str, DEFAULT_MASTER);
	for(i=0; i<argc; i++){
		if( (argv[i])[0] == '-' && (argv[i])[1] == 'i' && i+1<argc){
			if( (size = strlen(argv[i+1])) >= STRING_LEN ){ size = STRING_LEN-1; }
			strncpy(tmp_str, argv[1+i++], size); tmp_str[size]=0;
			sprintf(fename, "fe%s", tmp_str);
         continue;
      }
      if( strcmp("-dumpevents",  argv[i]) == 0 ){ debug_data = 1; continue; }
		if( strcmp("-dumpdata",   argv[i]) == 0 ){ check_data = 1; continue; }
      if( (argv[i])[0] == '-' && (argv[i])[1] == 'v' ){ fe_verbose=1; }
    //if( (argv[i])[0] == '-' && (argv[i])[1] == 'm' ){ master_grifc=1; }
    //if( (argv[i])[0] == '-' && (argv[i])[1] == 's' ){ slave_grifc =1; }
   }
   if(      strcmp(tmp_str, "grifip09") == 0 ||
            strcmp(tmp_str, "grifloc" ) == 0){ master_grifc=1; }
   else if( strcmp(tmp_str, "grifip07") == 0 ||
            strcmp(tmp_str, "grifip08") == 0 ){ slave_grifc =1; }
   else if( strcmp(tmp_str, "grifadc01") == 0 ||
            strcmp(tmp_str, "grifadc02") == 0 ||
            strcmp(tmp_str, "grifadc03") == 0 ||
            strcmp(tmp_str, "grifadc04") == 0 ||
            strcmp(tmp_str, "grifadc05") == 0 ||
            strcmp(tmp_str, "grifadc09") == 0 ||
            strcmp(tmp_str, "grifadc10") == 0 ||
            strcmp(tmp_str, "grifadc11") == 0 ||
            strcmp(tmp_str, "grifadc12") == 0 ||
            strcmp(tmp_str, "grifadc13") == 0 ||
            strcmp(tmp_str, "grifadc14") == 0 ||
            strcmp(tmp_str, "grifadc15") == 0 ||
            strcmp(tmp_str, "grifadc16") == 0 ||
            strcmp(tmp_str, "grifadc17") == 0 ||
            strcmp(tmp_str, "grifadc18") == 0 ||
            strcmp(tmp_str, "grifadc19") == 0 ){ /*grif16*/; }
   else {
      printf("unrecognised data source: %s\n", tmp_str );
      return(FE_ERR_HW); // could attempt to carry on here ...
   }
   if( master_grifc ){
      printf("Module is Master Grifc [3-level system]\n");
   } else if( slave_grifc ){
      printf("Module is Slave Grifc [2-level system]\n");
   } else {
      printf("Module is Digitizer [1-level system]\n");
   }
   printf("griffin init ... connecting to %s\n", tmp_str );
   if( (data_socket = open_udp_socket(tmp_str, DATA_PORT,&data_addr)) < 0 ){ ; }
   if( (cmd_socket  = open_udp_socket(tmp_str, CMD_PORT, &cmd_addr )) < 0 ){ ; }
        
   printf("Writing parameters ...\n");
   if( master_grifc ){
      printf("   Setting Master GRIF-C ...\n");
      param_encode(msgbuf, PARAM_CSR, WRITE, GRIFC_MSTPAR, par = MASTER_CONFIG);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write Master GRIF-C config[0x%08x], (reply:%d bytes) ...\n", par, status);
      printf("      Master Bit Set: %s",(par & CSR_SETMASTER  )  ?"yes":"no ");
      printf("    Filter Active : %s\n",(par & CSR_ENABLE_FILTER)?"yes":"no ");
      printf("      PPG Events    : %s",(par & CSR_PPG_EVENTS )  ?"yes":"no ");
      printf("    PPG Control   : %s",  (par & CSR_ENABLE_PPG )  ?"yes":"no ");
      printf("    VME enabled   : %s\n",(par & CSR_ENABLE_VME )  ?"yes":"no ");
      printf("      Param RunCtrl : %s",(par & CSR_PARAMRUN_ON ) ?"yes":"no ");
      printf("    Esata RunCtrl : %s\n",(par & CSR_ESATARUN_OFF) ?"no ":"yes");
      
      param_encode(msgbuf,PPG_VMEADDR, WRITE, GRIFC_PPGPAR,par=DEFAULT_VMEADDR);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write VME address of PPG module[0x%08x], reply:%d bytes\n", par, status);

      param_encode(msgbuf, PPG_IDLEPAT, WRITE, GRIFC_PPGPAR, par=DEFAULT_IDLE_PATTERN);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write PPG idle pattern         [%-10d], reply:%d bytes\n", par, status);

      param_encode(msgbuf, PARAM_NETSIZE, WRITE, GRIFC_MSTPAR,par=NET_PKT_SIZE);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write Network Packet Size      [%-10d], reply:%d bytes\n", par, status);
   } else if( slave_grifc ){
      printf("   Setting Grifc [2 level system] ...\n");
      param_encode(msgbuf,PARAM_CSR,WRITE,GRIFC_MSTPAR, par=TWOLEVGRIFC_CONFIG);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write GRIF-C config[0x%08x], (reply:%d bytes) ...\n", par, status);
      printf("      Master Bit Set: %s",(par & CSR_SETMASTER  )  ?"yes":"no ");
      printf("    Filter Active : %s\n",(par & CSR_ENABLE_FILTER)?"yes":"no ");
      printf("      PPG Events    : %s",(par & CSR_PPG_EVENTS )  ?"yes":"no ");
      printf("    PPG Control   : %s",  (par & CSR_ENABLE_PPG )  ?"yes":"no ");
      printf("    VME enabled   : %s\n",(par & CSR_ENABLE_VME )  ?"yes":"no ");
      printf("      Param RunCtrl : %s",(par & CSR_PARAMRUN_ON ) ?"yes":"no ");
      printf("    Esata RunCtrl : %s\n",(par & CSR_ESATARUN_OFF) ?"no ":"yes");

      param_encode(msgbuf, PARAM_NETSIZE, WRITE, GRIFC_MSTPAR, par=NET_PKT_SIZE);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write Network Packet Size      [%d], reply:%d bytes\n", par, status);
   } else {
      printf("   Setting Single level system ...\n");
      param_encode(msgbuf,PARAM_CSR,WRITE,GRIF16_PAR, par=SINGLELEVEL_CONFIG);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write Digitiser config[0x%08x], (reply:%d bytes) ...\n", par, status);
      printf("      Master Bit Set: %s",(par & CSR_SETMASTER  )  ?"yes":"no ");
      printf("    Filter Active : %s\n",(par & CSR_ENABLE_FILTER)?"yes":"no ");
      printf("      PPG Events    : %s",(par & CSR_PPG_EVENTS )  ?"yes":"no ");
      printf("    PPG Control   : %s",  (par & CSR_ENABLE_PPG )  ?"yes":"no ");
      printf("    VME enabled   : %s\n",(par & CSR_ENABLE_VME )  ?"yes":"no ");
      printf("      Param RunCtrl : %s",(par & CSR_PARAMRUN_ON ) ?"yes":"no ");
      printf("    Esata RunCtrl : %s\n",(par & CSR_ESATARUN_OFF) ?"no ":"yes");
      printf("    Esata Clock   : %s\n",(par & CSR_ESATACLK_OFF) ?"no ":"yes");

      param_encode(msgbuf, PARAM_NETSIZE, WRITE, GRIF16_PAR, par=NET_PKT_SIZE);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write Network Packet Size      [%d], reply:%d bytes\n", par, status);
   }

	//if(hDB == nullptr) {
	//	std::cout<<"No handle for ODB, aborting"<<std::endl;
	//	exit(0);
	//}

   delete gDigitizer;
   gDigitizer = new CaenDigitizer(hDB);
   printf("done\n");

   return SUCCESS;
}
#ifdef __cplusplus
}
#endif

// initial test ...
//   p,a, r,m,   2,0, 0,0,   83,3, 0,0    len=12
//   data sent to chan starts with rm, ends with 83,3
//   memcpy(msgbuf, "PARM", 4);
//   *(unsigned short *)(&msgbuf[4]) =   2; // parnum = 2       [0x0002]
//   *(unsigned short *)(&msgbuf[6]) =   0; // op=write, chan=0 [0x0000]
//   *(unsigned int   *)(&msgbuf[8]) = 899; // value = 899 [0x0000 0383]
void param_encode(char *buf, int par, int write, int chan, int val)
{
   memcpy(buf, "PARM", 4);
   msgbuf [4] = (par & 0x3f00)     >>  8; msgbuf[ 5] = (par & 0xff);
   msgbuf[ 6] = (chan& 0xff00)     >>  8; msgbuf[ 7] = (chan& 0xff);
   msgbuf[ 8] = (val & 0xff000000) >> 24; msgbuf[ 9] = (val & 0xff0000) >> 16;
   msgbuf[10] = (val & 0xff00    ) >>  8; msgbuf[11] = (val & 0xff);
   if( ! write ){ msgbuf[4] |= 0x40; }
}

int param_decode(unsigned char *buf, int *par, int *chan, int *val)
{
   if( strncmp((char *)buf, "RDBK", 4) != 0 ){ return(-1); } // wrong header
   if( ! (buf[4] & 0x40) ){ return(-1); } // readbit not set
   *par  = ((buf[4] & 0x3f ) << 8) | buf[5];
   *chan = ((buf[6] & 0xff ) << 8) | buf[7];
   *val  = (buf[8]<<24) | (buf[9]<<16) | (buf[10]<<8) | buf[11];
   return(0);
}

#ifdef __cplusplus
extern "C" {
#endif

int begin_of_run(int run_number, char *error)
{
   int i;//, status, size;

   if(gDigitizer != nullptr) gDigitizer->StartAcquisition(hDB);

   //size = sizeof(TRIGGER_SETTINGS); /* read Triggger settings again ? */
   //if ((status = db_get_record (hDB, hSet, &ts, &size, 0)) != DB_SUCCESS){
   //   return status;
   //}
   printf("Writing parameters ...\n");
   write_settings(hDB, hSet);

   printf("Starting ACQ ...\n");
   printf("requesting data ...\n");
   if( sndmsg(data_socket, &data_addr, "DRQ ", 4, NULL) < 0 ){
      printf("   datarequest failed\n");
   }
   if( testmsg(cmd_socket,10000) > 0 ){
      printf("reply:%d bytes\n", readmsg(cmd_socket) );
   } else {
      printf("reply:No Reply to DataStart Rcvd!\n");
   }
   //grifc_eventread_init();
   dump_data_buffers(data_socket);
   // set run-bit csr[8] - par 63 on chan255 or 3840 to 256
   // [master also needs bit9 set]
   if( master_grifc ){
      param_encode(msgbuf, PARAM_SSCSR, WRITE, GRIFC_MSTPAR, CSR_RUN);
   } else if( slave_grifc ){
      param_encode(msgbuf, PARAM_SSCSR, WRITE, GRIFC_SLAVEPAR, CSR_RUN);
   } else {
      param_encode(msgbuf, PARAM_SSCSR, WRITE, GRIF16_PAR, CSR_RUN);
   }
   i = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf); // 96/144 bits
   printf("reply:%d bytes\n", i); 
   //last_event_update = -1;

   printf("End of BOR\n");
   return SUCCESS;
}

int end_of_run(int run_number, char *error)
{
  int i,size;
   char tmp[STRING_LEN];
   // clr run-bit csr[8] - par 63 on chan 255 to 0
   // [master needs bit9 to remain set]
   if( master_grifc ){
      param_encode(msgbuf, PARAM_SCCSR, WRITE, GRIFC_MSTPAR, CSR_RUN);
   } else if( slave_grifc ){
      param_encode(msgbuf, PARAM_SCCSR, WRITE, GRIFC_SLAVEPAR, CSR_RUN);
   } else {
      param_encode(msgbuf, PARAM_SCCSR, WRITE, GRIF16_PAR, CSR_RUN);
   }
   i = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf); // 96/144 bits
   printf("reply:%d bytes\n", i); 

   dump_data_buffers(data_socket);

   if( sndmsg(data_socket, &data_addr, "STOP", 4, NULL) < 0 ){
      printf("   datastop failed\n");
   }
   if( testmsg(cmd_socket,10000) > 0 ){
      printf("reply:%d bytes\n", readmsg(cmd_socket) );
   } else {
      printf("reply:No Reply to DataStop Rcvd!\n");
   }

   memset(EPICS_Rates, 0, sizeof(EPICS_Rates) ); // reset epics rates to zero
   // Copy EPICS scalar values to ODB
   sprintf(tmp,"/Equipment/Scalar/Variables/Rates");
   size = sizeof(EPICS_Rates);
   if( (db_set_value(hDB,0,tmp,&EPICS_Rates,size,size/sizeof(EPICS_Rates[0]),TID_FLOAT)) != DB_SUCCESS){
     cm_msg(MINFO,"FE","Can't set value for Key %s",tmp); return(-1);}
   
   printf("stopping VX1730 digitizer ...\n");
   if(gDigitizer!= nullptr) gDigitizer->StopAcquisition();
   printf("done!\n");

   return SUCCESS;
}
#ifdef __cplusplus
}
#endif

#define MIN_EVT_SIZE 6 /* 32bit words */
/* test/count mode is used to determine poll timing */
static int data_available;
static bool caen_data_available;

#ifdef __cplusplus
extern "C" {
#endif

INT poll_event(INT source, INT count, BOOL test)
{
   int i, have_data=0;

   for(i=0; i<count; i++){
		// we can't check if there is data w/o reading it. To ensure we don't overwrite what we've read
		// we wait until read_dt5730_event sets caen_data_available back to false
		if( !caen_data_available  && gDigitizer != nullptr) { 
			caen_data_available = gDigitizer->DataReady();
		}
      if( caen_data_available ){ break; }
      if( data_available ){ break; }
      have_data = ( testmsg(data_socket, 0) > 0 );
      if( have_data && !test ){ break; }
   }
   return( (have_data || data_available || caen_data_available) && !test );
}

#ifdef __cplusplus
}
#endif

void printreply(int bytes)
{
   int i; printf("  ");
   for(i=0; i<bytes; i++){
      printf("%02x", replybuf[i]);  if( !((i+1)%2) ){ printf(" "); }
   } printf("::");
   for(i=0; i<bytes; i++){
      printf( (replybuf[i]>20 && replybuf[i]<127) ? "%c" : ".", replybuf[i]);
   } printf("\n");
}

// param format ...
//    parm [32bit] x50,41,52,4D
//       1bit used in firmware + 15bit parnum 
//       16bit addr: 4bit ctrl, 4bitgrifc, 4bit port, 4bit chan
//       32bit val
#define NUM_PPG_PATTERN 10
#define NUM_FILTERPAR   64 // 16 conditions of 4 words each -> 64 words
#define NUM_PRESCALES    8 // 16 prescales of   16bits each -> 8 words
static int ppg_codes[NUM_PPG_PATTERN], ppg_durations[NUM_PPG_PATTERN];
static int filter_par[NUM_FILTERPAR], prescales[NUM_PRESCALES], winsize, mask;
extern "C" int get_ppg_program(HNDLE, HNDLE, int *, int *);
extern "C" int get_filter_prog(HNDLE, HNDLE, int *, int *, int *, int *);
int write_settings(HNDLE hDB, HNDLE hSet) // odb, trigset
{
   int i, j, par, status, size;

   if( master_grifc ){
      param_encode(msgbuf, PPG_VMEADDR, WRITE, GRIFC_PPGPAR, par=DEFAULT_VMEADDR);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write VME address of PPG module[0x%08x], reply:%d bytes\n", par, status);

      param_encode(msgbuf, PPG_IDLEPAT, WRITE, GRIFC_PPGPAR, par=DEFAULT_IDLE_PATTERN);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write PPG idle pattern         [%-10d], reply:%d bytes\n", par, status);

      param_encode(msgbuf, PARAM_NETSIZE, WRITE, GRIFC_MSTPAR, par=NET_PKT_SIZE);
      status = sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf);
      printf("   Write Network Packet Size      [%-10d], reply:%d bytes\n", par, status);
   }
   printf("   clearing network buffers ");
   while( readmsg(cmd_socket) > 0 ){ printf("."); }
   printf("\n");

   if( master_grifc ){
      printf("   Writing ppg program ...\n");
      if( get_ppg_program(hDB, hSet, ppg_codes, ppg_durations) != 0 ){
         cm_msg(MINFO,"FE","failed to read ppg program"); return(-1);
      }
      for(i=0; i<NUM_PPG_PATTERN; i++){
	if( ppg_durations[i] ){
            param_encode(msgbuf, PPGPROG_OFFSET+2*i, WRITE, GRIFC_PPGPAR, ppg_durations[i] );
            if( (size=sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf)) != 4 ){
	       cm_msg(MINFO,"FE","failed to write ppg duration %d [%d bytes:%s]",
                  i, size, replybuf); return(-1);
            }
            param_encode(msgbuf, PPGPROG_OFFSET+2*i+1, WRITE, GRIFC_PPGPAR, ppg_codes[i] );
            if( (size=sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf)) != 4 ){
	       cm_msg(MINFO,"FE","failed to write ppg code %d [%d bytes:%s]",
                  i, size, replybuf); return(-1);
            }
            printf("      #%2d - Duration: %8d[0x%08x] Pattern:0x%08x\n",
		   i, ppg_durations[i], ppg_durations[i], ppg_codes[i]); }
      }
      printf("   Writing filter program ...\n");
      if( get_filter_prog(hDB, hSet, filter_par, prescales, &winsize, &mask) != 0 ){
         cm_msg(MINFO,"FE","failed to read filter program"); return(-1);
      }
      
      param_encode(msgbuf, PARAM_CSR, WRITE, GRIFC_FILPAR, par=FILTER_CONFIG);
      if( (size=sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf)) != 4 ){
         cm_msg(MINFO,"FE","failed to write filter csr [%d bytes:%s]",
            size, replybuf); return(-1);
      } printf("      filter control register: 0x%08x\n", par );
      printf("        Unfiltered Data:%s\n",(par&FILCSR_SKIP_UNFILTERED)?"no":"yes");
      printf("        Filtered Data  :%s\n",(par&FILCSR_WRITE_FILTERED )?"yes":"no");

      param_encode(msgbuf, PARAM_COINCWIN, WRITE, GRIFC_FILPAR, par=winsize );
      if( (size=sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf)) != 4 ){
         cm_msg(MINFO,"FE","failed to write coinc window size [%d bytes:%s]",
            size, replybuf); return(-1);
      } printf("      filter window size: %d [ns]\n", 10*winsize );

      param_encode(msgbuf, PARAM_DETMASK, WRITE, GRIFC_FILPAR, par=mask );
      if( (size=sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf)) != 4 ){
         cm_msg(MINFO,"FE","failed to write detector mask [%d bytes:%s]",
            size, replybuf); return(-1);
      } printf("      filter detector mask: 0x%08x\n", mask );

      printf("      filter program ...[16 conditions of 4 * 4*8bits]\n       ");
      // must write entire program (or parts of old program may stay active)
      for(i=0,j=0; i<NUM_FILTERPAR; i++){
        param_encode(msgbuf,FILTPROG_OFFSET+i,WRITE,GRIFC_FILPAR,filter_par[i]);
         if( (size=sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf)) != 4 ){
	    cm_msg(MINFO,"FE","failed to write filter param %d [%d bytes:%s]",
               i, size, replybuf); return(-1);
         }
	 if( filter_par[i] ){ ++j;
	   printf("  #%2d:[%2dw%1d]:0x%08x", i, i/4, i%4, filter_par[i]);
         }
         if( j == 3 ){  printf("\n       "); j=0; }
      }
      if( j ){ printf("\n"); } printf("      Prescales ...\n       ");
      for(i=0,j=0; i<NUM_PRESCALES; i++){
         param_encode(msgbuf,PRESCALE_OFFSET+i,WRITE,GRIFC_FILPAR,prescales[i]);
         if( (size=sndmsg(cmd_socket, &cmd_addr, msgbuf, 12, replybuf)) != 4 ){
	    cm_msg(MINFO,"FE","failed to write prescale %d [%d bytes:%s]",
               i, size, replybuf); return(-1);
         }
	 if( prescales[i] ){ printf("  #%2d:0x%08x", i, prescales[i] ); }
         if( j == 4 ){  printf("\n       "); j=0; }
      }
       printf("\n");
   }
   return(0);
}

int test_settings()
{
   int status, size, winsize, mask;

   size = sizeof(TRIGGER_SETTINGS); /* read Triggger settings again ? */
   if ((status = db_get_record (hDB, hSet, &ts, &size, 0)) != DB_SUCCESS){
      return status;
   }
   //write_settings(hDB, hSet);
   get_filter_prog(hDB, hSet, filter_par, prescales, &winsize, &mask);
   return(0);
}

#define GRIF16_CHAN 16
#define GRIF16_PORT 80
//#define GRIF16_NUM  5
#define GRIF16_NUM  12

#define TICKS_PER_S 24414.0625 // tick=10ns*4096=41us => 24k ticks/s
// times are 28+16=44bits with 8bits overlap in event trailer
//    can >>12 to get 32bit count in units of 41us
// data[0-3] is prev_time, prev_count, time, count
static unsigned scalar_data[GRIF16_NUM][GRIF16_CHAN][4];
int record_scalar(char *replybuf, int words) // record scalar data from stream
{  // **events here have no network pkt num yet** => hdr, t, 4sclr trlr
   int mod, chan, tstamp, counts;
   int *ptr = (int *)replybuf;
   // the firmware was changed for some reason to give both 0xe0
   //    so now have to use word4,5=0 to distinguish
   //if( (ptr[0] & 0xF000000F) != 0x8000000F ||
   //    (ptr[6] & 0xFF000000) != 0xE1000000 ){
   if( (ptr[0] & 0xF000000F) != 0x8000000F || ptr[4] != 0 || ptr[5] != 0 ){
      return(-1); // -1 => NOT RATE SCALAR - LEAVE IN DATASTREAM
   }
   mod  = (ptr[0] & 0xff000) >> 12;
   // mod needs adjusting from 0-255 to correct range
   chan = (ptr[0] & 0x00ff0) >>  4;

   if( chan < 0 || chan >= GRIF16_CHAN || mod < 0 || mod >= GRIF16_NUM ){
      return(0); // silently drop rates we don't care about
   }
   tstamp = ((ptr[6] & 0xffff00)<<8) | ((ptr[1] & 0xffff000)>>12);
   counts = ptr[2];
   scalar_data[mod][chan][0] = scalar_data[mod][chan][2];
   scalar_data[mod][chan][1] = scalar_data[mod][chan][3];
   scalar_data[mod][chan][2] = tstamp;
   scalar_data[mod][chan][3] = counts;

   return(0);
}

#ifdef __cplusplus
extern "C" {
#endif

// below assumes sceptar is in grif16#2
INT read_scalar_event(char *pevent, INT off)
{
   int *pdata, module, chan, size, val, tdiff, cdiff;
   char tmp[STRING_LEN];

   bk_init(pevent);
   bk_create(pevent, "SCLR", TID_DWORD, reinterpret_cast<void**>(&pdata));
   memset(EPICS_Rates, 0, sizeof(EPICS_Rates) ); // reset epics rates to zero
   for(module=0; module<GRIF16_NUM; module++){
      *(pdata++) = 0x80000FFF | (module << 12);
      for(chan=0; chan<GRIF16_CHAN; chan++){
         tdiff = scalar_data[module][chan][2] - scalar_data[module][chan][0];
         cdiff = scalar_data[module][chan][3] - scalar_data[module][chan][1];
	 if( tdiff == 0 ){ tdiff = 0x7FFFFFFF; }
         val  = TICKS_PER_S * cdiff / ((tdiff > 0) ? 1.0*tdiff : -1.0*tdiff);
	 *(pdata++) = val;

	 //new arrangement using x10 Rev1 GRIF-16s. All HPGe are still on collector 0, SCEPTAR on collector 0
	 if(module<4 ){ EPICS_Rates[0] += val; } 								// HPGe array low gain rate
	 if(module>=4 && module<8 ){ EPICS_Rates[12] += val; } 	// HPGe array high gain rate
	 if(module==8){ EPICS_Rates[1] += val; } 		// beta rate (SCEPTAR+ZDS)
      }
      *(pdata++) = 0xE0000FFF | (module << 12);
   }
   bk_close(pevent, pdata);

   // Copy EPICS scalar values to ODB
   sprintf(tmp,"/Equipment/Scalar/Variables/Rates");
   size = sizeof(EPICS_Rates);
   if( (db_set_value(hDB,0,tmp,&EPICS_Rates,size,size/sizeof(EPICS_Rates[0]),TID_FLOAT)) != DB_SUCCESS){
     cm_msg(MINFO,"FE","Can't set value for Key %s",tmp); return(-1);}
   
    return bk_size(pevent);
}

// readout sequence ...
// poll_event() true (if still have data in buffer or testmsg() true)
// -> read_trigger_event() -> read_grifc_event() - re-buffers into midas events
//                         -> grifc_eventread()  - returns single grif fragment
//                         -> grifc_dataread()   - returns single net-pkt
int read_grifc_event(char *pevent, int tig4g_id);
int read_trigger_event(char *pevent, int off)
{
   static int grifc_id=0;
   return( read_grifc_event(pevent, grifc_id) ); // return 0; discards event? */
}

int read_dt5730_event(char *pevent, int off)
{
	//printf("read_dt5730_event, caen_data_available %s\n", caen_data_available?"true":"false");
	if(!caen_data_available) {
		return 0;
	}
   bk_init(pevent);

	uint32_t nofEvents = 0;
   if(gDigitizer != nullptr) nofEvents = gDigitizer->ReadData(pevent, "CAEN");
	caen_data_available = false;

	if(nofEvents > 1) {
		SERIAL_NUMBER(pevent) += nofEvents - 1;
	}

   return bk_size(pevent);
}

#define UPDATE_INTERVAL 10 // seconds
static char multi_evt_buf[MAX_EVT_SIZE]; int multi_evt_pos;
static char progress[] = "|/-\\";
static int last_event_update = -1; // also set to -1 on BOR;
static char one_ev_buf[MAX_EVT_SIZE];
/* read multiple griffin fragments into midas events (return(0)) ... */
/* until TARGET_EVENT_SIZE met or timeout expired (return(evtsize))  */
/*              storage is in multi_evt_buf, multi_evt_pos           */
int read_grifc_event(char *pevent, int grifc_id)
{
   static int i_bar, lastev_count, ev_count, update_cnt, multi_cnt;
   int *pdata, *ptr, words, pkt_num=0, type, serial, header, word_count; 
   static int lastpkt_count, lastmid_count, mid_count;
   time_t curr_time = time(NULL);
   //if( last_event_update == -1 ){ last_event_update = curr_time; }

   // packetnumber is removed from replybuf and returned as pkt_num argument
   grifc_eventread(data_socket, 0x1234, (int *)one_ev_buf,&words,&pkt_num,&type);
   if( words < 1 ){ return(0); }
   if( pkt_num != 0 ){ pkt_num = 0xD0000000 | (pkt_num & 0xfffffff); }
   ++ev_count;
   if( type != 0 ){  // record rate scalar events, and then remove from datastream
      if( record_scalar(one_ev_buf,words) == 0 ){ return(0); }
   }                 // leave deadtime scalars in datastream
   if( check_data ){ check_counts((unsigned *)one_ev_buf,words); }

   ++multi_cnt;
   ptr = (int *)(multi_evt_buf+multi_evt_pos);
   // IF non-zero, insert packet number after Nth word, defined below
   // NETPOS = 1 places the packet number immediately after the header
   #define NETPOS 1
   if( pkt_num != 0 ){
      multi_evt_pos += (words+1)*sizeof(int);
      if( multi_evt_pos >= MAX_EVENT_SIZE ){
         fprintf(stderr,"Exceeded eventsize [now at %d] with event:%d\n",
            multi_evt_pos, words*4 );
      }
      memcpy(ptr,one_ev_buf,NETPOS*sizeof(int));
      header = *(int *)ptr;  // adjust #words in event header
      word_count = ((header >> 20) & 0x1F) + 1;
      *(int *)ptr = (header & 0xFE0FFFFF) | ((word_count & 0x1F) << 20);
      ptr += NETPOS; words -=NETPOS; *ptr++ = pkt_num;
      memcpy(ptr,one_ev_buf+NETPOS*sizeof(int), words*sizeof(int)); ptr += words;
   } else { // otherwise just copy the event as is
      multi_evt_pos += (words)*sizeof(int);
      if( multi_evt_pos >= MAX_EVENT_SIZE ){
         fprintf(stderr,"Exceeded eventsize [now at %d] with event:%d\n",
            multi_evt_pos, words*4 );
      }
      memcpy(ptr,one_ev_buf,words*sizeof(int)); ptr += words;
   }
   if( multi_evt_pos<TARGET_EVENT_SIZE && curr_time==last_event_update &&
       multi_evt_pos+8*EVTBUFSIZ < MAX_EVENT_SIZE ){
      return(0); // dont risk overflowing midasbuf with unknown-size next-event
   }

   ++mid_count;
   bk_init(pevent);
   if( multi_cnt > 1 ){ SERIAL_NUMBER(pevent) += multi_cnt - 1; }
   bk_create(pevent, "GRF4", TID_DWORD, reinterpret_cast<void**>(&pdata));
   memcpy(pdata, multi_evt_buf, multi_evt_pos); pdata += multi_evt_pos/4;
   bk_close(pevent, pdata);
   multi_evt_pos = 0;  multi_cnt = 0;

   //send_event(), also interrupt_routine() and multithread_readout()
   // also check_polled_events() and scheduler()
   //   set event serial# from equip serial#
   //      they prepare event (inc #), call readout, and if no data, reset #!

   if( curr_time != last_event_update ){
      serial = SERIAL_NUMBER(pevent);
      printf(" %c -", progress[i_bar++%4] );
      printf(" Midas :%8d[%6d/s]", serial, mid_count - lastmid_count );
      printf(" - Frag:%6d/s", ev_count-lastev_count );
      printf(" - Pkt:%6d/s", pkt_num-lastpkt_count );
      printf("          %c", ( (i_bar % 1000) ) ? '\r' : '\n' );
      last_event_update = curr_time;
      lastev_count = ev_count;
      lastpkt_count = pkt_num;
      lastmid_count = mid_count;
      if( !(++update_cnt % UPDATE_INTERVAL) && check_data ){
         report_counts(UPDATE_INTERVAL);
      }
   }
   printf(" %c - Serial:%8d[", progress[i_bar++%4], SERIAL_NUMBER(pevent));
   printf("%c", ( (i_bar % 1000) ) ? '\r' : '\n' );

   return bk_size(pevent); 
}
#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////////
//////////////////////      network data link      ////////////////////////

int sndmsg(int sock_fd, struct sockaddr_in *addr, const char *message, int msglen, char *reply)
{
   struct sockaddr_in client_addr;
   int bytes, flags=0;

   if( (bytes = sendto(sock_fd, message, msglen, flags,
        (struct sockaddr *)addr, addr_len) ) < 0 ){
      fprintf(stderr,"sndmsg: sendto failed\n");
      return(-1);
   }
   if( reply == NULL ){ return(0); } // we don't expect any response
   if( testmsg(sock_fd, 10000) <= 0 ){  // wait 10ms for msg
     fprintf(stderr,"sndmsg: reply expected, but no reply received\n");
      return(-1);
   }
   if( (bytes = recvfrom(sock_fd, reply, max_event_size, flags,
        (struct sockaddr *) &client_addr, (socklen_t *)&addr_len) ) == -1 ){
      fprintf(stderr,"sndmsg: reply expected, but no reply received\n");
      return(-1); 
   }
   return(bytes);
}

// select: -ve=>error, zero=>no-data +ve=>data-avail
int testmsg(int socket, int timeout)
{
	fd_set read_fds;
   struct timeval tv;

   tv.tv_sec = 0;
	tv.tv_usec = timeout;
	FD_ZERO(&read_fds);
	FD_SET(socket, &read_fds);
   return select(socket+1, &read_fds, NULL, NULL, &tv);
}

// reply pkts have 16bit dword count, then #count dwords
int readmsg(int socket)
{
   struct sockaddr_in client_addr;
   int bytes;

   if( testmsg(socket, 10000) <= 0 ){ return(-2); } // wait 10ms for msg
   if( (bytes = recvfrom(socket, replybuf, MSG_BUF_SIZE, 0,
        (struct sockaddr *) &client_addr, (socklen_t *)&addr_len) ) == -1 ){
       return -1;
   }
   return( bytes );
}

int open_udp_socket(char *host, int server_port, struct sockaddr_in *addr)
{
   struct sockaddr_in local_addr;
   struct hostent *hp;
   int sock_fd;

   int mxbufsiz = 0x00800000; /* 8 Mbytes ? */
   int sockopt=1; // Re-use the socket
   if( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
      fprintf(stderr,"udptest: ERROR opening socket\n");
      return(-1);
   }
   setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(int));
   if(setsockopt(sock_fd, SOL_SOCKET,SO_RCVBUF, &mxbufsiz, sizeof(int)) == -1){
      fprintf(stderr,"udptest: setsockopt for buff size\n");
      return(-1);
   }
   memset(&local_addr, 0, sizeof(local_addr));
   local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(0);          // 0 => use any available port
   local_addr.sin_addr.s_addr = INADDR_ANY; // listen to all local addresses
   if( bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr) )<0) {
      fprintf(stderr,"udptest: ERROR on binding\n");
      return(-1);
   }
   // now fill in structure of server addr/port to send to ...
   bzero((char *) addr, addr_len );
   if( (hp=gethostbyname(host)) == NULL ){
      fprintf(stderr,"udptest: can't lookup ip address for host:%s\n", host);
      return(-1);
   }
   memcpy(&addr->sin_addr, hp->h_addr_list[0], hp->h_length);
   addr->sin_family = AF_INET;
   addr->sin_port = htons(server_port);

   return(sock_fd);
}

int open_tcp_socket(char *host, int server_port, struct sockaddr_in *addr )
{
   struct hostent *hp;
   int sock_fd;

   if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      perror("create socket failed");
      return(-1);
   }
   if( (hp=gethostbyname(host)) == NULL){
      perror("get hostname failed");
      close(sock_fd);
      return(-1);
   }
   memset((char *)addr, 0, sizeof(struct sockaddr) );
   addr->sin_family = AF_INET;
   addr->sin_port = htons(server_port);
   memcpy((char *)&addr->sin_addr, hp->h_addr_list[0], hp->h_length);

   if( connect(sock_fd,(struct sockaddr *)addr,sizeof(struct sockaddr)) == -1){
      perror("connect failed");
      close(sock_fd);
      return(-1);
   }
   return(sock_fd);
}

///////////////////////////////////////////////////////////////////////////
//////////////////////        Event assembly       ////////////////////////

/* There is often leftover data after end of an event(start of next event) */
/* Another data transfer will be needed to complete this partial event */
/* Don't want to copy data elsewhere to enable reusing data rcv buffer */
/* - Could have 2 buffers, and write to empty one.  To make sure always have */
/*   an empty one, each would have to be bigger than max vf48 eventsize */
/* Or could just use single buffer >= 3*max eventsize, write to start/end */
/* BUT rcv buffer size here is 1Mbyte, can use this directly as write buffer */
/*             each data transfer could get many events -  */
/*             complete 1 and copy rest ofdata to local buffer */
/* data buffer could be reused for other modules so cannot avoid this copy */
/* each call to this function look for complete events and return, */
/* or get new data, find rest of event, copy remaining data and return */
/* complicated to use circular buffer (may consider if absolutely necessary) */
/* so use single buffer 2*Max_eventsize, and copy any unused data to start */

#define MAX_MODULES    4 /* will not have more than 4 grifc's ? */
typedef struct eventbuf_struct {
  int module_addr;
  int *data;
  int bufpos; /* first valid data */
  int buflen; /* end of valid data */
} EventBuf;
static EventBuf eventbuf[MAX_MODULES];

EventBuf *find_buffer(int module_address)
{
   EventBuf *ptr;
   int i;
   /* find correct buffer for this module */
   for(i=0; i<MAX_MODULES; i++){
      ptr = eventbuf+i;
      if( ptr->module_addr == module_address ){ break; }
      if( ptr->data != NULL ){ continue; }
      /* reached end of allocated buffers - make new one */
      if( (ptr->data = static_cast<int*>(malloc(EVTBUFSIZ*2*sizeof(int)))) == NULL ){ /*!!*2!!*/
         printf("EventRead: can't alloc memory for event reconstr. buffer\n");
         return NULL;
      }
      ptr->module_addr = module_address; break;
   }
   if( i == MAX_MODULES ){
      printf("EventRead: All event reconstruction buffers in use\n");
      return NULL;
   }
   return(ptr);
}

void  grifc_eventread_init() /* dump old data from buffers after end of run */
{
   int i;
   for(i=0; i<MAX_MODULES; i++){
      eventbuf[i].bufpos = eventbuf[i].buflen = 0;
   }
}

int event_item_offset(int *data, int len, int item)
{
   int i = -1;
   while( ++i < len ){
     if( ((int)(data[i] & 0xF0000000)) == item ){ return(i); }
   }
   return(-1);
}

#define MAX_FLUSH_TIME 10
// do not take more than 10 seconds
// assume 100 consecutive reads with no data => empty
int dump_data_buffers(int data_socket)
{
   int i, count, words, pkt_num, type;
   time_t curr_time, start_time = time(NULL);
   fprintf(stdout,"flushing leftover data ...\n");
   for(i=0,count=0; i<100; i++){
      grifc_eventread(data_socket,0x1234,(int *)one_ev_buf,&words,&pkt_num,&type);
      if( words >= 1 ){ ++count; i=0; }
      if( ((curr_time = time(NULL)) - start_time) > MAX_FLUSH_TIME){ break; }
   }
   if( i==100 ){fprintf(stdout,"done");} else {fprintf(stdout,"failed");}
   fprintf(stdout, " [%04d]\n", count);
   return(0);
}

#define GRIF_HEADER  0x80000000
#define GRIF_TRAILER 0xE0000000

// read a single chunk of data (contents of single network packet)
// readmsg limits response to max_netpkt_size[10K]
// readmsg puts data in replybuf
static char prevreply[MSG_BUF_SIZE], prevprevreply[MSG_BUF_SIZE];
static int  prevreplybytes, prevprevreplybytes;
int grifc_dataread(int data_socket, char *ptr)
{
  //static char *firstptr;
  //static int pkt;
   int bytes;

   if(  testmsg(data_socket, 10000)  < 0 ){ return(0); }

   if( fe_verbose ){
      memcpy(prevprevreply, prevreply, prevreplybytes);
      prevprevreplybytes = prevreplybytes;
      memcpy(prevreply, replybuf, replybufbytes);
      prevreplybytes = replybufbytes;
   }

   if( ( bytes=readmsg(data_socket)) < 2 ){ return(0); }
   // data packets are now all data - should be multiple of 4 bytes
   memcpy(ptr, replybuf, bytes);  replybufbytes = bytes;

   if( debug_data ){
      int i, j, x, y, k, e, t;
      fprintf(stdout,"grifc_dataread: received data packet: %d bytes\n", bytes);
      for(i=0; i<(bytes+3)/4; i++){
         fprintf(stdout,"  0x%08x%s", *(((int *)ptr)+i), (((i+1)%6)?"":"\n") );
      }
      fprintf(stdout,"%s\r", ((i%6)?"\n":"") );
      for(i=5; i<(bytes+3)/4-1; i+=2){ // 1[0], 5[k], 26[e/t]
         j = *(((int *)ptr)+3) & 0x3ffffff;
	 x = *(((int *)ptr)+i); y = *(((int *)ptr)+i+1);
         k = ((x & 0x7c000000) >> 21) | ((y & 0x7c000000) >> 26);
         e = x & 0x03ffffff;  t = y & 0x03ffffff;
         fprintf(stdout,"  T,E,E/K,K=%8d,%6d,%4d,%4d\n",t-j,e,(k!=0)?e/k:0, k);
      }
   }
   return( (bytes+3)/4 ); // units of 32bit (4byte) words (round up)
}

void discarded_data(int *data, int offset, int address)
{
   int i;

   if( !fe_verbose ){ printf("discarded %4d words\n", offset); return; }

   printf("EventRead: Module 0x%06x: Discarded ", address);
   printf("%d data words looking for Event Header\n    ", offset );
   for(i=0; i<offset; i++){
     //if( ((i+1) % 8) == 0 ){ printf("\n    "); }
      printf("  0x%08x", data[i] );
   }
   //if( (i % 8) != 0 ){printf("\n"); }
   
   fprintf(stdout,"PrevPrev data packet: %d bytes\n", prevprevreplybytes);
   for(i=0; i<(prevprevreplybytes+3)/4; i++){
      fprintf(stdout,"  0x%08x%s", *(((int *)prevprevreply)+i), (((i+1)%6)?"":"\n") );
   }
   fprintf(stdout,"%s\r", ((i%6)?"\n":"") );
      
   fprintf(stdout,"Previous data packet: %d bytes\n", prevreplybytes);
   for(i=0; i<(prevreplybytes+3)/4; i++){
      fprintf(stdout,"  0x%08x%s", *(((int *)prevreply)+i), (((i+1)%6)?"":"\n") );
   }
   fprintf(stdout,"%s\r", ((i%6)?"\n":"") );
      
   fprintf(stdout,"Current data packet: %d bytes\n", replybufbytes);
   for(i=0; i<(replybufbytes+3)/4; i++){
      fprintf(stdout,"  0x%08x%s", *(((int *)replybuf)+i), (((i+1)%6)?"":"\n") );
   }
   fprintf(stdout,"%s\r", ((i%6)?"\n":"") );

   printf("\n");
}

int grifc_eventread(int data_socket, int addr, int *pdest, int *nentry, int *pkt_num, int *type)
{
   int i, offset, bufpos, buflen, *data;
   EventBuf *ptr;

   *nentry = 0; data_available = 0;
   if( (ptr = find_buffer(addr)) == NULL){ return(0); }
   data = ptr->data; bufpos = ptr->bufpos; buflen = ptr->buflen;

   // #################  Find event header    #################
   offset = event_item_offset(data+bufpos, buflen-bufpos, GRIF_HEADER);
   if( offset == 1 ){ *pkt_num = data[bufpos++]; offset=0; }
   if( offset > 0 ){ discarded_data(data+bufpos,offset,addr); bufpos+=offset; }
   if( offset== -1){ /* reached end of data, no header found */
                     /* read 10k more data and try again */
      if( buflen != bufpos ){
         fprintf(stdout,"No event found, discarded %4d words ...\n",
             buflen-bufpos );
      }
      if( (i=grifc_dataread(data_socket, (char *)data)) <= 0 ){ return(0); }
      bufpos = 0; ptr->buflen = buflen = i;
      offset = event_item_offset(data+bufpos, buflen-bufpos, GRIF_HEADER);
      if( offset == 1 ){ *pkt_num = data[bufpos++]; offset=0; }
      if( offset > 0){ discarded_data(data+bufpos,offset,addr);bufpos+=offset;}
      if( offset==-1){ ptr->bufpos = bufpos; return(0); }
   }
   // now have event starting at bufpos
   // #################  Find event trailer    #################
   offset = event_item_offset(data+bufpos, buflen-bufpos, GRIF_TRAILER);
   if( offset == -1 ){ /* reached end of data, no trailer found */
                       /* read more data and try again */
     if( buflen + 64 >= EVTBUFSIZ ){ // just use 64 for now - small enough
	 // The buffer hasnt enough room - maybe there is lots of junk at start
         // relocate good data to buffer start, to make room for rest of event
	 memmove(data, data+bufpos, buflen-bufpos); buflen-=bufpos; bufpos=0;
      }
      if( buflen >= 2*EVTBUFSIZ - MSG_BUF_SIZE/4 ){
         fprintf(stdout,"Event Too Large, discarded %5d words ...\n", buflen );
         ptr->bufpos = ptr->buflen = 0; return(0);
      }
      if((i=grifc_dataread(data_socket,(char *)(data+buflen)))<=0){return(0);}
      buflen += i; ptr->buflen = buflen;
      offset = event_item_offset(data+bufpos, buflen-bufpos, GRIF_TRAILER);
      if( offset == -1 ){ ptr->bufpos = bufpos; return(0); }
   }
   // #################       Store event       #################
   ++offset; *nentry = offset; *type=0; while( offset-- ){
      if( (data[bufpos] & 0xf000000f) == 0x8000000f ){ *type |= 1; } //scalar 
      if( (data[bufpos] & 0xff000000) == 0xe1000000 ){ *type |= 2; } //rate
      *pdest++ = data[bufpos++];
   }
   ptr->bufpos = bufpos;
   data_available = (buflen > bufpos);
   return( data_available ); // more data available
}

#define NUM_ADDR 65536
static int addr_count[NUM_ADDR+1];
int check_counts(unsigned *replybuf, int words)
{
   int addr = (replybuf[0] >> 4) & 0xFFFF;
   if( (replybuf[0] >> 28) != 0x8 ){ addr = NUM_ADDR; }
   ++addr_count[addr];
   return(0);
}
int report_counts(int interval)
{
   int i;
   for(i=0; i<=NUM_ADDR; i++){
      if( addr_count[i] == 0 ){ continue; }
      fprintf(stdout,"   Chan:0x%04x [%5d] - %4d/s\n", i, i,
         addr_count[i]/interval );
   }
   memset(addr_count, 0, sizeof(addr_count) );
   return(0);
}
