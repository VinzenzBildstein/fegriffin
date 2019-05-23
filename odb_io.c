#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "midas.h"

#define ODB_STRING_SIZE 32
#define LINE_LENGTH     80 // for reasonable sized strings

//#############################################################################
//###############             Uniform coloring          #######################
//#############################################################################

#define WARN_COL "\033[33m"
#define ERR_COL  "\033[31m"
#define NORM_COL "\033[0m"

//#############################################################################
//###############             ODB/String Stuff          #######################
//#############################################################################

#define MAX_STRING_ARRAY_ELEM 256 // Changed by CU from 128 - Nov11
#define ODBBUF_SIZE           ODB_STRING_SIZE*MAX_STRING_ARRAY_ELEM
static char odbbuf[ODBBUF_SIZE], odbdata[ODBBUF_SIZE];

static void print_odb_key(KEY *key)
{
   fprintf(stdout,   "Name       : %s\n",     key->name           );
   fprintf(stdout,"   Type       : %d [%s]\n",(int)key->type, rpc_tid_name(key->type) );
   fprintf(stdout,"   Num Values : %d\n",     key->num_values     );
   fprintf(stdout,"   Item Size  : %d\n",     key->item_size      );
   fprintf(stdout,"   Total Size : %d\n",     key->total_size     );
   fprintf(stdout,"   Data Addr  : 0x%08x\n", key->data           );
   fprintf(stdout,"   Access Mode: %d\n",     key->access_mode    );
   fprintf(stdout,"   NotifyCount: %d\n",     key->notify_count   );
   fprintf(stdout,"   NextKey    : %d\n",     key->next_key       );
   fprintf(stdout,"   ParentList : %d\n",     key->parent_keylist );
   fprintf(stdout,"   LastWritten: %d\n",     key->last_written   );
   fprintf(stdout,"\n");
}

static int read_odb_value(HNDLE h_Odb, HNDLE h, char *name, void *data, int *size, int type)
{
   KEY key, subkey;
   HNDLE hSubkey;
   int stat;

   db_get_key(h_Odb, h, &key);
   if( db_find_key(h_Odb, h, name, &hSubkey) != DB_SUCCESS){
      fprintf(stderr,"Can't find odb Entry %s in %s\n", name, key.name );
      print_odb_key(&key); return(-1);
   }
   db_get_key(h_Odb, hSubkey, &subkey); //print_odb_key(&key);
   if( subkey.total_size > *size ){
      fprintf(stderr,"Supplied buffer [%d] too small for Key %s/%s\n",
         *size, key.name, name );
      print_odb_key(&subkey); return(-1);
   }
   if( (stat=db_get_value(h_Odb,h, name, data, size, type, 0)) != DB_SUCCESS ){
     fprintf(stderr,"Error %d: reading Key Value %s/%s\n", stat,key.name,name);
     print_odb_key(&subkey); return(-1);
   }
   if( *size > subkey.total_size || *size == 0 ){
      static int done;
      if( ! done ){ /* only show this error once to remind us */
         done = 1;
         fprintf(stderr,"db_get_value gave wrong size:%d instead of ", *size);
         fprintf(stderr,"%d, for key %s/%s\n",subkey.total_size,key.name,name);
      }
      *size = subkey.total_size;
   }
   return(0);
}

/* odb string arrays contain old junk after end of each string in the array  */
/* this invalid data inbetween each good region makes processing awkward ... */
/* so copy only the valid data at start of each string ->odbdata, + use this */
/* keep result as single string with newlines separating original elements   */
static int read_odb_strings(HNDLE h_Odb, HNDLE h, char *name)
{
   int i, j, size = ODBBUF_SIZE;
   char *ptr=odbdata;

   *ptr='\0';
   if( read_odb_value(h_Odb, h, name, odbbuf,&size,TID_STRING) ){ return(-1); }
   for(i=0; i<size; i+=ODB_STRING_SIZE){                /* loop over strings */
             /* put newlines btween strings (but don't insert one at start!) */
      if( ptr != odbdata ){ *(ptr++) = '\n'; }
      for(j=0; j<ODB_STRING_SIZE; j++){            /* loop within string ... */
         if( (*(ptr++) = odbbuf[i+j]) == '\0' ){ --ptr; break; }/*copyToNull */
      }
   }
   return(0);
}

/* input string contains name:value pairs - extract these ...      */
/* return number characters used or -1 on error                    */
/*    zero length              : return zero                       */
/*    [space] to end           : return >0, both strings empty     */
/*    [space]name[:][space]    : return >0, 2nd string empty       */
/*       name could be missing => 1st string also empty            */
/*    [space]name:value[space] : return >0, both strings notempty  */
/*     *:*:*                   : return -1                         */

/* since names can contain spaces ... and strings are also separated */
/* by spaces, a pair of strings: "name1" (without value) followed by */
/* "name2:value", would be stored as ... name1[spaces]name2:val and  */
/* would be treated as a single string consisting of  a single name  */
/* with embedded spaces followed by value. To avoid this, it's       */
/* necessary to retain the end of the original strings  - the end of */
/* strings are saved as newlines.  i.e. Newlines separate strings -  */
/* stop at each of these or string end */
static char pair_name[ODB_STRING_SIZE], pair_value[ODB_STRING_SIZE];
int next_pair(char *input)
{
   char *ptr = pair_name;
   int i;

   memset(pair_name,  0, ODB_STRING_SIZE);
   memset(pair_value, 0, ODB_STRING_SIZE);
   for(i=0; i<strlen(input); i++){ /* first skip leading spaces/tabs */
      if( input[i] == '\n' ){ return(i+1); }
      if( input[i] != ' ' && input[i] != '\t' ){ break; }
   }
   for(;i<ODB_STRING_SIZE && i<strlen(input); i++){
      if( input[i] == '\n' ){ return(i+1); }
      if( input[i] == ' ' || input[i] == '\t' ){ 
	 if( strlen(pair_value) != 0 ){ break; } /* end at space if on value */
      }
      if( input[i] == ':' ){
	 if( strlen(pair_value) != 0 ){
	    fprintf(stderr,"Second colon found in pair:value [%s]\n", input );
	    return(-1);
	 }
	 ptr = pair_value; continue;
      }
      *(ptr++) = input[i];
   }
   for(; i<strlen(input); i++){ /* skip any trailing spaces/tabs */
      if( input[i] == '\n' ){ return(i+1); }
      if( input[i] != ' ' && input[i] != '\t' ){ break; }
   }
   return(i);
}

//#############################################################################
//###############            Filter Settings          #########################
//#############################################################################

#define MAX_FILTERS     64 // can be as much as we want
#define MAX_DETTYPE     14
#define MAX_FILTER_COND 14

typedef struct filter_condition_struct {
   int           window;   int clover;   int suppress;
   int      subsys_mult[MAX_DETTYPE];
   int  subsys_prescale[MAX_DETTYPE];
} Filter_condition;

typedef struct filter_setting_struct {
   int             mask;
   char                  name[ODB_STRING_SIZE];
  Filter_condition conditions[MAX_FILTER_COND];
} Filter_setting;

Filter_setting filter_settings[MAX_FILTERS];

void init_filter_settings(Filter_setting *ptr);
int check_filter_settings(Filter_setting *ptr);
int read_filters(HNDLE hDB, HNDLE hSet);
int read_single_filter(HNDLE hDB, HNDLE hKey, Filter_setting *ptr);
int read_filter_condition(Filter_condition *ptr, char *buf);
int read_enabled_mask(int *mask, char *buf);

// Odb Filter directory consists of a filter string which selects one of
// a series of named subdirectories 
// Each subdirectory has a list of Filter conditions
void init_filter_settings(Filter_setting *ptr)
{
   memset(ptr, -1, sizeof(Filter_setting) );
   memset(ptr->name, 0, ODB_STRING_SIZE);
}

/* Should also check values within allowed range  */
int check_filter_settings(Filter_setting *ptr)
{
   int err=0; 
   if(strlen(ptr->name)  == 0) {err=1; fprintf(stderr,"NULL Name\n"); }
   /* check all other settings for -1 => not set */
   return(err);
}

// this isn't used at the moment - we don't read in all filters
//                                (and later choose one to apply)
// we just read current filter and load that one
int read_filters(HNDLE hDB, HNDLE hSet)
{
   Filter_setting *ptr;
   HNDLE hKey, hSubkey;
   int i, filter;
   KEY   key;

   /* Read Filter settings from odb and store */
   if( db_find_key (hDB, hSet, "Filter", &hKey) != DB_SUCCESS){
      cm_msg(MINFO,"FE","Key %s not found", "Filter");
      return(-1);
   }
   filter = 0;
   for(i=0;;i++){ // loop over subdirs [type=key]
      ptr = &filter_settings[filter];
      if( db_enum_key (hDB, hKey, i, &hSubkey) != DB_SUCCESS ){ break; }
      db_get_key(hDB, hSubkey, &key); //print_odb_key(&key);
      if( key.type != TID_KEY ){
         fprintf(stdout,"Ignored [%16s] in Filter Settings\n", key.name );
         continue;
      }
      fprintf(stdout,"Reading Filter Settings: %16s ... ", key.name );
      init_filter_settings(ptr);
      strncpy(ptr->name, key.name, 32);
      read_single_filter(hDB, hSubkey, ptr);
      if( check_filter_settings(ptr) ){
	 printf("%s *** Problems ***\n%s", ERR_COL, NORM_COL);
      } else { fprintf(stdout,"     Done\n"); }
      ++filter;
   }
   filter_settings[filter].name[0] = 0;/* key not found-truncate list here */
   return(0);
}

#define NUM_SUBSYSTEMS 14
static char subsys_names[NUM_SUBSYSTEMS][ODB_STRING_SIZE] = {
  "GRGa", "GRGb", "SEP",  "DAN",  "DAT",  "PAC", "DSC", "GRS",
  "DAS",  "SET",  "DSCg", "DSCn", "CLOV", "SUPN"
};
//   "UN", "GR", "SE", "PA", "DS", "DA", "TI"
// each condition is array of subsys - sing/coinc/prscl - mult/ps - win2
//   each condition is AND of array elements - satisfy all of single condition
//   but overall is OR of #conditions can satisfy any full condition
//   each condition: %s-%c-%d [-%d]

int read_single_filter(HNDLE hDB, HNDLE hKey, Filter_setting *ptr)
{
  int i, j, status, size=4; //, strsize=ODB_STRING_SIZE;
   //char mask_str[ODB_STRING_SIZE];
   void *value=NULL;
   HNDLE hSubkey;
   KEY key;

   for(i=0;;i++){ // loop over all entries in directory
      if( db_enum_key (hDB, hKey, i, &hSubkey) != DB_SUCCESS ){ break; }
      db_get_key(hDB, hSubkey, &key); //print_odb_key(&key);

      if( sscanf(key.name,"coincWindow%d", &j) == 1 ){
	 if( j < 0 || j >= MAX_FILTER_COND ){
	    fprintf(stderr,"filter condition%d out of range\n", j);
	    continue;
	 }
	 if( ptr->conditions[j].window != -1 ){
	    fprintf(stderr,"repeated coincwin%d\n",j); continue;
	 }
	 if( key.type > 7 ){ // byte sbyte char word short dword int
	    fprintf(stderr,"coincwin%d wrong type:%d\n", j, key.type);
	    continue;
	 }
	 value = &ptr->conditions[j].window;
	 status = db_get_value(hDB, hKey, key.name, value, &size, TID_INT, 0);
	 if( status != DB_SUCCESS ){
	    fprintf(stderr,"coincwin%d can't read value\n", j);
	 }
         continue;
      }
      else if( sscanf(key.name,"orCondition%d", &j) == 1 ){
	 if( j < 0 || j >= MAX_FILTER_COND ){
	    fprintf(stderr,"filter condition%d out of range\n", j);
	    continue;
	 }
         read_odb_strings(hDB, hKey, key.name);
	 read_filter_condition(&ptr->conditions[j], odbdata);
	 continue;
      }
      else if( strcmp(key.name,"EnabledDetTypes") == 0 ){
	 if( key.type != 12 ){ //str=12, byte sbyte chr wrd shrt dwrd int=1-7
	    fprintf(stderr,"Enabled Types wrong type:%d\n", key.type);
	    continue;
	 }
       // // this block was for single-string masks 
       //value = mask_str;
       //status = db_get_value(hDB,hKey,key.name,value,&strsize,TID_STRING,0);
       //if( status != DB_SUCCESS ){
       //    fprintf(stderr,"Enabled Types: can't read value\n");
       //}
       //if( decode_mask_v1(mask_str, &ptr->mask) ){
       //   fprintf(stderr,"Enabled Types: can't decode value:%s\n", mask_str);
       //}
	 read_odb_strings(hDB, hKey, key.name);
         read_enabled_mask(&ptr->mask, odbdata);
         continue;
      } else {
	 fprintf(stderr,"Unknown filter entry:%s\n", key.name );
	 continue;
      }
   }
   return(0);
}

// use name:value parser - expect value to be empty
// each name is of form %s-%c-%d [-%d]
static char  subsys[ODB_STRING_SIZE];
static char subtype[ODB_STRING_SIZE];
int read_filter_condition(Filter_condition *ptr, char *buf)
{
  int i, num, bytes, val1, val2, multwin=-1;

   while( 1 ){
      if( (bytes=next_pair(buf)) == -1 ){ return(-1); }   /* Format Error */
      if( bytes == 0 ){ break; }                           /* End of data */
      buf += bytes;
      if( strlen(pair_name)==0 && strlen(pair_value)==0 ){ /* empty string */
	 continue;
      }
      if( strlen(pair_name ) == 0  ){
	 fprintf(stderr,"blank item in [%s]\n", buf-bytes );
	 continue;
      }
      num = sscanf(pair_name,"%[A-Za-z]-%[SCP]-%d-%d",
         subsys, subtype, &val1, &val2);
      if( num < 3 ){
	 fprintf(stderr,"can't read filter entry:%s\n", pair_name );
      }
      for(i=0; i<NUM_SUBSYSTEMS; i++){
         if(!strncasecmp(subsys_names[i], subsys, strlen(subsys_names[i]))){ break; }
      }
      if( i == NUM_SUBSYSTEMS ){
	 fprintf(stderr,"can't read subsystem in entry:%s\n", pair_name );
      }


//########################
      // extra quick modification for clover fold and suppressed mult.
      if(      i == NUM_SUBSYSTEMS-1 ){
	 ptr->suppress   = 1; continue; //  suppress flag
      } else if( i == NUM_SUBSYSTEMS-2 ){
         ptr->clover = 1; continue; // clover mult flag
      }
//########################



      if( strlen(subtype) != 1 ){
	 fprintf(stderr,"can't read entry-type in entry:%s\n", pair_name );
      }
      if( subtype[0] == 'S' ){ // S always comes with 1 ?
         if( ptr->subsys_mult[i] != -1 ){
	    fprintf(stderr,"duplicate mult:%s\n", pair_name );
         } else if( val1 != 1 ){
	    fprintf(stderr,"singles multiplicity not 1 in:%s\n", pair_name );
         } else {
	    //fprintf(stderr,"singles multiplicity good\n");
	    ptr->subsys_mult[i] = 1;
	 }
      }
      if( subtype[0] == 'C' ){
         if( ptr->subsys_mult[i] != -1 ){
	    fprintf(stderr,"duplicate mult:%s\n", pair_name );
         } else {
	    ptr->subsys_mult[i] = val1;
	 }
      }
      if( subtype[0] == 'P' ){
         if( ptr->subsys_prescale[i] != -1 ){
	    fprintf(stderr,"duplicate prescale:%s\n", pair_name );
         } else if( ptr->subsys_mult[i] != -1 ){
	    fprintf(stderr,"prescale set as well as mult:%s\n", pair_name );
	 } else {
	    //fprintf(stderr,"setting prescale\n");
	    ptr->subsys_mult[i]     =    1; // prescale only works on mult=1
	    ptr->subsys_prescale[i] = val1;
	 }
      }
      if( num == 4 ){ // additional multiplicity window given
         if(  subtype[0] != 'C' ){
            fprintf(stderr,"coincwin without coincflag in:%s\n", pair_name );
	 } else if( multwin != -1 ){
            fprintf(stderr,"duplicate coincwindow in:%s\n", pair_name );
         } else {
	    multwin = val2;
	 }
      }
   }
   return(0);
}

// use name:value parser - expect value to be empty
// each name is one of subsystem types 
int read_enabled_mask(int *mask, char *buf)
{
   int i, bytes;
   char *ptr;

   *mask = 0;
   while( 1 ){
      if( (bytes=next_pair(buf)) == -1 ){ return(-1); }   /* Format Error */
      if( bytes == 0 ){ break; }                          /* End of data */
      buf += bytes;
      if( strlen(pair_name)==0 && strlen(pair_value)==0 ){ /* empty string */
         continue;
      }
      if( strlen(pair_name ) == 0  ){
         fprintf(stderr,"blank item in [%s]\n", buf-bytes );
         continue;
      }
      for(i=0; i<NUM_SUBSYSTEMS; i++){ ptr = subsys_names[i];
 	 if( strlen(ptr) != strlen(pair_name) ){ continue; }
	 if( !strncasecmp(ptr, pair_name, strlen(ptr)) ){ break; }
      }
      if( i == NUM_SUBSYSTEMS ){
	 fprintf(stderr,"can't read subsystem in entry:%s\n", pair_name );
      } else {
	*mask |= (1<<(15-i)); // mask is backwards in hardware ??
      }
   }
   if( *mask == 0 ){
      fprintf(stderr,"Enabled types: empty! => no data\n");
   }
   return(0);
}

// decode_mask_v1 was for single-string masks of the form "ge-sep-pac"
int decode_mask_v1(char *data, int *mask)
{
   char *ptr, subsys[ODB_STRING_SIZE];
   int i, j;
   *mask = 0;
   i = 0;
   // loop over subsystems separated by "-"
   while(i<strlen(data) && i<ODB_STRING_SIZE){
      ptr = subsys; memset(subsys, 0, ODB_STRING_SIZE);
      for(   ; i<strlen(data) && i<ODB_STRING_SIZE; i++){
	 if( data[i] == ' ' || data[i] == '\t' ){ continue; } // ignore space
         if( data[i] == '-' ){ ++i; break; }
         *(ptr++) = data[i];
      }
      for(j=0; j<NUM_SUBSYSTEMS; j++){ ptr = subsys_names[j]; // reuse ptr
         if( !strncasecmp(ptr, subsys, strlen(ptr)) ){ break; }
      }
      if( j == NUM_SUBSYSTEMS ){
	  fprintf(stderr,"can't read subsystem in entry:%s\n", subsys );
      } else {
         *mask |= (1<<j);
      }
   }
   if( *mask == 0 ){
      fprintf(stderr,"Enabled types: empty! => no data\n");
   }
   return(0);
}

// to write [pick single filter] + only allowed 8 [64bit] conditions
// each condition is 2 x 32bits - 8 bits * 7 dettypes + 8 prescale on/off
//    there is single 16bit prescale per subsys [8] -> 4x32bit prescales
// each subsys is either present with mult. - fill in, or prescale - set flag
//    set relevant prescale count at same time as flag
int get_filter_prog(HNDLE hDB, HNDLE hSet, int *filter_par, int *prescales, int *wsize, int *mask)
{  // filter directory contains string "current" and dir "Filters"
  int i, j, k, mult, prescale, condition_active;
   char current[ODB_STRING_SIZE];
   Filter_setting *ptr;
   HNDLE hKey, hSubkey;
   KEY key;

   // filter_par and prescales are arrays to hold final program
   // they are related to #conditions and dettypes but separate and have
   // their own sizes ... see fegriffin.c:NUM_FILTERPAR,NUM_PRESCALES
   memset(filter_par, 0, 64*sizeof(int) );
   memset(prescales,   0, 8*sizeof(int) );

   if( db_find_key (hDB, hSet, "Filter", &hKey) != DB_SUCCESS){
      cm_msg(MINFO,"FE","Key %s not found", "Filter");
      return(-1);
   }
   if( read_odb_strings(hDB, hKey, "Current") ){ return(-1); }
   strncpy(current, odbdata, ODB_STRING_SIZE);

   if( db_find_key (hDB, hSet, "Filter/Filters", &hKey) != DB_SUCCESS){
      cm_msg(MINFO,"FE","Key %s not found", "Filter/Filters");
      return(-1);
   }
   for(i=0;;i++){ // loop over all entries in directory
      if( db_enum_key (hDB, hKey, i, &hSubkey) != DB_SUCCESS ){ break; }
      db_get_key(hDB, hSubkey, &key); //print_odb_key(&key);
      if( key.type != TID_KEY ){
         fprintf(stdout,"Ignored [%16s] in Filters list\n", key.name );
         continue;
      }
      if( strcmp(current, key.name) != 0 ){ continue; }
      ptr = &filter_settings[0];
      init_filter_settings(ptr);
      strncpy(ptr->name, key.name, ODB_STRING_SIZE);
      fprintf(stdout,"   reading filter: %s\n", key.name);
      read_single_filter(hDB, hSubkey, ptr);
      *mask = ptr->mask;
      *wsize = -1;
      for(j=0; j<MAX_FILTER_COND; j++){ condition_active = 0;
         for(k=0; k<MAX_DETTYPE; k++){
	    mult    =ptr->conditions[j].subsys_mult[k];
	    prescale=ptr->conditions[j].subsys_prescale[k];
	    if( (mult != -1) || (prescale != -1) ){
	       if( !condition_active ){
                  fprintf(stdout,"      Condition: %d -", j );
	          condition_active = 1;
	       }
               fprintf(stdout,"  Dtype%d", k );
               if( mult != -1 ){
	          fprintf(stdout," x%d", mult );
	          filter_par[4*j + (int)(k/4)] |= (mult << (8*(k%4)));
	       }
               if( prescale != -1 ){
                  fprintf(stdout," [Prscx%d]", prescale );
	          prescales[k/2] |= ( (prescale & 0xffff) << (16*(k%2)) );
	          filter_par[4*j + 3] |= (1 << (16+k)); // set flag
	       }
               if( *wsize == -1 ){
                  *wsize = ptr->conditions[j].window;
                  fprintf(stdout,"  Window:%d", *wsize );
               }
	    }
	 }
         if( ptr->conditions[j].clover != -1  ){
            filter_par[4*j + 3] |= (1<<30); // set flag
            fprintf(stdout,"  Clover Ge Mult");
	 }
         if( ptr->conditions[j].suppress != -1 ){
            filter_par[4*j + 3] |= (1<<31); // set flag
            fprintf(stdout,"  Suppressed Ge Mult");
	 }
         if( condition_active ){ printf("\n"); }
      }
      return(0);
   }
   cm_msg(MINFO,"FE","Didn't find filter %s in odb", current);
   return(-1);
}

//#############################################################################
//###############              PPG program            #########################
//#############################################################################

#define STRING_LEN     256
#define NUM_PPG_PATTERN 10
int get_ppg_program(HNDLE hDB, HNDLE hSet, int *ppg_codes, int *ppg_durations)
{
   char tmp[STRING_LEN], cycle_name[STRING_LEN];
   HNDLE hSubkey, hKey, hCycle;
   int i, status, size;
   KEY key;

   memset(ppg_codes,0,sizeof(int)*NUM_PPG_PATTERN);
   memset(ppg_durations,0,sizeof(int)*NUM_PPG_PATTERN);

   sprintf(tmp,"/PPG/Current");
   if( (status=db_find_key(hDB, 0, tmp, &hSubkey)) != DB_SUCCESS){
      cm_msg(MINFO,"FE","Key %s not found", tmp); return(-1);
   }
   size=sizeof(cycle_name);
   if( (db_get_data(hDB,hSubkey,cycle_name,&size,TID_STRING)) != DB_SUCCESS){
       cm_msg(MINFO,"FE","Can't get data for Key %s", tmp); return(-1);
   }
   sprintf(tmp,"/PPG/Cycles");
   if( (status=db_find_key(hDB, 0, tmp, &hKey)) != DB_SUCCESS){
       cm_msg(MINFO,"FE","Key %s not found", tmp); return(-1);
   }
   hCycle=0;
   for(i=0;;i++){
      if( db_enum_key (hDB, hKey, i, &hSubkey) != DB_SUCCESS ){ break; }
      db_get_key(hDB, hSubkey, &key);
      if( key.type != TID_KEY ){
         fprintf(stdout,"Ignored [%16s] in /PPG/Cycles\n", key.name );
         continue;
      }
      size = strlen(cycle_name);
      if( size < strlen(key.name) ){ size = strlen(key.name); }
      if( size > STRING_LEN ){ size = STRING_LEN; } 
      if( strncmp(cycle_name, key.name, size) != 0 ){ continue; }
      hCycle=hSubkey; break;
   }
   if( hCycle == 0 ){
      cm_msg(MINFO,"FE","Key %s/%s not found", tmp, cycle_name); return(-1);
   }
   sprintf(tmp,"/PPG/Cycles/%s/PPGcodes", cycle_name);
   if( (status=db_find_key(hDB, 0, tmp, &hKey)) != DB_SUCCESS){
     cm_msg(MINFO,"FE","Key %s not found", tmp); return(-1);
   }
   size = NUM_PPG_PATTERN * sizeof(int);
   if( (db_get_data(hDB, hKey, ppg_codes, &size, TID_INT)) != DB_SUCCESS){
      cm_msg(MINFO,"FE","Can't get data for Key %s", tmp); return(-1);
   }
   for(i=size/4; i<NUM_PPG_PATTERN; i++){ ppg_codes[i] = 0; } // clear unused
   sprintf(tmp,"/PPG/Cycles/%s/durations", cycle_name);
   if( (status=db_find_key(hDB, 0, tmp, &hKey)) != DB_SUCCESS){
      cm_msg(MINFO,"FE","Key %s not found", tmp); return(-1);
   }
   size = NUM_PPG_PATTERN * sizeof(int);
   if( (db_get_data(hDB, hKey, ppg_durations, &size, TID_INT)) != DB_SUCCESS){
      cm_msg(MINFO,"FE","Can't get data for Key %s", tmp); return(-1);
   }
   for(i=size/4; i<NUM_PPG_PATTERN; i++){ ppg_durations[i]=0;} // clear unused
   return(0);
}
