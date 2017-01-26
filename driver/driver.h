
//
// .NAME Polaris driver
// .LIBRARY C++ CVDL
// .HEADER Polaris
// .INCLUDE
// .FILE driver/driver.h

// .SECTION Description
// The Polaris driver parses the command line and calls the compilation
// passes.  The command line must have the form:
// polaris [-s switch1=value1,switch2=value2,...] source file
//
// For the user the switches are knobs and buttons to turn on/off
// individual compilation passes, and to set options for these passes. 
// Such options include whether passes should run quietly or be verbose, 
// what strategies should be applied by the passes, what parameters 
// should be chosen, etc. These switches can be added by programmers 
// in a flexible manners. Therefore the user should expect meanings 
// and names of switches to change. Switches are read in from the file 
// /groups/polaris/switches (or from the file given by the environment
// variable POLARIS_SWITCHES, respectively) where names, default values,
// and a one-line description can be found. (This file is read in by the 
// driver and only those switches are permitted on the command line.)
//
// For the programmer switches are a flexible means to input data to the
// experimental compilation pass and may eventually be supplied by either
// more formal command line options or by some intelligent strategies. 
// Some switches may only have meaning to the polaris developers, such as 
// debug levels, other may be for general users. In order to resolve name 
// conflicts of switches, new switches should always be added to the file 
// /groups/polaris/switches. For experiments you may make a copy of this
// file and change default values and use the filename given in environment
// variable POLARIS_SWITCHES. Values of switches can be read using the
// function switches.h:switch_value("switch-name");
//
// .SECTION Bugs .

int             driver(int argc, char *argv[]);

