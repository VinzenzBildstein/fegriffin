///////////////////////////////////////////////////////////////////////////
//////////////////////       midas-mfe stuff       ////////////////////////

int main(int argc, char *argv[])
{
   INT status, i, j, size;
   INT daemon_flag;
   int sys_max_event_size = MAX_EVENT_SIZE;

   host_name[0] = 0; exp_name[0] = 0;
   debug = FALSE;    daemon_flag = 0;
   // setbuf(stdout, 0); setbuf(stderr, 0);
   // signal(SIGPIPE, SIG_IGN); ifdef SIGPIPE - ignore it

   cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

   _argc = argc; /* store arguments for user use */
   _argv = (char **)malloc(sizeof(char *)*argc);
   for (i=0 ; i<argc ; i++) { _argv[i] = argv[i]; }

   /* parse command line parameters */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'd')
         debug = TRUE;
      else if (argv[i][0] == '-' && argv[i][1] == 'D')
         daemon_flag = 1;
      else if (argv[i][0] == '-' && argv[i][1] == 'O')
         daemon_flag = 2;
      else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'e')
            strcpy(exp_name, argv[++i]);
         else if (argv[i][1] == 'h')
            strcpy(host_name, argv[++i]);
         else if (argv[i][1] == 'i')
            frontend_index = atoi(argv[++i]);
         else if (argv[i][1] == '-') {
          usage:
            printf
                ("usage: frontend [-h Hostname] [-e Experiment] [-d] [-D] [-O] [-i n]\n");
            printf("         [-d]     Used to debug the frontend\n");
            printf("         [-D]     Become a daemon\n");
            printf("         [-O]     Become a daemon but keep stdout\n");
            printf("         [-i n]   Set frontend index (used for event building)\n");
            return 0;
         }
      }
   }
   if (event_buffer_size < 2 * max_event_size) {/* check event/buffer size */
      cm_msg(MERROR, "mainFE", "event_buffer_size %d too small for max. event size %d\n", event_buffer_size, max_event_size);
      ss_sleep(5000);
      return 1;
   }
   /* retrieve frontend index from environment if defined */
   if (getenv("MIDAS_FRONTEND_INDEX"))
      frontend_index = atoi(getenv("MIDAS_FRONTEND_INDEX"));

   /* add frontend index to frontend name if present */
   strcpy(full_frontend_name, frontend_name);
   if (frontend_index >= 0)
      sprintf(full_frontend_name + strlen(full_frontend_name), "%02d", frontend_index);

   /* inform user of settings */
   printf("Frontend name          :     %s\n", full_frontend_name);
   printf("Event buffer size      :     %d\n", event_buffer_size);
   printf("User max event size    :     %d\n", max_event_size);
   if (max_event_size_frag > 0)
      printf("User max frag. size    :     %d\n", max_event_size_frag);
   printf("# of events per buffer :     %d\n\n", event_buffer_size / max_event_size);

   if (daemon_flag) {
      printf("\nBecoming a daemon...\n");
      ss_daemon_init(daemon_flag == 2); // libmidas.a
   }
   set_rate_period(3000); /* set default rate period */
   if (display_period) {  /* now connect to server */
      if (host_name[0]) {
         if (exp_name[0])
            printf("Connect to experiment %s on host %s...\n", exp_name, host_name);
         else
            printf("Connect to experiment on host %s...\n", host_name);
      } else if (exp_name[0])
         printf("Connect to experiment %s...\n", exp_name);
      else
         printf("Connect to experiment...\n");
   }
   status = cm_connect_experiment1(host_name, exp_name, full_frontend_name,
                                   NULL, DEFAULT_ODB_SIZE, DEFAULT_FE_TIMEOUT);
   if (status != CM_SUCCESS) {
      cm_msg(MERROR, "mainFE", "Cannot connect to experiment \'%s\' on host \'%s\', status %d", exp_name, host_name, status);
      ss_sleep(5000); /* let user read message before window might close */
      return 1;
   }
   if (display_period) printf("OK\n");
   event_buffer = malloc(max_event_size); /* allocate buffer space */
   if (event_buffer == NULL) {
      cm_msg(MERROR, "mainFE", "mfe: Cannot allocate event buffer of max_event_size %d\n", max_event_size);
      return 1;
   }
   cm_cleanup(full_frontend_name, FALSE); /* remomve any dead frontend */
   status = cm_shutdown(full_frontend_name, FALSE);/* shutdown prev frontend */
   if (status == CM_SUCCESS && display_period) {
      printf("Previous frontend stopped\n");
      ss_sleep(3000); /* let user read message */
   }
   /* register transition callbacks */
   if( cm_register_transition(TR_START,  tr_start,  500) != CM_SUCCESS ||
       cm_register_transition(TR_STOP,   tr_stop,   500) != CM_SUCCESS ||
       cm_register_transition(TR_PAUSE,  tr_pause,  500) != CM_SUCCESS ||
       cm_register_transition(TR_RESUME, tr_resume, 500) != CM_SUCCESS ){
      printf("Failed to start local RPC server");
      cm_disconnect_experiment();
      ss_sleep(5000);  /* let user read message before window might close */
      return 1;
   }
   cm_set_run_state(run_state);
   cm_get_experiment_database(&hDB, &status);
   size = sizeof(sys_max_event_size);
   status = db_get_value(hDB, 0, "/Experiment/MAX_EVENT_SIZE", &sys_max_event_size, &size, TID_DWORD, TRUE);

   if (max_event_size > sys_max_event_size) {
      cm_msg(MERROR, "mainFE", "Requested max_event_size (%d) exceeds max. system event size (%d)",
             max_event_size, sys_max_event_size);
      return 1;
   }
   if (debug) cm_set_watchdog_params(FALSE, 0); /* debug mode => no watchdog */

   /* increase RPC timeout to 2min for logger with exabyte or blocked disk */
   rpc_set_option(-1, RPC_OTIMEOUT, 120000);

   /* set own message print function */
   if (display_period) cm_set_msg_print(MT_ALL, MT_ALL, message_print);

   /* reqister equipment in ODB */
   if (register_equipment() != SUCCESS) {
      if (display_period) printf("\n");
      cm_disconnect_experiment();
      ss_sleep(5000);/* let user read message before window might close */
      return 1;
   }
   if (display_period) printf("Init hardware...\n");  /* call user init fn */
   if (frontend_init() != SUCCESS) {
      if (display_period) printf("\n");
      cm_disconnect_experiment();
      ss_sleep(5000); /* let user read message before window might close */
      return 1;
   }
   initialize_equipment();
   if (display_period) printf("OK\n");
   if (display_period) { /* initialize screen display */
      ss_sleep(300);  display(TRUE);
   }
   /* switch on interrupts or readout thread if running */
   if (run_state == STATE_RUNNING) readout_enable(TRUE);
   ss_getchar(0);         /* initialize ss_getchar */


   status = scheduler();  /* call main scheduler loop */


   stop_readout_threads();/* stop readout thread */
   rb_set_nonblocking();
   while (is_readout_thread_active()) ss_sleep(100);
   ss_getchar(TRUE);  /* reset terminal */
   if (interrupt_eq) {/* switch off interrupts and detach */
      interrupt_configure(CMD_INTERRUPT_DISABLE, 0, 0);
      interrupt_configure(CMD_INTERRUPT_DETACH, 0, 0);
   }
   frontend_exit(); /* call user exit function */
   for (i = 0; equipment[i].name[0]; i++)  /* close slow control drivers */
      if ((equipment[i].info.eq_type & EQ_SLOW) && equipment[i].status == FE_SUCCESS) {
         for (j = 0; equipment[i].driver[j].name[0]; j++)
            if (equipment[i].driver[j].flags & DF_MULTITHREAD)
               break;
         /* stop all threads if multithreaded */
         if (equipment[i].driver[j].name[0] && equipment[i].status == FE_SUCCESS)
            equipment[i].cd(CMD_STOP, &equipment[i]);   /* stop all threads */
      }
   for (i = 0; equipment[i].name[0]; i++)
      if ((equipment[i].info.eq_type & EQ_SLOW) && equipment[i].status == FE_SUCCESS)
         equipment[i].cd(CMD_EXIT, &equipment[i]);      /* close physical connections */
   free(n_events);
   cm_disconnect_experiment();/* close network connection to server */
   if (display_period) {
      if (status == RPC_SHUTDOWN) {
         ss_clear_screen();
         ss_printf(0, 0, "Frontend shut down.");
         ss_printf(0, 1, "");
      }
   }
   if (status != RPC_SHUTDOWN) printf("Network connection aborted.\n");
   return 0;
}
