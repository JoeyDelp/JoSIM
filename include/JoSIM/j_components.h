// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_COMPONENTS_H
#define J_COMPONENTS_H
#include "j_std_include.h"
class jj_volt;

class Component {
	public:
		static
		void
		jj_comp(std::string modName, std::string area, std::string jjLabel, std::string subckt = "");

		static
		void
		jj_comp_phase(std::string modName, std::string area, std::string jjLabel, std::string subckt = "");
};
#endif