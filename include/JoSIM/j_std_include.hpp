// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_STD_INCLUDE
#define J_STD_INCLUDE

// Version info
#define VERSION 1.3

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

// JoSIM includes
#include "j_globals.hpp"
#include "j_errors.hpp"
#include "j_misc.hpp"
#include "j_parser.hpp"
#include "j_input.hpp"
#include "j_components.hpp"
#include "j_matrix.hpp"
#include "j_simulation.hpp"
#include "j_plot.hpp"
#include "j_output.hpp"

#endif