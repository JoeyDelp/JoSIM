// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_output.hpp"

/*
        Function that writes the output file as requested by the user
*/
void
write_data(InputFile& iFile)
{
  std::vector<std::string> traceLabel;
  std::vector<std::vector<double>> traceData;
  traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
  std::ofstream outfile(OUTPUT_PATH);
  if (outfile.is_open()) {
    if (!traceLabel.empty()) {
      outfile << "time"
              << ",";
      for (int i = 0; i < traceLabel.size() - 1; i++) {
        outfile << traceLabel[i] << ",";
      }
      outfile << traceLabel[traceLabel.size() - 1] << "\n";
      for (int i = 0; i < traceData[0].size(); i++) {
        outfile << std::fixed << std::scientific << std::setprecision(16)
                << timeAxis[i] << ",";
        for (int j = 0; j < traceData.size() - 1; j++) {
          outfile << std::fixed << std::scientific << std::setprecision(16)
                  << traceData[j][i] << ",";
        }
        outfile << std::fixed << std::scientific << std::setprecision(16)
                << traceData[traceData.size() - 1][i] << "\n";
      }
      outfile.close();
    } else if (traceLabel.empty()) {
      std::cout << "W: Nothing specified to save. Saving all traces."
                << std::endl;
      outfile << "time"
              << ",";
      for (int i = 0; i < iFile.matA.columnNames.size() - 1; i++) {
        outfile << iFile.matA.columnNames[i] << ",";
      }
      outfile << iFile.matA.columnNames[iFile.matA.columnNames.size() - 1] << "\n";
      for (int i = 0; i < xVect[0].size(); i++) {
        outfile << std::fixed << std::scientific << std::setprecision(16)
                << timeAxis[i] << ",";
        for (int j = 0; j < xVect.size() - 1; j++) {
          outfile << std::fixed << std::scientific << std::setprecision(16)
                  << xVect[j][i] << ",";
        }
        outfile << std::fixed << std::scientific << std::setprecision(16)
                << xVect[xVect.size() - 1][i] << "\n";
      }
      outfile.close();
    }
  }
}
/*
        Function that writes a legacy output file in JSIM_N format
*/
void
write_legacy_data(InputFile& iFile)
{
  std::string label;
  std::vector<std::string> traceLabel, tokens;
  std::vector<std::vector<double>> traceData;
  traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
  std::ofstream outfile(OUTPUT_LEGACY_PATH);
  if (outfile.is_open()) {
    outfile << "time"
            << " ";
    for (int i = 0; i < traceLabel.size() - 1; i++) {
      tokens = tokenize_space(traceLabel[i]);
      label = tokens[0];
      for (int j = 1; j < tokens.size(); j++) label = label + "_" + tokens[j];
      outfile << label << " ";
    }
    tokens = tokenize_space(traceLabel[traceLabel.size() - 1]);
    label = tokens[0];
    for (int j = 1; j < tokens.size(); j++) label = label + "_" + tokens[j];
    outfile << label  << "\n";
    for (int i = 0; i < traceData[0].size(); i++) {
      outfile << std::fixed << std::scientific << std::setprecision(16)
              << timeAxis[i] << " ";
      for (int j = 0; j < traceData.size() - 1; j++) {
        outfile << std::fixed << std::scientific << std::setprecision(16)
                << traceData[j][i] << " ";
      }
      outfile << std::fixed << std::scientific << std::setprecision(16)
              << traceData[traceData.size() - 1][i] << "\n";
    }
    outfile.close();
  }
}