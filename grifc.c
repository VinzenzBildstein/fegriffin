#include <stdio.h>
#include <string.h>
#include <stdlib.h>  /* malloc */
#include <unistd.h>  /* for usleep */
#include <time.h>
#include "grifc.h"

#define MASTER_GRIFC                 0 /* could allow this to vary ... */
                      /* => would need argument to acqstart/stop/reset */

int grifc_init(void)
{
   //int i;
   // open tcp control connection[s]
   return(0);
}

void grifc_setCSR(int grifc_id, int value)
{
   char cmd[256]; sprintf(cmd,"set_csr %d", value); // sendto
}

void grifc_AcqStart(/*int grifc_id*/)
{
  char cmd[256]; sprintf(cmd," %d", 1); // sendto
}

void grifc_AcqStop(/*int grifc_id*/)
{
   char cmd[256]; sprintf(cmd," %d", 1); // sendto
}

void grifc_Reset(/*int grifc_id*/)
{ 
   char cmd[256]; sprintf(cmd," %d", 1); // sendto
}

#define TIMEOUT_COUNT 100000
void grifc_FlushEvents(int grifc_mask)
{
   //int i, j, count;
   printf("flushing data ...");
   fprintf(stdout,"done\n");
}

int grifc_NFrameRead(int grifc_id)
{
  char cmd[256]; sprintf(cmd," %d", 1); return(0); // sendto
}

int grifc_NFrameRead_B(int grifc_id)
{
   char cmd[256]; sprintf(cmd," %d", 1); return(0); // sendto
}
int grifc_NextFrame(int grifc_id)
{
   char cmd[256]; sprintf(cmd," %d", 1); return(0); // sendto
}
int grifc_NextFrame_B(int grifc_id)
{
   char cmd[256]; sprintf(cmd," %d", 1); return(0); // sendto
}

void grifc_set_64bit_transfer(int grifc_id, int value)
{
}

void grifc_SetTrigBox(int grifc_id, int boxnum)
{
}

void grifc_SetTrigPar(int grifc_id, int par_addr, int value)
{   
}

int grifc_ReadTrigPar(int grifc_id, int box, int par_addr){ return(0); }

int grifc_ReadStatus(int grifc_id, int addr){ return(0); }

#define FWTBUFSIZ 128
#define FWDATESTRLEN 13 /* length of formatted date string */
char *grifc_firmware_date(time_t val)
{
   static char fwtimebuf[FWTBUFSIZ];
   struct tm *tptr = gmtime(&val);
   if( (int)val != -1 ){
      strftime(fwtimebuf, FWTBUFSIZ, "%d%b%g_%H:%M", tptr);
   } else {
      sprintf(fwtimebuf, "-- Missing --");
   }
   return(fwtimebuf);
}

int grifc_ReadFirmwareId(int grifc_id){ return(0); }

int grifc_ParamWrite(int grifc_id, int port, int chan, int parmID, int parm)
{
   char cmd[256]; sprintf(cmd,"write_par 0xADDR 0xDATA"); return(0); // sendto
}

int grifc_ParamRead(int grifc_id, int port, int chan, int parmID)
{
   char cmd[256]; sprintf(cmd,"readback_parameter"); return(0); // sendto
}

int grifc_EventRead(int grifc_id, int *ev_buffer, int *ev_bufpos)
{ return(0); }

int grifc_EventRead_B(int grifc_id, int *ev_buffer, int *ev_bufpos)
{ return(0); }
