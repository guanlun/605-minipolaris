#include <stdlib.h>
#include <fstream>
#include "strstream"

//#include <string.h>
#include <sys/resource.h>
#include "ProgramUnit.h"
#include "Program.h"
//#include "StmtList.h"
#include "Collection/List.h"
//#include "Collection/KeyIterator.h"
//#include "p-setjmp.h"
#include "utilities/stmt_util.h"
#include "utilities/switches_util.h"

#include <limits.h>

#include "gotos/gotos.h"
#include "bblock/bblock.h"
#include "bblock/BasicBlock.h"
#include "ssa/ssa.h"
#include "constant/constant.h"
#include "ddtest/ddtest.h"
#include "subexpr/subexpr.h"

using namespace std;

// output filename provided with option -o
extern char polarisOutputFileName[PATH_MAX];


Boolean
always(const Expression &)
{
    return 1;
}


int
driver(int argc, char *argv[])
{
  // make sure there is enough memory available to run polaris
  char *memory_block = (char *) malloc(20000000);
  p_assert(memory_block, "Insufficient memory to run Polaris");
  free(memory_block);

  // read in the switches file
  const char *version = "1.6.5-38.0 Feb 23, 2000";
  char *intrin_file_name;
  int    rc = parse_switches( argc, argv, version, &intrin_file_name );
  
  if (rc <  0) {
    cout << "ERROR: parse_switches() returns -1" << endl;
    exit(1);
  }
  
  if (rc >= argc) {
    cout << "source file name missing" << endl;
    exit(2);
  }

  Program         prog;

  for (int i = rc; i < argc; ++i) {
    Program *q = new Program;
    q->read( argv[i] );
    prog.absorb( q );
  }
  
  // Compute the interprocedural call-relationships for this Program
  prog.compute_call_lists();


  /////////////////////////////////////////////////////////////////
  //    start running the various transformation/optimization    //
  //    passes and print the results after each pass             //
  /////////////////////////////////////////////////////////////////


  KeyIterator<String, ProgramUnit> pit = prog;

  for ( ; pit.valid(); ++pit) {
    
    ProgramUnit &pgm = pit.current_data();
    String key = pit.current_key();
    

    
    ///////////////////////////////////////
    //  print after scanning program     //
    ///////////////////////////////////////
    
    switch (switch_value("p_scan"))
      {
      case 1:
	cout << "AFTER SCAN\n";
	pgm.write(cout);
	break;
      case 2:
	cout << "AFTER SCAN\n";
	pgm.print(cout);
      }
    
    
    ///////////////////////////////////////
    //  run the convert_gotos pass       //
    ///////////////////////////////////////
    
    if (switch_value("convert_gotos")) {
      convert_gotos(pgm);
      
      switch (switch_value("p_congoto"))
	{
	case 1:
	  cout << "AFTER CONVERT GOTOS\n";
	  pgm.write(cout);
	  break;
	case 2:
	  cout << "AFTER CONVERT GOTOS\n";
	  pgm.print(cout);
	}
    }
    
    ///////////////////////////////////////
    //  run the bblock pass              //
    ///////////////////////////////////////
    
    List<BasicBlock>         * pgm_basic_blocks = 0;
    if (switch_value("basic_blocks")) {
      pgm_basic_blocks = find_basic_blocks(pgm);
      
      if (switch_value("p_bbsummary"))
	if (pgm_basic_blocks)
	  {
	    summarize_basic_blocks(pgm, pgm_basic_blocks, cout);
	  }
      
      switch (switch_value("p_bblocks"))
	{
	case 1:
	  cout << "AFTER FIND BASIC BLOCKS\n";
	  pgm.write(cout);
	  break;
	case 2:
	  cout << "AFTER FIND BASIC BLOCKS\n";
	  pgm.print(cout);
	}
    }


    ///////////////////////////////////////
    // run the constprop pass            //
    ///////////////////////////////////////

    if (switch_value("constprop"))
      {

        propagate_constants(pgm);

        switch (switch_value("p_constprop")) {
        case 1:
          cout << "AFTER CONSTANT PROPAGATION\n";
          pgm.write(cout);
          break;
        case 2:
          cout << "AFTER CONSTANT PROPAGATION\n";
          pgm.print(cout);
        }
      }

    ///////////////////////////////////////
    //  run the ssa pass                 //
    ///////////////////////////////////////
    
    if (switch_value("ssa")) {
      pgm_basic_blocks = find_basic_blocks(pgm);
      ssa(pgm, pgm_basic_blocks);
      
      switch (switch_value("p_ssa")) {
      case 1:
	cout << "AFTER SSA\n";
	pgm.write(cout);
	break;
      case 2:
	cout << "AFTER SSA\n";
	pgm.print(cout);
      }
    }

    ///////////////////////////////////////                                                                                                             
    // run the gcse pass                 //
    ///////////////////////////////////////  

    if (switch_value("subexpr")) {
      if (switch_value("availexpr") == 0) {
	if ((switch_value("gcseliminate") != 0) ||
	    (switch_value("copyprop") != 0)) {
	  adjust_switch_value("availexpr", 1);
	}
      }

      subexpr_elimination(pgm, pgm_basic_blocks);
      switch (switch_value("p_subexpr")) {
      case 1:
	cout << "AFTER SUBEXPR\n";
	pgm.write(cout);
	break;
      case 2:
	cout << "AFTER SUBEXPR\n";
	pgm.print(cout);
      }
    }
    

    ///////////////////////////////////////    
    // run the ddtest pass               //
    ///////////////////////////////////////     

    if (switch_value("ddtest"))
      {
	ddtest(pgm);

	switch (switch_value("p_ddtest")) {
	case 1:
	  cout << "AFTER DD Test\n";
	  pgm.write(cout);
	  break;
	case 2:
	  cout << "AFTER DD Test\n";
	  pgm.print(cout);
	}
      }


    
    
    ///////////////////////////////////////
    //  run the dessa pass               //
    ///////////////////////////////////////
    
    if (switch_value("dessa")) {
      dessa(pgm);
      switch (switch_value("p_dessa")) {
      case 1:
	cout << "AFTER deSSA\n";
	pgm.write(cout);
	break;
      case 2:
	cout << "AFTER deSSA\n";
	pgm.print(cout);
      }
    }
    
  }
  
  /////////////////////////////////////////////////////////////////
  //    finished running the transformation/optimization         //
  /////////////////////////////////////////////////////////////////
  


  ///////////////////////////////////////
  //  print program after all passes   //
  ///////////////////////////////////////

  int print_switch = switch_value("print");
  if(print_switch > 0) {
    if (print_switch == 1)
      prog.write(cout);
    else if (print_switch == 2)
      prog.print(cout);
  }

  cout << flush;
  return 0;

}

