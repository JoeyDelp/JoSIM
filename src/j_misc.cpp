// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_misc.h"

/*
  Returns the file part of a path if a path is present else returns entire path
  (file)
*/
std::string
Misc::file_from_path(std::string path)
{
	auto posLastSlash = path.find_last_of("/\\");
	if (posLastSlash == std::string::npos) {
		posLastSlash = 0;
		return path.substr(posLastSlash);
	}
	else
		return path.substr(posLastSlash + 1);
}
/*
  Returns false if the file is not of specified suffix.
  Dietrich Epp (Dec 7, 2013)
  https://stackoverflow.com/questions/20446201/how-to-check-if-string-ends-with-txt
*/
bool
Misc::has_suffix(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() &&
		str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
/*
  Returns true or false if a string starts with a character ignoring prepended
  spaces
*/
bool
Misc::starts_with(std::string input, char test)
{
	for (auto i : input) {
		if (i != ' ') {
			return i == test;
		}
	}
	return false;
}
/*
  Displays the circuit statistics such as JJ count and component count.
  1 or 0 sets full stats or not. Debug setting only. Not available to user.
*/
void
Misc::circuit_stats(int full, InputFile& iFile)
{
	std::stringstream ss;
	std::cout << "Circuit characteristics:" << std::endl;
	std::cout << std::setw(35) << std::left
		<< "Subcircuits:" << iFile.subCircuitCount << std::endl;
	std::cout << std::endl;
	if (full == 1) {
		for (auto i : iFile.subcircuitSegments) {
			std::cout << "Subcircuit: " << i.first << std::endl;
			std::cout
				<< "*************************************************************"
				<< std::endl;
			for (const auto& j : i.second.lines) {
				std::cout << j << std::endl;
			}
			std::cout
				<< "*************************************************************"
				<< std::endl;
			std::cout << i.first << " component count: "
				<< i.second.componentCount << std::endl;
			std::cout << i.first << " JJ count: " << i.second.jjCount
				<< std::endl;
			std::cout << std::endl;
		}
		std::cout << "Main circuit: " << std::endl;
		std::cout << "*************************************************************"
			<< std::endl;
		for (const auto& i : iFile.maincircuitSegment) {
			std::cout << i << std::endl;
		}
		std::cout << "*************************************************************"
			<< std::endl;
		std::cout << "Main circuit component count: " << iFile.componentCount
			<< std::endl;
		std::cout << "Main circuit JJ count: " << iFile.jjCount << std::endl;
		std::cout << std::endl;
		std::cout << "Simulation control: " << std::endl;
		std::cout << "*************************************************************"
			<< std::endl;
		for (const auto& i : iFile.controlPart) {
			std::cout << i << std::endl;
		}
		std::cout << "*************************************************************"
			<< std::endl;
	}
	else {
		for (auto i : iFile.subcircuitSegments) {
			ss.str(i.first + " component count:");
			std::cout << std::setw(35) << std::left << ss.str()
				<< i.second.componentCount << std::endl;
			ss.str(i.first + " JJ count:");
			std::cout << std::setw(35) << std::left << ss.str()
				<< i.second.jjCount << std::endl;
		}
		std::cout << std::endl;
		std::cout << std::setw(35) << std::left
			<< "Main circuit component count:" << iFile.circuitComponentCount
			<< std::endl;
		std::cout << std::setw(35) << std::left
			<< "Main circuit JJ count:" << iFile.circuitJJCount << std::endl;
		std::cout << std::endl;
	}
}
/*
  Turn string into tokens using only spaces
*/
std::vector<std::string>
Misc::tokenize_space(std::string c)
{
	std::string::size_type pos, lastPos = 0, length = c.length();
	std::string delimiters = " \t";
	std::vector<std::string> tokens;
	bool trimEmpty = true;

	using value_type = typename std::vector<std::string>::value_type;
	using size_type = typename std::vector<std::string>::size_type;

	while (lastPos < length + 1)
	{
		pos = c.find_first_of(delimiters, lastPos);
		if (pos == std::string::npos)
		{
			pos = length;
		}

		if (pos != lastPos || !trimEmpty)
			tokens.push_back(value_type(c.data() + lastPos,
			(size_type)pos - lastPos));

		lastPos = pos + 1;
	}
	return tokens;
}
/*
  Turn string into tokens using any delimiter (slower)
*/
std::vector<std::string>
Misc::tokenize_delimeter(std::string c, std::string d)
{
	std::vector<std::string> tokens;
	std::stringstream stringStream(c);
	std::string line;
	while (std::getline(stringStream, line)) {
		std::size_t prev = 0, pos;
		while ((pos = line.find_first_of(d, prev)) != std::string::npos) {
			if (pos > prev)
				tokens.push_back(line.substr(prev, pos - prev));
			prev = pos + 1;
		}
		if (prev < line.length())
			tokens.push_back(line.substr(prev, std::string::npos));
	}
	return tokens;
}
/*
  Count values equal to specified value in given map
*/
int
Misc::map_value_count(std::unordered_map<std::string, int> map, int value)
{
	int counter = 0;
	for (auto i : map) {
		if (i.second == value)
			counter++;
	}
	return counter;
}
/*
  Takes a string and returns double version with correct modifier
  Eg. "5PS" returns 5E-12
*/
double
Misc::modifier(std::string value, std::string subckt)
{
	std::string::size_type sz;
	double number;
	try {
		if(!std::isdigit(value[0]) && value[0] != '-') {
			number = Parser::parse_return_expression(value, subckt);
			sz = value.size();
		}
		else number = std::stod(value, &sz);
	}
	catch (const std::invalid_argument&) {
		Errors::misc_errors(STOD_ERROR, value);
		throw;
	}
	catch (std::exception &e) {
		Errors::misc_errors(STOD_ERROR, value);
		throw;
	}
	switch (value.substr(sz)[0]) {
		/* mega */
	case 'X':
		return number * 1E6;
		/* mega or milli */
	case 'M':
		/* mega */
		if (value.substr(sz)[1] == 'E' && value.substr(sz)[2] == 'G')
			return number * 1E6;
		/* milli */
		else
			return number * 1E-3;
		/* micro */
	case 'U':
		return number * 1E-6;
		/* nano */
	case 'N':
		return number * 1E-9;
		/* pico */
	case 'P':
		return number * 1E-12;
		/* femto */
	case 'F':
		return number * 1E-15;
		/* kilo */
	case 'K':
		return number * 1E3;
		/* giga */
	case 'G':
		return number * 1E9;
		/* tera */
	case 'T':
		return number * 1E12;
		/* auto modifier */
	case 'E':
		return std::stod(value);
	default:
		return number;
	}
}
/*
  Unique vector push. Only pushes if item being pushed is unique
*/
void
Misc::unique_push(std::vector<std::string>& vector, std::string string)
{
	if (std::find(vector.begin(), vector.end(), string) == vector.end()) {
		vector.push_back(string);
	}
}
/*
  Fetch index of value in vector, if not found return -1
*/
int
Misc::index_of(std::vector<std::string> vector, std::string value)
{
	int counter = 0;
	for (const auto& i : vector) {
		/* Value found, return counter */
		if (value == vector[counter])
			return counter;
		counter++;
	}
	/* Value was not found, set counter equal to -1 and return */
	counter = -1;
	return counter;
}
/*
  Substring from after specified string
*/
std::string
Misc::substring_after(std::string str, std::string whatpart)
{
	std::size_t pos = 0;
	std::string substring;
	if (str.find(whatpart) != std::string::npos)
		pos = str.find(whatpart) + whatpart.size();
	substring = str.substr(pos);
	return substring;
}
/*
Substring from before specified string
*/
std::string
Misc::substring_before(std::string str, std::string whatpart)
{
	std::size_t pos = 0;
	std::string substring;
	if (str.find(whatpart) != std::string::npos) {
		pos = str.find(whatpart);
		substring = str.substr(0, pos);
		return substring;
	}
	else
		return str;
}
/*
  Function parser. Turns a string of pwl, pulse, sin, cos into a vector of
  values for the duration of the simulation.
*/
std::vector<double>
Misc::function_parse(std::string str, InputFile& iFile, std::string subckt) {
	std::vector<double> functionOfT(iFile.tsim.simsize(), 0.0);
	std::vector<std::string> tokens;
	std::string posVarName; //subckt = "";
	tokens = tokenize_space(str);
	// if (tokens[0].find_first_of("|") != std::string::npos) {
	// 	subckt = iFile.subcircuitNameMap.at(tokens[0].substr(tokens[0].find_first_of("|") + 1));
	// }
	/* Identify string parrameter part of the string */
	auto first = str.find('(') + 1;
	auto last = str.find(')');
	std::string params = str.substr(first, last - first);
	tokens = tokenize_delimeter(params, " ,");
	/* Determine type of source and handle accordingly*/
	/* PWL */
	if (str.find("PWL") != std::string::npos) {
		if (std::stod(tokens[0]) != 0.0 || std::stod(tokens[1]) != 0.0)
			Errors::function_errors(INITIAL_VALUES, tokens[0] + " & " + tokens[1]);
		std::vector<double> timesteps, values;
		for (int i = 0; i < tokens.size(); i = i + 2) {
			//if (modifier(tokens[i]) > iFile.tsim.tstop) {
			//	timesteps.push_back(modifier(tokens.at(i - 2)));
			//}
			//else
				timesteps.push_back(modifier(tokens[i]));
		}
		for (int i = 1; i < tokens.size(); i = i + 2) {
			if (iFile.paramValues.paramMap.count(tokens[i]) != 0)
				values.push_back(iFile.paramValues.returnParam(tokens[i], subckt));
			else if (iFile.paramValues.paramMap.count(tokens[i] + "|" + subckt) != 0)
				values.push_back(iFile.paramValues.returnParam(tokens[i], subckt));
			else
				values.push_back(modifier(tokens[i], subckt));
		}
		if (timesteps.size() < values.size())
			Errors::function_errors(TOO_FEW_TIMESTEPS,
				std::to_string(timesteps.size()) + " timesteps & " +
				std::to_string(timesteps.size()) + " values");
		if (timesteps.size() > values.size())
			Errors::function_errors(TOO_FEW_VALUES,
				std::to_string(timesteps.size()) + " timesteps & " +
				std::to_string(timesteps.size()) + " values");
		if((timesteps.back() > iFile.tsim.tstop) && (values.back() > values[values.size() - 2])) {
			values[values.size() - 1] = (iFile.tsim.tstop / timesteps.back())*(values.back() - values[values.size() - 2]);
			timesteps[timesteps.size() - 1] = iFile.tsim.tstop;
		}
		else if((timesteps.back() > iFile.tsim.tstop) && (values.back() == values[values.size() - 2])) {
			timesteps[timesteps.size() - 1] = iFile.tsim.tstop;
		}
		if (values.at(values.size() - 1) != 0.0) {
			std::fill(functionOfT.begin() +
				timesteps.at(timesteps.size() - 1) / iFile.tsim.prstep,
				functionOfT.end(),
				values.at(values.size() - 1));
		}
		double startpoint, endpoint, value = 0.0;
		for (int i = 1; i < timesteps.size(); i++) {
			startpoint = ceil(timesteps.at(i - 1) / iFile.tsim.prstep);
			endpoint = ceil(timesteps[i] / iFile.tsim.prstep);
			functionOfT[startpoint] = values.at(i-1);
			for (int j = (int)startpoint + 1; j < (int)endpoint; j++) {
				if (values.at(i - 1) < values[i])
					if(values.at(i - 1) < 0) value = values.at(i - 1) + (values.at(i) - (values.at(i-1))) / (endpoint - startpoint) * (j - (int)startpoint);
					else value = values[i] / (endpoint - startpoint) * (j - (int)startpoint);
				else if (values.at(i - 1) > values[i])
					value = values.at(i - 1) - ((values.at(i - 1) - (values.at(i))) / (endpoint - startpoint) *
					(j - (int)startpoint));
				else if (values.at(i - 1) == values[i])
					value = values[i];
				functionOfT[j] = value;
			}
		}
	}
	/* PULSE */
	else if (str.find("PULSE") != std::string::npos) {
		if (std::stod(tokens[0]) != 0.0)
			Errors::function_errors(INITIAL_PULSE_VALUE, tokens[0]);
		if (tokens.size() < 7)
			Errors::function_errors(PULSE_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
		double vPeak, timeDelay, timeRise, timeFall, pulseWidth, pulseRepeat;
		vPeak = modifier(tokens[1], subckt);
		if (vPeak == 0.0)
			if (cArg.verbose)
				Errors::function_errors(PULSE_VPEAK_ZERO, tokens[1]);
		timeDelay = modifier(tokens[2], subckt);
		timeRise = modifier(tokens[3], subckt);
		timeFall = modifier(tokens[4], subckt);
		pulseWidth = modifier(tokens[5], subckt);
		if (pulseWidth == 0.0)
			if (cArg.verbose)
				Errors::function_errors(PULSE_WIDTH_ZERO, tokens[5]);
		pulseRepeat = modifier(tokens[6], subckt);
		if (pulseRepeat == 0.0)
			if (cArg.verbose)
				Errors::function_errors(PULSE_REPEAT, tokens[6]);
		int PR = pulseRepeat / iFile.tsim.prstep;
		int TD = timeDelay / iFile.tsim.prstep;
		std::vector<double> timesteps, values;
		double timestep;
		for (int i = 0; i < ((iFile.tsim.simsize() - TD) / PR); i++) {
			timestep = timeDelay + (pulseRepeat * i);
			if (timestep < iFile.tsim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(0.0);
			timestep = timeDelay + (pulseRepeat * i) + timeRise;
			if (timestep < iFile.tsim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(vPeak);
			timestep = timeDelay + (pulseRepeat * i) + timeRise + pulseWidth;
			if (timestep < iFile.tsim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(vPeak);
			timestep =
				timeDelay + (pulseRepeat * i) + timeRise + pulseWidth + timeFall;
			if (timestep < iFile.tsim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(0.0);
		}
		double startpoint, endpoint, value;
		for (int i = 1; i < timesteps.size(); i++) {
			startpoint = ceil(timesteps.at(i - 1) / iFile.tsim.prstep);
			endpoint = ceil(timesteps[i] / iFile.tsim.prstep);
			functionOfT[startpoint] = values.at(i-1);
			for (int j = (int)startpoint + 1; j < (int)endpoint; j++) {
				if (values.at(i - 1) < values[i])
					if(values.at(i - 1) < 0) value = values.at(i - 1) + (values.at(i) - (values.at(i-1))) / (endpoint - startpoint) * (j - (int)startpoint);
					else value = values[i] / (endpoint - startpoint) * (j - (int)startpoint);
				else if (values.at(i - 1) > values[i])
					value = values.at(i - 1) - ((values.at(i - 1) - (values.at(i))) / (endpoint - startpoint) *
					(j - (int)startpoint));
				else if (values.at(i - 1) == values[i])
					value = values[i];
				functionOfT[j] = value;
			}
		}
	}
	/* SIN(VO VA <FREQ <TD <THETA>>>) */	
	else if (str.find("SIN") != std::string::npos) {
		if (tokens.size() < 2)
			Errors::function_errors(SIN_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
		if (tokens.size() > 5)
			Errors::function_errors(SIN_TOO_MANY_ARGUMENTS, std::to_string(tokens.size()));
		double VO = 0.0, VA = 0.0, TD = 0.0, FREQ = 1/iFile.tsim.tstop, THETA = 0.0;
		VO = modifier(tokens[0], subckt);
		VA = modifier(tokens[1], subckt);
		if (VA == 0.0)
			if (cArg.verbose)
				Errors::function_errors(SIN_VA_ZERO, tokens[1]);
		if (tokens.size() == 5) {
			FREQ = modifier(tokens[2], subckt);
			TD = modifier(tokens[3], subckt);
			THETA = modifier(tokens[4], subckt);
		}
		else if (tokens.size() == 4) {
			FREQ = modifier(tokens[2], subckt);
			TD = modifier(tokens[3], subckt);
		}
		else if (tokens.size() == 3) {
			FREQ = modifier(tokens[2], subckt);
		}
		double currentTimestep, value;
		int beginTime;
		beginTime = TD / iFile.tsim.prstep;
		for (int i = beginTime; i < iFile.tsim.simsize(); i++) {
			currentTimestep = i * iFile.tsim.prstep;
			value = VO + VA * sin(2 * M_PI * FREQ * (currentTimestep - TD)) * exp(-THETA * (currentTimestep - TD));
			functionOfT[i] = value;
		}
	}
	/* CUSTOM: CUS(WaveFile.dat TS SF IM <TD PER>) */
	else if (str.find("CUS") != std::string::npos) {
		if (tokens.size() < 2)
			Errors::function_errors(CUS_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
		if (tokens.size() > 5)
			Errors::function_errors(CUS_TOO_MANY_ARGUMENTS, std::to_string(tokens.size()));
		std::string WFline = tokens[0];
		std::vector<std::string> WF;
		double TS = 0.0, SF = 0.0, TD = 0.0;
		int IM = 0, PER = 0;
		TS = modifier(tokens[1], subckt);
		SF = modifier(tokens[2], subckt);
		if (SF == 0.0)
			if (cArg.verbose)
				Errors::function_errors(CUS_SF_ZERO, tokens[2]);
		IM = stoi(tokens[3]);
		if (tokens.size() == 6) {
			TD = modifier(tokens[4], subckt);
			PER = stoi(tokens[5]);
		}
		else if (tokens.size() == 5) {
			TD = modifier(tokens[4], subckt);
		}
		std::ifstream wffile(WFline);
		if (wffile.good()) getline(wffile, WFline);
		else Errors::function_errors(CUS_WF_NOT_FOUND, WFline);
		wffile.close();
		WF = tokenize_delimeter(WFline, " ,;");
		std::vector<double> timesteps, values;
		for (int i = 0; i < WF.size(); i++) {
			values.push_back(modifier(WF[i], subckt) * SF);
			timesteps.push_back(TD + i * TS);
		}
		if(TS < iFile.tsim.prstep) TS = iFile.tsim.prstep;
		double functionSize = (iFile.tsim.tstop - TD)/TS;
		if(PER == 1) {
			double repeats = functionSize / values.size();
			double lastTimestep = timesteps.back();
			for(int j = 0; j < repeats; j++) {
				lastTimestep = timesteps.back() + TS;
				for (int i = 0; i < WF.size(); i++) {
					values.push_back(modifier(WF[i], subckt) * SF);
					timesteps.push_back(lastTimestep + i * TS);
				}
			}
		}
		double startpoint, endpoint, value = 0.0;
		for (int i = 1; i < timesteps.size(); i++) {
			startpoint = ceil(timesteps.at(i - 1) / iFile.tsim.prstep);
			endpoint = ceil(timesteps[i] / iFile.tsim.prstep);
			functionOfT[startpoint] = values.at(i-1);
			for (int j = (int)startpoint + 1; j < (int)endpoint; j++) {
				if (values.at(i - 1) < values[i])
					if(values.at(i - 1) < 0) value = values.at(i - 1) + (values.at(i) - (values.at(i-1))) / (endpoint - startpoint) * (j - (int)startpoint);
					else value = values.at(i - 1) + (values.at(i) - (values.at(i-1)))  / (endpoint - startpoint) * (j - (int)startpoint);
				else if (values.at(i - 1) > values[i])
					value = values.at(i - 1) - ((values.at(i - 1) - (values.at(i))) / (endpoint - startpoint) *
					(j - (int)startpoint));
				else if (values.at(i - 1) == values[i])
					value = values[i];
				functionOfT[j] = value;
			}
		}
	}
	return functionOfT;
}
/*
		Helper function for finding the depth of subcircuits in the design
*/
bool
Misc::findX(std::vector<std::string>& segment, std::string& theLine, int &linePos)
{
	for (int i = linePos; i < segment.size(); i++) {
		if (segment[i][0] == 'X') {
			theLine = segment[i];
			if(i < segment.size() - 1) linePos = i + 1;
			else linePos = segment.size() - 1;
			return true;
		}
	}
	return false;
}
/*
		Function that finds the depth of the subcircuits in the design
*/
int
Misc::subCircuitDepth(std::vector<std::string> segment,
	InputFile& iFile,
	int& thisDepth,
	int& overallDepth)
{
	std::string subcktLine, subcktName;
	std::vector<std::string> tokens;
	int linePos = 0;
	for(auto k : segment) {
		if (findX(segment, subcktLine, linePos)) {
			tokens = tokenize_space(subcktLine);
			thisDepth++;
			if (thisDepth > overallDepth)
				overallDepth = thisDepth;
			// Check if the second token can be identified as a subcircuit name. If yes
			// then
			if (iFile.subcircuitSegments.find(tokens[1]) !=
				iFile.subcircuitSegments.end()) {
				// Identify the type of subcircuit
				subcktName = tokens[1];
			}
			else if (iFile.subcircuitSegments.find(tokens.back()) !=
				iFile.subcircuitSegments.end()) {
				// Identify the type of subcircuit
				subcktName = tokens.back();
			}
			else {
				// The subcircuit name was not found therefore error out
				Errors::invalid_component_errors(MISSING_SUBCIRCUIT_NAME, subcktLine);
			}
			subCircuitDepth(iFile.subcircuitSegments[subcktName].lines,
				iFile,
				thisDepth,
				overallDepth);
		}
		else
			thisDepth = 1;
	}
	return overallDepth;
}
/*
  Compare two strings. Return difference
*/
std::string Misc::stringSubtract(std::string src, std::string comp)
{
	int srcIt = src.size() - 1, compIt = comp.size() - 1;
	std::string rslt = src;
	return rslt;
}

int Misc::numDigits(int number) {
    int digits = 0;
    if (number <= 0) digits = 1; // remove this line if '-' counts as a digit
    while (number) {
        number /= 10;
        digits++;
    }
    return digits;
}