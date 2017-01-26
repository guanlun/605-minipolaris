
#include <exception>
#include <iostream>
#include "driver/driver.h"
#include "stack_dump.h"
#include "p-assert.h"

using std::cerr;


int main(int argc, char *argv[]){
  set_abort_style(ABORT_ABORT);
  try{
    init_stack_dump(argv[0]);
    return driver(argc, argv);
  } catch(std::exception& e) {
    std::cerr<<"\nMini Polaris: Uncaught exception: "<<e.what();
    return 1;
  }
}
