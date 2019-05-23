DRV_DIR   = $(MIDASSYS)/drivers/vme
INC_DIR   = $(MIDASSYS)/include
LIB_DIR   = $(MIDASSYS)/linux/lib
#LIB_DIR   = $(MIDASSYS)/linux/lib
SRC_DIR   = $(MIDASSYS)/src

CC     = gcc
CFLAGS = -g -Wall -DOS_LINUX -Dextname
CFLAGS += -g -I$(DRV_DIR) -I. -I$(INC_DIR) -I$(DRV_DIR)/vmic -I$(VME_DIR)/include -L$(VME_DIR)/lib
LIBS = -lm -lz -lutil -lnsl -lpthread
LIB = $(LIB_DIR)/libmidas.a -lrt

MODULES = $(LIB_DIR)/mfe.o grifc.o odb_io.c

#all: fegrifdev
all: fegriffin

#fegrifdev: $(LIB) $(MODULES) fegrifdev.c
#	$(CC) $(CFLAGS) -o fegrifdev fegrifdev.c $(MODULES) $(LIB) $(LDFLAGS) $(LIBS)
fegriffin: $(LIB) $(MODULES) fegriffin.c
	$(CC) $(CFLAGS) -o fegriffin fegriffin.c $(MODULES) $(LIB) $(LDFLAGS) $(LIBS)

%.o: %.c grifc.h experim.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean::
	rm -f *.o *~ \#*
