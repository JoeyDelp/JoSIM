// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_STD_INCLUDE
#define J_STD_INCLUDE

// Version info
#define VERSION 2.1

// Executable info
#ifndef WIN32
#define EXEC josim.exe
#else
#define EXEC josim
#endif

// Standard library includes
#include <algorithm>
#include <cmath>
#include <float.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <locale>
#include <map>
#include <math.h>
#include <sstream>
#include <string>
#include <time.h>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <exception>
#include <ctime>
#ifdef USING_OPENMP
#include <omp.h>
#endif

// Graph plotting includes
#ifdef USING_FLTK
#ifdef WIN32
#include "../Fl/Fl.H"
#include "../Fl/Fl_Box.H"
#include "../Fl/Fl_Button.H"
#include "../Fl/Fl_Chart.H"
#include "../Fl/Fl_Double_Window.H"
#include "../Fl/Fl_Group.H"
#include "../Fl/Fl_Scroll.H"
#include "../Fl/Fl_Scrollbar.H"
#include "../Fl/Fl_Window.H"
#include "../Fl/fl_draw.H"

#else
#include <Fl/Fl.H>
#include <Fl/Fl_Box.H>
#include <Fl/Fl_Button.H>
#include <Fl/Fl_Chart.H>
#include <Fl/Fl_Double_Window.H>
#include <Fl/Fl_Group.H>
#include <Fl/Fl_Scroll.H>
#include <Fl/Fl_Scrollbar.H>
#include <Fl/Fl_Window.H>

#endif
#elif USING_MATPLOTLIB
#include "matplotlibcpp.h"
#endif

// Linear algebra include
#include "klu.h"

#include "j_globals.h"
#include "j_args.h"
#include "j_input.h"
#include "j_errors.h"
#include "j_misc.h"
#include "j_parser.h"
#include "j_components.h"
#include "j_matrix.h"
#include "j_simulation.h"
#include "j_plot.h"
#include "j_output.h"

#endif