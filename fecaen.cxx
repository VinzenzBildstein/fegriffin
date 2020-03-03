/* -------------- fecaen: for CAEN VX1730 digitizers ------------- */
/*    fecaen.cxx: midas frontend interface (init,start/stopRun,readEvent) */
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
//#include "experim.h"

#include "CaenDigitizer.hh"

#define STRING_LEN      256 // typical length for temporary strings
#define MAX_EVT_SIZE 1000000 // was originally 500000

//#ifdef __cplusplus
//extern "C" {
//#endif

char fename[STRING_LEN]="fecaen";

/////////////////////////  Midas Variables ////////////////////////////////////
char *frontend_name = fename;                        /* fe MIDAS client name */
const char *frontend_file_name = __FILE__;               /* The frontend file name */
BOOL frontend_call_loop = FALSE;   /* frontend_loop called periodically TRUE */
int display_period = 0;          /* status page displayed with this freq[ms] */
int max_event_size = MAX_EVT_SIZE;     /* max event size produced by this frontend */
int max_event_size_frag = 5 * 1024 * 1024;       /*max for fragmented events */
int event_buffer_size = 4 * max_event_size;            /* buffer size to hold events */
extern HNDLE hDB; // Odb Handle

CaenDigitizer* gDigitizer = nullptr;

static bool caen_data_available;
static uint32_t nofCaenEvents = 0; // variable to output number of caen events since last output

int frontend_init();                  int frontend_exit();
int begin_of_run(int run, char *err); int end_of_run(int run, char *err);
int pause_run(int run, char *err);    int resume_run(int run, char *err);
int frontend_loop(){ return SUCCESS; }
int read_caen_event(char *pevent, INT off);

EQUIPMENT equipment[] = {
   {"VX1730",                                             /* equipment name */
      {3, 0, "SYSTEM",                      /* event ID, trigger mask, Evbuf */
       EQ_POLLED, 0, "MIDAS",         /* equipment type, EventSource, format */
       TRUE, RO_RUNNING,                              /* enabled?, WhenRead? */
       50, 0, 0, 0,                 /* poll[ms], Evt Lim, SubEvtLim, LogHist */
       "", "", "",}, read_caen_event,                  /* readout routine */
   },

   {""}
};
////////////////////////////////////////////////////////////////////////////
//#ifdef __cplusplus
//}
//#endif

HNDLE hSet;

//#ifdef __cplusplus
//extern "C" {
//#endif

int pause_run(INT run_number, char *error){ return SUCCESS; }
int resume_run(INT run_number, char *error){ return SUCCESS; }
int interrupt_configure(INT cmd, INT source, PTYPE adr){ return SUCCESS; }

int frontend_exit()
{ 
	delete gDigitizer; 
	return SUCCESS;
}

int frontend_init()
{
   delete gDigitizer;
   gDigitizer = new CaenDigitizer(hDB, false);
	gDigitizer->PrintEventsPerAggregate();
	gDigitizer->PrintAggregatesPerBlt();

   printf("init done\n");
   return SUCCESS;
}

int begin_of_run(int run_number, char *error)
{
	nofCaenEvents = 0;
   if(gDigitizer != nullptr) gDigitizer->StartAcquisition(hDB);

	gDigitizer->PrintEventsPerAggregate();
	gDigitizer->PrintAggregatesPerBlt();

   printf("End of BOR\n");
   return SUCCESS;
}

int end_of_run(int run_number, char *error)
{
   printf("\nstopping VX1730 digitizer ...\n");
   if(gDigitizer!= nullptr) gDigitizer->StopAcquisition();
   printf("done, read %u CAEN VX1730 events!\n", nofCaenEvents);

   return SUCCESS;
}
//#ifdef __cplusplus
//}
//#endif

/* test/count mode is used to determine poll timing */

//#ifdef __cplusplus
//extern "C" {
//#endif

INT poll_event(INT source, INT count, BOOL test)
{
   for(int i=0; i<count; i++){
		// we can't check if there is data w/o reading it. To ensure we don't overwrite what we've read
		// we wait until read_caen_event sets caen_data_available back to false
		if( !caen_data_available  && gDigitizer != nullptr) { 
			caen_data_available = gDigitizer->DataReady();
		}
      if( caen_data_available ){ break; }
   }
   return( (caen_data_available) && !test );
}

int read_caen_event(char *pevent, int off)
{
	// The CaenDigitizer::ReadData function will take events out of the buffer
	// which was filled by CaenDigitizer::DataReady and return whether the buffer
	// still has events left.
	// Arguments for it are the midas event buffer, the bank name, the maximum size
	// to write to the midas buffer, and sets the number of events read
	//printf("read_caen_event, caen_data_available %s\n", caen_data_available?"true":"false");
	if(!caen_data_available) {
		return 0;
	}
   bk_init(pevent);

	uint32_t nofEvents = 0;
   if(gDigitizer != nullptr) caen_data_available = gDigitizer->ReadData(pevent, max_event_size, nofEvents);

	if(nofEvents > 1) {
		SERIAL_NUMBER(pevent) += nofEvents - 1;
		nofCaenEvents += nofEvents;
	}

	gDigitizer->Status();

   return bk_size(pevent);
}
//#ifdef __cplusplus
//}
//#endif

