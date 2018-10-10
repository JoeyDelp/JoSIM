// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
/*
  Contains all the global variables used in this program and where they are
  instantiated
*/
// josim.cpp
extern std::string OUTPUT_PATH, OUTPUT_FILE;
extern std::string OUTPUT_LEGACY_PATH, OUTPUT_LEGACY_FILE;
extern std::string INPUT_PATH, INPUT_FILE;
extern bool VERBOSE, OUTPUT, OUTPUT_LEGACY;
extern int subcktConv, analType;

#define PHI_ZERO 2.067833831170082E-15

#define LEFT 0
#define RIGHT 1

#define SUBGAP 0
#define TRANSITION 1
#define NORMAL 2

#define VANAL 0
#define PANAL 1