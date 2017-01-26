#-------------------------------------------------------------------
# Generic rules to be used by each package
# Makefile.in should define CPPSRCS, CPPHDRS, PACKAGE, SUBPACKAGES
# and SUBPACKOBJS then include this file (Note: SUBPACKAGES can be empty).
# Additional rules can be added after the include statement
# ------------------------------------------------------------------

.SUFFIXES:
.SUFFIXES: .C .cc .o .h .a

SHELL = /bin/sh

# *******************************************************************
#          Configurable Section - see configure.in
# *******************************************************************

# --------- Compilers, linkers, librarians and others --------------
CC          = gcc 
CCC	    = g++

LD	    = ld 
SHARED_LD   = g++
AR	    = ar
RM	    = rm
RANLIB	    = ranlib
LINK	    = ln -s
MAKE	    = make
MAKE_DEPEND = makedepend
INSTALL	    = /usr/bin/install -c

ARCH        = pc

# ------------ Directories -----------------------------------------
MINI_POLARIS_DIR = /pub/courses/cpsc605/minipolaris
MINI_POLARIS_WORK    = $(MINI_POLARIS_DIR)/cvdl
MINI_POLARIS_SHARED_LIB_DIR = $(MINI_POLARIS_WORK)/shared_libs
MINI_POLARIS_LOCAL = $(HOME)/minipolaris
LOCAL_SHARED_LIB_DIR = $(MINI_POLARIS_LOCAL)/shared_libs

INCLUDES    = -I$(MINI_POLARIS_WORK) -I$(MINI_POLARIS_WORK)/base \
	      -I$(MINI_POLARIS_DIR)/include -I$(MINI_POLARIS_LOCAL)


# -------- Flags ---------------------------------------------------
CFLAGS	= -Wno-write-strings -O2 -Wall -Wno-comment  -DMINI_POLARIS_GNU_PRAGMAS=1  $(EXTRAFLAGS) $(INCLUDES)
CCFLAGS = -Wno-write-strings -O2 -fPIC -fno-operator-names -Wall -Wno-comment -Wno-switch -Wno-deprecated  -DMINI_POLARIS_GNU_PRAGMAS=1  $(EXTRAFLAGS) $(INCLUDES)
SHARED_LDFLAGS = -shared
LDFLAGS = -r
LINKFLAGS =  
ARFLAGS = ruv
IFLAGS = -m 444

# ------- Macros ---------------------------------------------------
CPPOBJS = $(CPPSRCS:.cc=.o)

# ------- Implicit Rules -------------------------------------------

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
.cc.o:
	$(CCC) $(CCFLAGS) -c -o $@ $<

.C.o:
	time $(CCC) $(CCFLAGS) -c -o $@ $<

# ----- Explicit Rules ---------------------------------------------
default:: my_package
MY_PACKAGE = $(PACKAGE)

my_package:: lib$(MY_PACKAGE)_pkg.so.1
	@echo lib$(MY_PACKAGE)_pkg.so.1 done

lib$(MY_PACKAGE)_pkg.so.1: $(CPPOBJS)
#	$(SHARED_LD) $(SHARED_LDFLAGS) -Wl,-soname,lib$(MY_PACKAGE)_pkg.so.1 -o lib$(MY_PACKAGE)_pkg.so.1 $(CPPOBJS) 
	@if [ "x$(CPPOBJS)" != "x" ] ; then \
	$(SHARED_LD) $(SHARED_LDFLAGS) \
          -Wl,-soname,lib$(MY_PACKAGE)_pkg.so.1 \
          -o lib$(MY_PACKAGE)_pkg.so.1 $(CPPOBJS) ; \
        pwd > $(LOCAL_SHARED_LIB_DIR)/tmp ; \
        cd $(LOCAL_SHARED_LIB_DIR); \
        rm -f lib$(MY_PACKAGE)_pkg.so.1 lib$(MY_PACKAGE)_pkg.so ; \
        ln -s `cat tmp`/lib$(MY_PACKAGE)_pkg.so.1 \
          lib$(MY_PACKAGE)_pkg.so ; \
        ln -s `cat tmp`/lib$(MY_PACKAGE)_pkg.so.1 \
          lib$(MY_PACKAGE)_pkg.so.1 ; \
        rm -f tmp; \
        else \
           touch lib$(MY_PACKAGE)_pkg.so.1; \
        fi



$(MY_SUBPACKOBJS):
	(export spkg; spkg=`echo $@ | sed -e 's%/.*%%g'`; \
	   cd $$spkg ; $(MAKE) my_package ; cd .. ;)


include make.depend



