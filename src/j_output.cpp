// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_output.h"

#include <cassert>

void
Output::relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj) {
  // Clear Traces so that we don't repopulate it through the interface
  traces.clear();

	timesteps = &sObj.results.timeAxis;
	std::vector<std::string> tokens, tokens2;
	std::string label, label2;
	int index1, index2;
	RowDescriptor &dev = mObj.deviceLabelIndex.at(mObj.deviceLabelIndex.begin()->first);
	RowDescriptor &dev2 = mObj.deviceLabelIndex.at(mObj.deviceLabelIndex.begin()->first);
	for (const auto& i : iObj.controls) {
		if(i.find("PRINT") != std::string::npos) {
			Trace thisTrace;
			tokens = Misc::tokenize_space(i);
			if(tokens.at(1) == "DEVI") {
				if(tokens.size() > 4) 
					Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				} else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				thisTrace.name = label;
				thisTrace.type = 'C';
				if (mObj.deviceLabelIndex.count(label) != 0) {
					dev = mObj.deviceLabelIndex.at(label);
					switch(dev.type) {
						case RowDescriptor::Type::VoltageResistor:
							if(mObj.components.voltRes.at(dev.index).posNCol == -1) {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).negNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).negNCol))).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										(1 / mObj.components.voltRes.at(dev.index).value)));
							} else if(mObj.components.voltRes.at(dev.index).negNCol == -1) {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).posNCol))).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										(1 / mObj.components.voltRes.at(dev.index).value)));
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								std::transform(
									thisTrace.calcData.begin(),
									thisTrace.calcData.end(),
									thisTrace.calcData.begin(),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										(1 / mObj.components.voltRes.at(dev.index).value)));
							}
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::PhaseResistor:
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								mObj.components.phaseRes.at(dev.index).curNCol)));
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::VoltageInductor:
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								mObj.components.voltInd.at(dev.index).curNCol)));
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::PhaseInductor:
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								mObj.components.phaseInd.at(dev.index).curNCol)));
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::VoltageCapacitor:
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								mObj.components.voltCap.at(dev.index).curNCol)));
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::PhaseCapacitor:
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								mObj.components.phaseCap.at(dev.index).curNCol)));
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::VoltageJJ:
							thisTrace.traceData = &mObj.components.voltJJ.at(dev.index).jjCur;
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::PhaseJJ:
							thisTrace.traceData = &mObj.components.phaseJJ.at(dev.index).jjCur;
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::VoltageVS:
							Errors::control_errors(CURRENT_THROUGH_VOLT, i);
							break;
						case RowDescriptor::Type::PhaseVS:
							Errors::control_errors(CURRENT_THROUGH_VOLT, i);
							break;
						case RowDescriptor::Type::VoltageCS:
							thisTrace.traceData = &mObj.sources.at(dev.index);
							traces.push_back(thisTrace);
							break;
						default:
							Errors::control_errors(UNKNOWN_DEVICE, label);
							break;
					}
				} else Errors::control_errors(UNKNOWN_DEVICE, label);
			} else if(tokens.at(1) == "DEVV") {
				if(tokens.size() > 4) Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				}
				else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				thisTrace.name = label;
				thisTrace.type = 'V';
				if (mObj.deviceLabelIndex.count(label) != 0) { 
					dev = mObj.deviceLabelIndex.at(label);
					switch(dev.type) {
						case RowDescriptor::Type::VoltageResistor:
							if(mObj.components.voltRes.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltRes.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.voltRes.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltRes.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltRes.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::PhaseResistor:
							Errors::control_errors(VOLT_WHEN_PHASE, label);
							if(mObj.components.phaseRes.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseRes.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseRes.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseRes.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseRes.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseRes.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseRes.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageInductor:
							if(mObj.components.voltInd.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltInd.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.voltInd.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltInd.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltInd.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltInd.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltInd.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::PhaseInductor:
							Errors::control_errors(VOLT_WHEN_PHASE, label);
							if(mObj.components.phaseInd.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseInd.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseInd.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseInd.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseInd.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseInd.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseInd.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageCapacitor:
							if(mObj.components.voltCap.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltCap.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.voltCap.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltCap.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltCap.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltCap.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltCap.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::PhaseCapacitor:
							Errors::control_errors(VOLT_WHEN_PHASE, label);
							if(mObj.components.phaseCap.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseCap.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseCap.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseCap.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseCap.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseCap.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseCap.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageJJ:
              {
              auto& voltjj = mObj.components.voltJJ.at(dev.index);

							if(voltjj.posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltJJ.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(voltjj.negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltJJ.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {

                std::cout << "posNCol " << voltjj.posNCol << std::endl;
                std::cout << "negNCol " << voltjj.negNCol << std::endl;

                for(auto& value: mObj.relXInd)
                  std::cout << value << " ";
                std::cout << std::endl;

                auto posncol_iterator = std::find(mObj.relXInd.begin(), mObj.relXInd.end(), voltjj.posNCol);
                auto negncol_iterator = std::find(mObj.relXInd.begin(), mObj.relXInd.end(), voltjj.negNCol);

                assert(posncol_iterator != mObj.relXInd.end());
                assert(negncol_iterator != mObj.relXInd.end());

                auto index_posncol = std::distance(mObj.relXInd.begin(), posncol_iterator);
                auto index_negncol = std::distance(mObj.relXInd.begin(), negncol_iterator);

                auto& vector_posncol = sObj.results.xVect.at(index_posncol);
                auto& vector_negncol = sObj.results.xVect.at(index_negncol);

								std::transform(
									vector_posncol.begin(),
                  vector_posncol.end(),
                  vector_negncol.begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());

								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
              }
							break;
						case RowDescriptor::Type::PhaseJJ:
							Errors::control_errors(VOLT_WHEN_PHASE, label);
							if(mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseJJ.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseJJ.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseJJ.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseJJ.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseJJ.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseJJ.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageVS:
							thisTrace.traceData = &mObj.sources.at(dev.index);
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::PhaseVS:
							thisTrace.traceData = &mObj.sources.at(dev.index);
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::VoltageCS:
							Errors::control_errors(VOLT_ACROSS_CURRENT, i);
							break;
						case RowDescriptor::Type::PhaseCS:
							Errors::control_errors(VOLT_ACROSS_CURRENT, i);
							break;
						default:
							Errors::control_errors(UNKNOWN_DEVICE, label);
							break;
					}
				} else Errors::control_errors(UNKNOWN_DEVICE, label);
			} else if(tokens.at(1) == "NODEV") {
				thisTrace.type = 'V';
				if (tokens.size() > 4) {
					Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
				} else if (tokens.size() == 3) {
					label = tokens.at(2);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(mObj.deviceLabelIndex.count(label) != 0) {
						index1 = mObj.deviceLabelIndex.at(label).index;
						if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
							Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
							thisTrace.type = 'P';
							thisTrace.name = "NP_" + label;
						} else thisTrace.name = "NV_" + label;
						thisTrace.traceData = &sObj.results.xVect.at(
							std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
							index1)));
						traces.push_back(thisTrace);
					} else Errors::control_errors(UNKNOWN_NODE, label);
				} else if (tokens.size() == 4) {
					label = tokens.at(2);
					label2 = tokens.at(3);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					} 
					if(label2.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label2, "_");
						label2 = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label2 = label + "|" + tokens.at(j);
					} else if(label2.find('.') != std::string::npos) {
						std::replace(label2.begin(), label2.end(), '.', '|');
					}
					if(label == "0" || label == "GND") {
						if(mObj.deviceLabelIndex.count(label2) != 0) {
							index2 = mObj.deviceLabelIndex.at(label2).index;
							if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
								Errors::control_errors(NODEVOLT_WHEN_PHASE, label2);
								thisTrace.type = 'P';
								thisTrace.name = "NP_" + label2;
							} else thisTrace.name = "NV_" + label2;
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								index2)));
							traces.push_back(thisTrace);
						} else Errors::control_errors(UNKNOWN_NODE, label2);
					} else if (label2 == "0" || label2 == "GND") {
						if(mObj.deviceLabelIndex.count(label) != 0) {
							index1 = mObj.deviceLabelIndex.at(label).index;
							if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
								Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
								thisTrace.type = 'P';
								thisTrace.name = "NP_" + label;
							} else thisTrace.name = "NV_" + label;
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								index1)));
							traces.push_back(thisTrace);
						} else Errors::control_errors(UNKNOWN_NODE, label);
					} else {
						if(mObj.deviceLabelIndex.count(label2) != 0) {
							if(mObj.deviceLabelIndex.count(label) != 0) {
								if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
									Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
									thisTrace.type = 'P';
									thisTrace.name = "NP_" + label + "_" + label2;
								} else thisTrace.name = "NV_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index1))).begin(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index1))).end(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index2))).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							} else Errors::control_errors(UNKNOWN_NODE, label);
						} else Errors::control_errors(UNKNOWN_NODE, label2);
					}						
				} else 
					Errors::control_errors(INVALID_NODEV, i);
			} else if(tokens.at(1) == "NODEP") {
				thisTrace.type = 'P';
				if (tokens.size() > 4) {
					Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
				} else if (tokens.size() == 3) {
					label = tokens.at(2);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(mObj.deviceLabelIndex.count(label) != 0) {
						index1 = mObj.deviceLabelIndex.at(label).index;
						if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
							Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
							thisTrace.type = 'V';
							thisTrace.name = "NV_" + label;
						} else thisTrace.name = "NP_" + label;
						thisTrace.traceData = &sObj.results.xVect.at(
							std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
							index1)));
						traces.push_back(thisTrace);
					} else Errors::control_errors(UNKNOWN_NODE, label);
				} else if (tokens.size() == 4) {
					label = tokens.at(2);
					label2 = tokens.at(3);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					} 
					if(label2.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label2, "_");
						label2 = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label2 = label + "|" + tokens.at(j);
					} else if(label2.find('.') != std::string::npos) {
						std::replace(label2.begin(), label2.end(), '.', '|');
					}
					if(label == "0" || label == "GND") {
						if(mObj.deviceLabelIndex.count(label2) != 0) {
							index2 = mObj.deviceLabelIndex.at(label2).index;
							if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
								Errors::control_errors(NODEPHASE_WHEN_VOLT, label2);
								thisTrace.type = 'V';
								thisTrace.name = "NV_" + label2;
							} else thisTrace.name = "NP_" + label2;
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								index2)));
							traces.push_back(thisTrace);
						} else Errors::control_errors(UNKNOWN_NODE, label2);
					} else if (label2 == "0" || label2 == "GND") {
						if(mObj.deviceLabelIndex.count(label) != 0) {
							index1 = mObj.deviceLabelIndex.at(label).index;
							if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
								Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
								thisTrace.type = 'V';
								thisTrace.name = "NV_" + label;
							} else thisTrace.name = "NP_" + label;
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								index1)));
							traces.push_back(thisTrace);
						} else Errors::control_errors(UNKNOWN_NODE, label);
					} else {
						if(mObj.deviceLabelIndex.count(label2) != 0) {
							if(mObj.deviceLabelIndex.count(label) != 0) {
								if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
									Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
									thisTrace.type = 'V';
									thisTrace.name = "NV_" + label + "_" + label2;
								} else thisTrace.name = "NP_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index1))).begin(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index1))).end(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index2))).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							} else Errors::control_errors(UNKNOWN_NODE, label);
						} else Errors::control_errors(UNKNOWN_NODE, label2);
					}
				} else 
					Errors::control_errors(INVALID_NODEV, i);
			} else if(tokens.at(1) == "PHASE") {
				if(tokens.size() > 4) 
					Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				} else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				thisTrace.name = label;
				thisTrace.type = 'P';
				if (mObj.deviceLabelIndex.count(label) != 0) {
					dev = mObj.deviceLabelIndex.at(label);
					switch(dev.type) {
						case RowDescriptor::Type::VoltageResistor:
							Errors::control_errors(PHASE_WHEN_VOLT, i);
							break;
						case RowDescriptor::Type::PhaseResistor:
							if(mObj.components.phaseRes.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseRes.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseRes.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseRes.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseRes.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseRes.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseRes.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageInductor:
							Errors::control_errors(PHASE_WHEN_VOLT, i);
							break;
						case RowDescriptor::Type::PhaseInductor:
							if(mObj.components.phaseInd.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseInd.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseInd.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseInd.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseInd.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseInd.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseInd.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageCapacitor:
							Errors::control_errors(PHASE_WHEN_VOLT, i);
							break;
						case RowDescriptor::Type::PhaseCapacitor:
							if(mObj.components.phaseCap.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseCap.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseCap.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseCap.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseCap.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseCap.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseCap.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageJJ:
							thisTrace.traceData = &sObj.results.xVect.at(
								std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
								mObj.components.voltJJ.at(dev.index).phaseNCol)));
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::PhaseJJ:
							if(mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseJJ.at(dev.index).negNCol)));
								traces.push_back(thisTrace);
							} else if(mObj.components.phaseJJ.at(dev.index).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseJJ.at(dev.index).posNCol)));
								traces.push_back(thisTrace);
							} else {
								std::transform(
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseJJ.at(dev.index).posNCol))).begin(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseJJ.at(dev.index).posNCol))).end(),
									sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseJJ.at(dev.index).negNCol))).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							break;
						case RowDescriptor::Type::VoltageVS:
							Errors::control_errors(PHASE_OF_VOLT, i);
							break;
						case RowDescriptor::Type::PhaseVS:
							Errors::control_errors(PHASE_OF_VOLT, i);
							break;
						case RowDescriptor::Type::PhasePS:
							thisTrace.traceData = &mObj.sources.at(dev.index);
							traces.push_back(thisTrace);
							break;
						case RowDescriptor::Type::VoltageCS:
							Errors::control_errors(PHASE_OF_CURRENT, i);
							break;
						case RowDescriptor::Type::PhaseCS:
							Errors::control_errors(PHASE_OF_CURRENT, i);
							break;
						default:
							Errors::control_errors(UNKNOWN_DEVICE, label);
							break;
					}
				} else Errors::control_errors(UNKNOWN_DEVICE, label);
			}
		} else if(i.find("PLOT") != std::string::npos) {
			tokens = Misc::tokenize_space(i);
			for (int j = 1; j < tokens.size(); j++) {
				Trace thisTrace;
				if (tokens.at(j)[0] == 'V') {
					thisTrace.type = 'V';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "V() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						thisTrace.name = label;
						if (mObj.deviceLabelIndex.count(label) != 0) { 
							dev = mObj.deviceLabelIndex.at(label);
							switch(dev.type) {
								case RowDescriptor::Type::VoltageResistor:
									if(mObj.components.voltRes.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.voltRes.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::PhaseResistor:
									Errors::control_errors(VOLT_WHEN_PHASE, label);
									if(mObj.components.phaseRes.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseRes.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseRes.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseRes.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseRes.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseRes.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseRes.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageInductor:
									if(mObj.components.voltInd.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltInd.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.voltInd.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltInd.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltInd.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltInd.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltInd.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::PhaseInductor:
									Errors::control_errors(VOLT_WHEN_PHASE, label);
									if(mObj.components.phaseInd.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseInd.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseInd.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseInd.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseInd.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseInd.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(std::distance(
												mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseInd.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageCapacitor:
									if(mObj.components.voltCap.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltCap.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.voltCap.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltCap.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltCap.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltCap.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltCap.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::PhaseCapacitor:
									Errors::control_errors(VOLT_WHEN_PHASE, label);
									if(mObj.components.phaseCap.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseCap.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseCap.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseCap.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseCap.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseCap.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseCap.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageJJ:
									if(mObj.components.voltJJ.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltJJ.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.voltJJ.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltJJ.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltJJ.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltJJ.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltJJ.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::PhaseJJ:
									Errors::control_errors(VOLT_WHEN_PHASE, label);
									if(mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseJJ.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseJJ.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseJJ.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseJJ.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseJJ.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseJJ.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageVS:
									thisTrace.traceData = &mObj.sources.at(dev.index);
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::PhaseVS:
									thisTrace.traceData = &mObj.sources.at(dev.index);
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::VoltageCS:
									Errors::control_errors(VOLT_ACROSS_CURRENT, i);
									break;
								case RowDescriptor::Type::PhaseCS:
									Errors::control_errors(VOLT_ACROSS_CURRENT, i);
									break;
								default:
									if(mObj.deviceLabelIndex.count(label) != 0) { 
										index1 = mObj.deviceLabelIndex.at(label).index;
										if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
											Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
											thisTrace.type = 'P';
											thisTrace.name = "NP_" + label;
										} else thisTrace.name = "NV_" + label;
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index1)));
										traces.push_back(thisTrace);
									}
									else Errors::control_errors(UNKNOWN_NODE, label);
									break;
							}
						} else Errors::control_errors(UNKNOWN_DEVICE, label);
					} else {
						label = tokens2.at(0);
						label2 = tokens2.at(1);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						if(label2.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label2, "_");
							label2 = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label2 = label + "|" + tokens2.at(k);
						} else if(label2.find('.') != std::string::npos) {
							std::replace(label2.begin(), label2.end(), '.', '|');
						}
						if(label == "0" || label == "GND") {
							if(mObj.deviceLabelIndex.count(label2) != 0) {
								index2 = mObj.deviceLabelIndex.at(label2).index;
								if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
									Errors::control_errors(NODEVOLT_WHEN_PHASE, label2);
									thisTrace.type = 'P';
									thisTrace.name = "NP_" + label2;
								} else thisTrace.name = "NV_" + label2;
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									index2)));
								traces.push_back(thisTrace);
							} else Errors::control_errors(UNKNOWN_NODE, label2);
						} else if (label2 == "0" || label2 == "GND") {
							if(mObj.deviceLabelIndex.count(label) != 0) {
								index1 = mObj.deviceLabelIndex.at(label).index;
								if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
									Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
									thisTrace.type = 'P';
									thisTrace.name = "NP_" + label;
								} else thisTrace.name = "NV_" + label;
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									index1)));
								traces.push_back(thisTrace);
							} else Errors::control_errors(UNKNOWN_NODE, label);
						} else {
							if(mObj.deviceLabelIndex.count(label2) != 0) {
								if(mObj.deviceLabelIndex.count(label) != 0) {
									if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
										Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
										thisTrace.type = 'P';
										thisTrace.name = "NP_" + label + "_" + label2;
									} else thisTrace.name = "NV_" + label + "_" + label2;
									std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												index1))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												index1))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												index2))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
									thisTrace.pointer = false;
									traces.push_back(thisTrace);
								} else Errors::control_errors(UNKNOWN_NODE, label);
							} else Errors::control_errors(UNKNOWN_NODE, label2);
						}
					}
				} else if (tokens.at(j)[0] == 'C') {
					thisTrace.type = 'C';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "C() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						thisTrace.name = label;
						thisTrace.type = 'C';
						if (mObj.deviceLabelIndex.count(label) != 0) {
							dev = mObj.deviceLabelIndex.at(label);
							switch(dev.type) {
								case RowDescriptor::Type::VoltageResistor:
									if(mObj.components.voltRes.at(dev.index).posNCol == -1) {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).negNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).negNCol))).end(),
											std::back_inserter(thisTrace.calcData),
											std::bind(std::multiplies<double>(),
												std::placeholders::_1,
												(1 / mObj.components.voltRes.at(dev.index).value)));
									} else if(mObj.components.voltRes.at(dev.index).negNCol == -1) {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).posNCol))).end(),
											std::back_inserter(thisTrace.calcData),
											std::bind(std::multiplies<double>(),
												std::placeholders::_1,
												(1 / mObj.components.voltRes.at(dev.index).value)));
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.voltRes.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										std::transform(
											thisTrace.calcData.begin(),
											thisTrace.calcData.end(),
											thisTrace.calcData.begin(),
											std::bind(std::multiplies<double>(),
												std::placeholders::_1,
												(1 / mObj.components.voltRes.at(dev.index).value)));
									}
									thisTrace.pointer = false;
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::PhaseResistor:
									thisTrace.traceData = &sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseRes.at(dev.index).curNCol)));
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::VoltageInductor:
									thisTrace.traceData = &sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltInd.at(dev.index).curNCol)));
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::PhaseInductor:
									thisTrace.traceData = &sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseInd.at(dev.index).curNCol)));
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::VoltageCapacitor:
									thisTrace.traceData = &sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltCap.at(dev.index).curNCol)));
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::PhaseCapacitor:
									thisTrace.traceData = &sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.phaseCap.at(dev.index).curNCol)));
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::VoltageJJ:
									thisTrace.traceData = &mObj.components.voltJJ.at(dev.index).jjCur;
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::PhaseJJ:
									thisTrace.traceData = &mObj.components.phaseJJ.at(dev.index).jjCur;
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::VoltageVS:
									Errors::control_errors(CURRENT_THROUGH_VOLT, i);
									break;
								case RowDescriptor::Type::PhaseVS:
									Errors::control_errors(CURRENT_THROUGH_VOLT, i);
									break;
								case RowDescriptor::Type::VoltageCS:
									thisTrace.traceData = &mObj.sources.at(dev.index);
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::PhaseCS:
									thisTrace.traceData = &mObj.sources.at(dev.index);
									traces.push_back(thisTrace);
									break;
								default:
									Errors::control_errors(UNKNOWN_DEVICE, label);
									break;
							}
						} else Errors::control_errors(UNKNOWN_DEVICE, label);
					} else 
						Errors::control_errors(INVALID_CURRENT, i);
				} else if (tokens.at(j).find("#BRANCH") != std::string::npos) {
					thisTrace.type = 'C';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), " #");
					label = tokens2.at(0);
					if(label.find('_') != std::string::npos) {
						tokens2 = Misc::tokenize_delimeter(label, "_");
						label = tokens2.back();
						for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					thisTrace.name = label;
					if (mObj.deviceLabelIndex.count(label) != 0) {
						dev = mObj.deviceLabelIndex.at(label);
						switch(dev.type) {
							case RowDescriptor::Type::VoltageResistor:
								if(mObj.components.voltRes.at(dev.index).posNCol == -1) {
									std::transform(
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).negNCol))).begin(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).negNCol))).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											(1 / mObj.components.voltRes.at(dev.index).value)));
								} else if(mObj.components.voltRes.at(dev.index).negNCol == -1) {
									std::transform(
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).posNCol))).begin(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).posNCol))).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											(1 / mObj.components.voltRes.at(dev.index).value)));
								} else {
									std::transform(
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).posNCol))).begin(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).posNCol))).end(),
										sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.voltRes.at(dev.index).negNCol))).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
									std::transform(
										thisTrace.calcData.begin(),
										thisTrace.calcData.end(),
										thisTrace.calcData.begin(),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											(1 / mObj.components.voltRes.at(dev.index).value)));
								}
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::PhaseResistor:
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseRes.at(dev.index).curNCol)));
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::VoltageInductor:
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltInd.at(dev.index).curNCol)));
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::PhaseInductor:
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseInd.at(dev.index).curNCol)));
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::VoltageCapacitor:
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.voltCap.at(dev.index).curNCol)));
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::PhaseCapacitor:
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									mObj.components.phaseCap.at(dev.index).curNCol)));
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::VoltageJJ:
								thisTrace.traceData = &mObj.components.voltJJ.at(dev.index).jjCur;
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::PhaseJJ:
								thisTrace.traceData = &mObj.components.phaseJJ.at(dev.index).jjCur;
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::VoltageVS:
								Errors::control_errors(CURRENT_THROUGH_VOLT, i);
								break;
							case RowDescriptor::Type::PhaseVS:
								Errors::control_errors(CURRENT_THROUGH_VOLT, i);
								break;
							case RowDescriptor::Type::VoltageCS:
								thisTrace.traceData = &mObj.sources.at(dev.index);
								traces.push_back(thisTrace);
								break;
							case RowDescriptor::Type::PhaseCS:
								thisTrace.traceData = &mObj.sources.at(dev.index);
								traces.push_back(thisTrace);
								break;
							default:
								Errors::control_errors(UNKNOWN_DEVICE, label);
								break;
						}
					} else Errors::control_errors(UNKNOWN_DEVICE, label);
				} else if (tokens.at(j)[0] == 'P') {
					thisTrace.type = 'P';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "P() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						thisTrace.name = label;
						if (mObj.deviceLabelIndex.count(label) != 0) {
							dev = mObj.deviceLabelIndex.at(label);
							switch(dev.type) {
								case RowDescriptor::Type::VoltageResistor:
									Errors::control_errors(PHASE_WHEN_VOLT, i);
									break;
								case RowDescriptor::Type::PhaseResistor:
									if(mObj.components.phaseRes.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseRes.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseRes.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseRes.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseRes.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseRes.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseRes.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageInductor:
									Errors::control_errors(PHASE_WHEN_VOLT, i);
									break;
								case RowDescriptor::Type::PhaseInductor:
									if(mObj.components.phaseInd.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseInd.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseInd.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseInd.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseInd.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseInd.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseInd.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageCapacitor:
									Errors::control_errors(PHASE_WHEN_VOLT, i);
									break;
								case RowDescriptor::Type::PhaseCapacitor:
									if(mObj.components.phaseCap.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseCap.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseCap.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseCap.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseCap.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseCap.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseCap.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageJJ:
									thisTrace.traceData = &sObj.results.xVect.at(
										std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
										mObj.components.voltJJ.at(dev.index).phaseNCol)));
									traces.push_back(thisTrace);
									break;
								case RowDescriptor::Type::PhaseJJ:
									if(mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseJJ.at(dev.index).negNCol)));
										traces.push_back(thisTrace);
									} else if(mObj.components.phaseJJ.at(dev.index).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											mObj.components.phaseJJ.at(dev.index).posNCol)));
										traces.push_back(thisTrace);
									} else {
										std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseJJ.at(dev.index).posNCol))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseJJ.at(dev.index).posNCol))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												mObj.components.phaseJJ.at(dev.index).negNCol))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
									break;
								case RowDescriptor::Type::VoltageVS:
									Errors::control_errors(PHASE_OF_VOLT, i);
									break;
								case RowDescriptor::Type::VoltageCS:
									Errors::control_errors(PHASE_OF_CURRENT, i);
									break;
								default:
									if(mObj.deviceLabelIndex.count(label) != 0) {
										index1 = mObj.deviceLabelIndex.at(label).index;
										if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
											Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
											thisTrace.type = 'V';
											thisTrace.name = "NV_" + label;
										} else thisTrace.name = "NP_" + label;
										thisTrace.traceData = &sObj.results.xVect.at(
											std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
											index1)));
										traces.push_back(thisTrace);
									} else Errors::control_errors(UNKNOWN_NODE, label);
									break;
							}
						} else Errors::control_errors(UNKNOWN_DEVICE, label);
					}
					else {
						label = tokens2.at(0);
						label2 = tokens2.at(1);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						if(label2.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label2, "_");
							label2 = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label2 = label + "|" + tokens2.at(k);
						} else if(label2.find('.') != std::string::npos) {
							std::replace(label2.begin(), label2.end(), '.', '|');
						}
						if(label == "0" || label == "GND") {
							if(mObj.deviceLabelIndex.count(label2) != 0) {
								index2 = mObj.deviceLabelIndex.at(label2).index;
								if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
									Errors::control_errors(NODEVOLT_WHEN_PHASE, label2);
									thisTrace.type = 'V';
									thisTrace.name = "NV_" + label2;
								} else thisTrace.name = "NP_" + label2;
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									index2)));
								traces.push_back(thisTrace);
							} else Errors::control_errors(UNKNOWN_NODE, label2);
						} else if (label2 == "0" || label2 == "GND") {
							if(mObj.deviceLabelIndex.count(label) != 0) {
								index1 = mObj.deviceLabelIndex.at(label).index;
								if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
									Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
									thisTrace.type = 'V';
									thisTrace.name = "NV_" + label;
								} else thisTrace.name = "NP_" + label;
								thisTrace.traceData = &sObj.results.xVect.at(
									std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
									index1)));
								traces.push_back(thisTrace);
							} else Errors::control_errors(UNKNOWN_NODE, label);
						} else {
							if(mObj.deviceLabelIndex.count(label2) != 0) {
								if(mObj.deviceLabelIndex.count(label) != 0) {
									if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
										Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
										thisTrace.type = 'V';
										thisTrace.name = "NV_" + label + "_" + label2;
									} else thisTrace.name = "NP_" + label + "_" + label2;
									std::transform(
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												index1))).begin(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												index1))).end(),
											sObj.results.xVect.at(
												std::distance(mObj.relXInd.begin(), std::find(mObj.relXInd.begin(), mObj.relXInd.end(), 
												index2))).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
									thisTrace.pointer = false;
									traces.push_back(thisTrace);
								} else Errors::control_errors(UNKNOWN_NODE, label);
							} else Errors::control_errors(UNKNOWN_NODE, label2);
						}
					}
				} else if (tokens.at(j).find("TRAN") != std::string::npos) {}
				else if (tokens.at(j)[0] == '"')
					Errors::control_errors(MATHOPS, tokens.at(j));
				else
					Errors::control_errors(UNKNOWN_PLOT, i);
			}
		}
		else if(i.find("SAVE") != std::string::npos) {
			Trace thisTrace;
			tokens = Misc::tokenize_space(i);
			for (int k = 1; k < tokens.size(); k++) {
				index1 = tokens.at(k).find('@');
				if (index1 != std::string::npos) tokens.at(k) = tokens.at(k).substr(0, index1) + tokens.at(k).substr(index1 + 1);
				index1 = tokens.at(k).find(".at(");
				if (index1 != std::string::npos) tokens.at(k) = tokens.at(k).substr(0, index1);
				index1 = tokens.at(k).find('.');
				if (index1 != std::string::npos) {
					tokens.at(k) = tokens.at(k).substr(0, index1) + "|" + tokens.at(k).substr(index1 + 1);
				}
				index1 = tokens.at(k).find('[');
				if (index1 != std::string::npos) tokens.at(k) = tokens.at(k).substr(0, index1);
				/* If this is a current source */
				if(mObj.deviceLabelIndex.count(tokens.at(k)) != 0) {
					dev = mObj.deviceLabelIndex.at(tokens.at(k));
					thisTrace.type = 'C';
					thisTrace.name = tokens.at(k);
					thisTrace.traceData = &mObj.sources.at(dev.index);
					traces.push_back(thisTrace);
				}
			}
		}
	}

}

void
Output::write_data(std::string &outname) {
	int loopsize = 0;
	std::ofstream outfile(outname);
	if (outfile.is_open()) {
		if (!traces.empty()) {
			if(traces.at(0).pointer)
				loopsize = traces.at(0).traceData->size();
			else
				loopsize = traces.at(0).calcData.size();
			outfile << "time"
				<< ",";
			for (int i = 0; i < traces.size() - 1; i++) {
				outfile << traces.at(i).name << ",";
			}
			outfile << traces.at(traces.size() - 1).name << "\n";
			for (int i = 0; i < loopsize; i++) {
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< timesteps->at(i) << ",";
				for (int j = 0; j < traces.size() - 1; j++) {
					if(traces.at(j).pointer)
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).traceData->at(i) << ",";
					else
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).calcData.at(i) << ",";
				}
				if(traces.at(traces.size() - 1).pointer)
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).traceData->at(i) << "\n";
				else
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).calcData.at(i) << "\n";
			}
			outfile.close();
		}
		else if (traces.empty()) {
			std::cerr << "W: Nothing specified to save."
				<< std::endl;
			std::cerr << std::endl;
			outfile.close();
		}
	}
	else {
		std::cerr << "E: " << outname << " could not be opened for saving."
				<< std::endl;
		std::cerr << std::endl;
	}
}

void
Output::write_legacy_data(std::string &outname) {
	int loopsize = 0;
	std::ofstream outfile(outname);
	if (outfile.is_open()) {
		if(!traces.empty()) {
			if(traces.at(0).pointer)
				loopsize = traces.at(0).traceData->size();
			else
				loopsize = traces.at(0).calcData.size();
			outfile << "time"
				<< " ";
			for (int i = 0; i < traces.size() - 1; i++) {
				outfile << traces.at(i).name << " ";
			}
			outfile << traces.at(traces.size() - 1).name << "\n";
			for (int i = 0; i < loopsize; i++) {
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< timesteps->at(i) << " ";
				for (int j = 0; j < traces.size() - 1; j++) {
					if(traces.at(j).pointer)
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).traceData->at(i) << " ";
					else
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).calcData.at(i) << " ";
				}
				if(traces.at(traces.size() - 1).pointer)
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).traceData->at(i) << "\n";
				else
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).calcData.at(i) << "\n";
			}
			outfile.close();
		}
		else if (traces.empty()) {
			std::cerr << "W: Nothing specified to save."
				<< std::endl;
			std::cerr << std::endl;
			outfile.close();
		}
	}
	else {
		std::cerr << "E: " << outname << " could not be opened for saving."
				<< std::endl;
			std::cerr << std::endl;
	}
}

void
Output::write_wr_data(std::string &outname) {
	std::string label;
	std::vector<std::string> tokens;
	int loopsize = 0;
	std::ofstream outfile(outname);
	if (outfile.is_open()) {
		if (!traces.empty()) {
			outfile << "Title: CKT1\n";
			std::time_t result = std::time(nullptr);
			outfile << "Date: " << std::asctime(std::localtime(&result));
			outfile << "Plotname: Transient analysis JoSIM\n";
			outfile << "Flags: real\n";
			outfile << "No. Variables: " << traces.size() + 1 << "\n";
			if(traces.at(0).pointer)
				loopsize = traces.at(0).traceData->size();
			else
				loopsize = traces.at(0).calcData.size();
			outfile << "No. Points: " << loopsize << "\n";
			outfile << "Command: version 4.3.8\n";
			outfile << "Variables:\n";
			outfile << " 0 time S\n";
			for (int i = 0; i < traces.size(); i++) {
				if(traces.at(i).type == 'V') {
					if(traces.at(i).name.substr(0, 2) == "NV") {
						tokens = Misc::tokenize_delimeter(traces.at(i).name, "_");
						if (tokens.size() > 2) outfile << " "
							<< i+1 << " v(" << tokens[1] << "," << tokens[2] <<") V\n";
						else outfile << " " << i+1 << " v(" << tokens[1] <<") V\n";
					}
					else outfile << " " << i+1 << " " << traces.at(i).name <<" V\n";
				}
				else if(traces.at(i).type == 'P') {
					if(traces.at(i).name.substr(0, 2) == "NP") {
						tokens = Misc::tokenize_delimeter(traces.at(i).name, "_");
						if (tokens.size() > 2) outfile << " "
							<< i+1 << " p(" << tokens[1] << "," << tokens[2] <<") P\n";
						else outfile << " " << i+1 << " p(" << tokens[1] <<") P\n";
					}
					else outfile << " " << i+1 << " " << traces.at(i).name <<" P\n";
				}
				else if(traces.at(i).type == 'C') {
					label = traces.at(i).name;
					std::replace(label.begin(), label.end(), '|', '.');
					outfile << " " << i+1 << " " << "@" << label << "[c] C\n";
				}
			}
			outfile << "Values:\n";
			for (int i = 0; i < loopsize; i++) {
				outfile << " " << i << " "
					<< std::fixed << std::scientific << std::setprecision(16)
					<< timesteps->at(i) << "\n";
				for (int j = 0; j < traces.size(); j++) {
					if(traces.at(j).pointer)
						outfile << " " << std::string( Misc::numDigits(i), ' ' )
							<< " " << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).traceData->at(i) << "\n";
					else
						outfile << " " << std::string( Misc::numDigits(i), ' ' )
							<< " " << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).calcData.at(i) << "\n";
				}
			}
			outfile.close();
		}
		else if (traces.empty()) {
			std::cerr << "W: Nothing specified to save."
				<< std::endl;
			std::cerr << std::endl;
			outfile.close();
		}
	}
	else {
		std::cerr << "E: " << outname << " could not be opened for saving."
				<< std::endl;
			std::cerr << std::endl;
	}
}

void
Output::write_cout(Matrix &mObj, Simulation &sObj) {
	int loopsize = 0;
	if(!traces.empty()) {
		if(traces.at(0).pointer)
			loopsize = traces.at(0).traceData->size();
		else
			loopsize = traces.at(0).calcData.size();
		std::cout << "time"
			<< " ";
		for (int i = 0; i < traces.size() - 1; i++) {
			std::cout << traces.at(i).name << " ";
		}
		std::cout << traces.at(traces.size() - 1).name << "\n";
		for (int i = 0; i < loopsize; i++) {
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< timesteps->at(i) << " ";
			for (int j = 0; j < traces.size() - 1; j++) {
				if(traces.at(j).pointer)
					std::cout << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(j).traceData->at(i) << " ";
				else
					std::cout << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(j).calcData.at(i) << " ";
			}
			if(traces.at(traces.size() - 1).pointer)
				std::cout << std::fixed << std::scientific << std::setprecision(16)
					<< traces.at(traces.size() - 1).traceData->at(i) << "\n";
			else
				std::cout << std::fixed << std::scientific << std::setprecision(16)
					<< traces.at(traces.size() - 1).calcData.at(i) << "\n";
		}
	}
	else if (traces.empty()) {
		std::cerr << "W: Nothing specified to output."
			<< std::endl;
		std::cerr << "W: Printing all vectors."
			<< std::endl;
		std::cerr << std::endl;
		std::cout << "time"
			<< " ";
		for (int i = 0; i < mObj.columnNames.size() - 1; i++) {
			std::cout << mObj.columnNames.at(i) << " ";
		}
		std::cout << mObj.columnNames.at(mObj.columnNames.size() - 1) << "\n";
		for (int i = 0; i < sObj.results.xVect.at(0).size(); i++) {
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< timesteps->at(i) << " ";
			for (int j = 0; j < sObj.results.xVect.size() - 1; j++) {
				std::cout << std::fixed << std::scientific << std::setprecision(16)
					<< sObj.results.xVect.at(j).at(i) << " ";
			}
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< sObj.results.xVect.at(sObj.results.xVect.size() - 1).at(i) << "\n";
		}
	}
}
