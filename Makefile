EXEC = mini_polaris

MY_SUBPACKOBJS = gotos/libgotos_pkg.so.1 \
	bblock/libbblock_pkg.so.1 \
	ssa/libssa_pkg.so.1 \
	ddtest/libddtest_pkg.so.1 \
	constant/libconstant_pkg.so.1 \
	subexpr/subexpr_pkg.so.1 \
	driver/libdriver_pkg.so.1

include config/Rules.make

default:: shared

shared: $(MY_SUBPACKOBJS) driver_test.o
	@echo Linking with shared libraries.
	@ls $(MINI_POLARIS_SHARED_LIB_DIR) | grep so.1 | \
	 sed s/lib/"-l"/ | sed s/pkg.so.1/pkg/ > shared_libs_list
	@ls $(LOCAL_SHARED_LIB_DIR) | grep so.1 | grep -v driver | \
	 sed s/lib/"-l"/ | sed s/pkg.so.1/pkg/ > local_shared_libs_list
	@$(CCC) driver_test.o -ldriver_pkg -L$(MINI_POLARIS_SHARED_LIB_DIR)  `cat shared_libs_list` \
	 -L$(LOCAL_SHARED_LIB_DIR)  `cat local_shared_libs_list` \
	 -o ${EXEC}
	@rm -f shared_libs_list local_shared_libs_list
	@echo Shared polaris done.


clean::
	@if [ -e gotos ]; then \
		(cd gotos ; $(RM) -f libgotos_pkg.so.1 *.o); \
	fi
	@if [ -e driver ]; then \
		(cd driver ; $(RM) -f libdriver_pkg.so.1 *.o); \
	fi
	@if [ -e bblock ]; then \
		(cd bblock ; $(RM) -f libbblock_pkg.so.1 *.o); \
	fi
	@if [ -e ssa ]; then \
		(cd ssa ; $(RM) -f libssa_pkg.so.1 *.o); \
	fi
	@if [ -e subexpr ]; then \
		(cd subexpr ; $(RM) -f libsubexpr_pkg.so.1 *.o); \
	fi
	@if [ -e ddtest ]; then \
		(cd ddtest ; $(RM) -f libddtest_pkg.so.1 *.o); \
	fi
	@if [ -e constant ]; then \
		(cd constant ; $(RM) -f libconstprop_pkg.so.1 *.o); \
	fi
