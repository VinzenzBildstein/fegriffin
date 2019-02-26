# Makefile
# $Id$

LIBS = -lm -lz -lutil -lpthread -lCAENDigitizer -lrt

DRV_DIR         = $(MIDASSYS)/drivers
INC_DIR         = $(MIDASSYS)/include
LIB_DIR         = $(MIDASSYS)/lib

# MIDAS library
MIDASLIBS = $(LIB_DIR)/libmidas.a

# fix these for MacOS
UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
LIB_DIR   = $(MIDASSYS)/darwin/lib
MIDASLIBS = $(LIB_DIR)/libmidas.a
endif

OSFLAGS  = -DOS_LINUX -Dextname
CFLAGS   = -g -Wall -Wuninitialized -I$(INC_DIR) -I$(DRV_DIR) -I$(VMICHOME)/include
CXXFLAGS = $(CFLAGS) 

MODULES = $(LIB_DIR)/mfe.o grifc.o odb_io.o

all: fegriffin 

#fegriffin: $(LIB) $(MODULES) fegriffin.c CaenSettings.o CaenDigitizer.o
	#$(CXX) $(CXXFLAGS) -o fegriffin fegriffin.c $(MODULES) $(LIB) $(LDFLAGS) $(LIBS)
fegriffin: $(MIDASLIBS) fegriffin.o $(MODULES) CaenSettings.o CaenDigitizer.o
	$(CXX) -o $@ $(CXXFLAGS) $(OSFLAGS) $^ $(MIDASLIBS) $(ROOTLIBS) $(LIBS)

%: %.cc $(MIDASLIBS) CaenSettings.o
	$(CXX) -o $@ $(CXXFLAGS) $(OSFLAGS) $^ $(MIDASLIBS) $(ROOTLIBS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $(OSFLAGS) -c $<

clean::
	-rm -f *.o *.exe

# end
