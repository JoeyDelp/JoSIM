// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <tuple>
#include <cmath>
#include <functional>

#include "j_globals.hpp"
#include "j_misc.hpp"
#include "j_errors.hpp"
#include "j_input.hpp"
#include "j_components.hpp"
#include "j_models.hpp"
#include "j_simulation.hpp"
#include "j_matrix.hpp"
#include "j_plot.hpp"
#include "j_output.hpp"

#include "klu.h"

#include "Fl\Fl.H"
#include "Fl\Fl_Box.H"
#include "Fl\Fl_Window.H"
#include "Fl\Fl_Chart.H"
#include "Fl_Double_Window.H"
#include "Fl\Fl_Group.H"
#include "Fl\Fl_Scroll.H"
#include "Fl\Fl_Scrollbar.H"
#include "Fl\Fl_Button.H"
#include "Fl\fl_draw.H"
