# Makefile
# $Id$

LIBS = -lm -lz -lutil -lpthread -lCAENDigitizer -lrt

DRV_DIR         = $(MIDASSYS)/drivers
INC_DIR         = $(MIDASSYS)/include
LIB_DIR         = $(MIDASSYS)/linux/lib

# MIDAS library
MIDASLIBS = $(LIB_DIR)/libmidas.a

# fix these for MacOS
UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
LIB_DIR   = $(MIDASSYS)/darwin/lib
MIDASLIBS = $(LIB_DIR)/libmidas.a
endif

OSFLAGS  = -DOS_LINUX -Dextname
CFLAGS   = -g -Wall -Wuninitialized -I$(INC_DIR) -I$(DRV_DIR) -I$(VMICHOME)/include -I.
CXXFLAGS = -std=c++0x $(CFLAGS) 

# use root if it's available
ifneq ($(ROOTSYS),)
	CXXFLAGS += -DUSE_TENV -I$(shell root-config --cflags)
	ROOTLIBS = $(shell root-config --libs)
endif

MODULES = $(LIB_DIR)/mfe.o

all: fecaen 

fecaen: $(MIDASLIBS) fecaen.o $(MODULES) CaenSettings.o CaenDigitizer.o
	$(CXX) -o $@ $(CXXFLAGS) $(OSFLAGS) $^ $(MIDASLIBS) $(ROOTLIBS) $(LIBS)

%: %.cc $(MIDASLIBS) CaenSettings.o
	$(CXX) -o $@ $(CXXFLAGS) $(OSFLAGS) $^ $(MIDASLIBS) $(ROOTLIBS) $(LIBS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

%.o: %.cxx %.hh
	$(CXX) $(CXXFLAGS) $(OSFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $(OSFLAGS) -c $<

clean::
	-rm -f *.o *.exe fecaen

# end
