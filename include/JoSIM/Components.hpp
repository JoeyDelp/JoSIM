// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_COMPONENTS_HPP
#define JOSIM_COMPONENTS_HPP

#include <variant>

#include "JoSIM/CCCS.hpp"
#include "JoSIM/CCVS.hpp"
#include "JoSIM/Capacitor.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/CurrentSource.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Inductor.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/JJ.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/PhaseSource.hpp"
#include "JoSIM/Resistor.hpp"
#include "JoSIM/TransmissionLine.hpp"
#include "JoSIM/VCCS.hpp"
#include "JoSIM/VCVS.hpp"
#include "JoSIM/VoltageSource.hpp"
namespace JoSIM {

class Components {
 public:
  std::vector<
      std::variant<Resistor, Inductor, Capacitor, JJ, VoltageSource,
                   PhaseSource, TransmissionLine, VCCS, CCCS, VCVS, CCVS>>
      devices;
  std::vector<CurrentSource> currentsources;
  std::vector<int64_t> junctionIndices, resistorIndices, inductorIndices,
      capacitorIndices, vsIndices, psIndices, txIndices, vccsIndices,
      cccsIndices, vcvsIndices, ccvsIndices;
  std::vector<std::pair<tokens_t, string_o>> mutualinductances;
};  // class Components

}  // namespace JoSIM
#endif
