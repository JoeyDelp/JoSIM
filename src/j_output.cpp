// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_output.h"

/*
	 Function that writes the output file as requested by the user
*/
void
Output::write_data(InputFile& iFile) {
	std::vector<std::string> traceLabel;
	std::vector<std::vector<double>> traceData;
	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	std::ofstream outfile(cArg.outName);
	if (outfile.is_open()) {
		if (!traceLabel.empty()) {
			outfile << "time"
				<< ",";
			for (int i = 0; i < traceLabel.size() - 1; i++) {
				outfile << traceLabel[i] << ",";
			}
			outfile << traceLabel.at(traceLabel.size() - 1) << "\n";
			for (int i = 0; i < traceData[0].size(); i++) {
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< iFile.timeAxis[i] << ",";
				for (int j = 0; j < traceData.size() - 1; j++) {
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traceData[j][i] << ",";
				}
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< traceData.at(traceData.size() - 1)[i] << "\n";
			}
			outfile.close();
		}
		else if (traceLabel.empty()) {
			std::cout << "W: Nothing specified to save. Saving all traces."
				<< std::endl;
			outfile << "time"
				<< ",";
			for (int i = 0; i < iFile.matA.columnNames.size() - 1; i++) {
				outfile << iFile.matA.columnNames[i] << ",";
			}
			outfile << iFile.matA.columnNames.at(iFile.matA.columnNames.size() - 1) << "\n";
			for (int i = 0; i < iFile.xVect[0].size(); i++) {
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< iFile.timeAxis[i] << ",";
				for (int j = 0; j < iFile.xVect.size() - 1; j++) {
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< iFile.xVect[j][i] << ",";
				}
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< iFile.xVect.at(iFile.xVect.size() - 1)[i] << "\n";
			}
			outfile.close();
		}
	}
}
/*
	Function that writes a legacy output file in JSIM_N format
*/
void
Output::write_legacy_data(InputFile& iFile) {
	std::string label;
	std::vector<std::string> traceLabel, tokens;
	std::vector<std::vector<double>> traceData;
	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	std::ofstream outfile(cArg.outName);
	if (outfile.is_open()) {
		outfile << "time"
			<< " ";
		for (int i = 0; i < traceLabel.size() - 1; i++) {
			tokens = Misc::tokenize_space(traceLabel[i]);
			label = tokens[0];
			for (int j = 1; j < tokens.size(); j++) label = label + "_" + tokens[j];
			outfile << label << " ";
		}
		tokens = Misc::tokenize_space(traceLabel.at(traceLabel.size() - 1));
		label = tokens[0];
		for (int j = 1; j < tokens.size(); j++) label = label + "_" + tokens[j];
		outfile << label << "\n";
		for (int i = 0; i < traceData[0].size(); i++) {
			outfile << std::fixed << std::scientific << std::setprecision(16)
				<< iFile.timeAxis[i] << " ";
			for (int j = 0; j < traceData.size() - 1; j++) {
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< traceData[j][i] << " ";
			}
			outfile << std::fixed << std::scientific << std::setprecision(16)
				<< traceData.at(traceData.size() - 1)[i] << "\n";
		}
		outfile.close();
	}
}
/*
	Function that writes a wr output file for opening in WRspice
*/
void 
Output::write_wr_data(InputFile &iFile) {
	std::string label;
	std::vector<std::string> traceLabel, tokens;
	std::vector<std::vector<double>> traceData;
	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	std::ofstream outfile(cArg.outName);
	if (outfile.is_open()) {
		outfile << "Title: CKT1\n";
		std::time_t result = std::time(nullptr);
		outfile << "Date: " << std::asctime(std::localtime(&result));
		outfile << "Plotname: Transient analysis JoSIM\n";
		outfile << "Flags: real\n";
		outfile << "No. Variables: " << traceData.size() + 1 << "\n";
		outfile << "No. Points: " << traceData.at(0).size() << "\n";
		outfile << "Command: version 4.3.8\n";
		outfile << "Variables:\n";
		outfile << " 0 time S\n";
		for (int i = 0; i < traceLabel.size(); i++) {
			tokens = Misc::tokenize_space(traceLabel[i]);
			if(tokens[0] == "NODE") {
				if(tokens[1] == "VOLTAGE") {
					if (tokens.size() > 3) outfile << " " 
						<< i+1 << " v(" << tokens[2] << "," << tokens[4] <<") V\n";
					else outfile << " " << i+1 << " v(" << tokens[2] <<") V\n";
				}
				else if(tokens[1] == "PHASE") {
					if (tokens.size() > 3) outfile << " " 
						<< i+1 << " p(" << tokens[2] << "," << tokens[4] <<") P\n";
					else outfile << " " << i+1 << " p(" << tokens[2] <<") P\n";
				}
			}
			else if(tokens[0] == "DEVICE") {
				if(tokens[1] == "VOLTAGE") {
					outfile << " " << i+1 << " " << tokens[2] <<" V\n";
				}
				else if(tokens[1] == "CURRENT") {
					outfile << " " << i+1 << " " << tokens[2] <<" C\n";
				}
				else if(tokens[1] == "PHASE") {
					outfile << " " << i+1 << " " << tokens[2] <<" P\n";
				}
			}
			else if(tokens[0] == "PHASE") {
				outfile << " " << i+1 << " " << tokens[1] <<" P\n";
			}
			else if(tokens[0] == "CURRENT") {
				std::replace(tokens[1].begin(), tokens[1].end(), '|', '.');
				outfile << " " << i+1 << " " << "@" << tokens[1] << "[c] C\n"; 
			}
		}
		outfile << "Values:\n";
		for (int i = 0; i < traceData[0].size(); i++) {
			outfile << " " << i << " " 
				<< std::fixed << std::scientific << std::setprecision(16)
				<< iFile.timeAxis[i] << "\n";
			for (int j = 0; j < traceData.size(); j++) {
				outfile << " " << std::string( Misc::numDigits(i), ' ' ) 
					<< " " << std::fixed << std::scientific << std::setprecision(16)
					<< traceData.at(j).at(i) << "\n";
			}
		}
		outfile.close();
	}
}
/*
	Function that writes the output to cout as requested by the user
*/
void
Output::write_cout(InputFile& iFile) {
	std::vector<std::string> traceLabel;
	std::vector<std::vector<double>> traceData;
	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
	if (!traceLabel.empty()) {
		std::cout << "time"
			<< " ";
		for (int i = 0; i < traceLabel.size() - 1; i++) {
			std::cout << traceLabel[i] << " ";
		}
		std::cout << traceLabel.at(traceLabel.size() - 1) << "\n";
		for (int i = 0; i < traceData[0].size(); i++) {
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< iFile.timeAxis[i] << " ";
			for (int j = 0; j < traceData.size() - 1; j++) {
				std::cout << std::fixed << std::scientific << std::setprecision(16)
					<< traceData[j][i] << " ";
			}
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< traceData.at(traceData.size() - 1)[i] << "\n";
		}
	}
	else if (traceLabel.empty()) {
		std::cout << "W: Nothing specified to output."
			<< std::endl;
	}
}
