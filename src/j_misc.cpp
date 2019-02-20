// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_misc.h"

std::string
Misc::file_from_path(const std::string& path)
{
	auto posLastSlash = path.find_last_of("/\\");
	if (posLastSlash == std::string::npos) {
		posLastSlash = 0;
		return path.substr(posLastSlash);
	}
	else
		return path.substr(posLastSlash + 1);
}

bool
Misc::has_suffix(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() &&
		str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

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

std::vector<std::string>
Misc::tokenize_space(const std::string& c)
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

std::vector<std::string>
Misc::tokenize_delimeter(const std::string& c, const std::string& d)
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

int
Misc::map_value_count(std::unordered_map<std::string, int> map, int value)
{
	int counter = 0;
	for (const auto& i : map) {
		if (i.second == value)
			counter++;
	}
	return counter;
}

double
Misc::modifier(const std::string& value)
{
	std::string::size_type sz;
	double number;
	try {
		number = std::stod(value, &sz);
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

void
Misc::unique_push(std::vector<std::string>& vector, const std::string& string)
{
	if (std::find(vector.begin(), vector.end(), string) == vector.end()) {
		vector.push_back(string);
	}
}

int
Misc::index_of(std::vector<std::string> vector, const std::string& value)
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

std::string
Misc::substring_after(const std::string& str, const std::string& whatpart)
{
	std::size_t pos = 0;
	std::string substring;
	if (str.find(whatpart) != std::string::npos)
		pos = str.find(whatpart) + whatpart.size();
	substring = str.substr(pos);
	return substring;
}

std::string
Misc::substring_before(std::string str, const std::string& whatpart)
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

std::vector<double>
Misc::parse_function(std::string &str, Input &iObj, std::string subckt) {
	std::vector<double> functionOfT(iObj.transSim.simsize(), 0.0);
	std::vector<std::string> tokens;
	std::string posVarName; //subckt = "";
	tokens = tokenize_space(str);
	auto first = str.find('(') + 1;
	auto last = str.find(')');
	std::string params = str.substr(first, last - first);
	tokens = tokenize_delimeter(params, " ,");
	if (str.find("PWL") != std::string::npos) {
		if (std::stod(tokens[0]) != 0.0 || std::stod(tokens[1]) != 0.0)
			Errors::function_errors(INITIAL_VALUES, tokens[0] + " & " + tokens[1]);
		std::vector<double> timesteps, values;
		for (int i = 0; i < tokens.size(); i = i + 2) {
				timesteps.push_back(modifier(tokens[i]));
		}
		for (int i = 1; i < tokens.size(); i = i + 2) {
			if (iObj.parameters.parsedParams.count(std::make_pair(tokens[i], subckt)) != 0)
					values.push_back(iObj.parameters.parsedParams.at(std::make_pair(tokens[i], subckt)));
			else
				// values.push_back(modifier(tokens[i]));
				values.push_back(Parser::parse_param(tokens[i], iObj.parameters.parsedParams, subckt));
		}
		if (timesteps.size() < values.size())
			Errors::function_errors(TOO_FEW_TIMESTEPS,
				std::to_string(timesteps.size()) + " timesteps & " +
				std::to_string(timesteps.size()) + " values");
		if (timesteps.size() > values.size())
			Errors::function_errors(TOO_FEW_VALUES,
				std::to_string(timesteps.size()) + " timesteps & " +
				std::to_string(timesteps.size()) + " values");
		if((timesteps.back() > iObj.transSim.tstop) && (values.back() > values[values.size() - 2])) {
			values[values.size() - 1] = (iObj.transSim.tstop / timesteps.back())*(values.back() - values[values.size() - 2]);
			timesteps[timesteps.size() - 1] = iObj.transSim.tstop;
		}
		else if((timesteps.back() > iObj.transSim.tstop) && (values.back() == values[values.size() - 2])) {
			timesteps[timesteps.size() - 1] = iObj.transSim.tstop;
		}
		if (values.at(values.size() - 1) != 0.0) {
			std::fill(functionOfT.begin() +
				timesteps.at(timesteps.size() - 1) / iObj.transSim.prstep,
				functionOfT.end(),
				values.at(values.size() - 1));
		}
		double startpoint, endpoint, value = 0.0;
		for (int i = 1; i < timesteps.size(); i++) {
			startpoint = ceil(timesteps.at(i - 1) / iObj.transSim.prstep);
			endpoint = ceil(timesteps[i] / iObj.transSim.prstep);
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
		vPeak = modifier(tokens[1]);
		if (vPeak == 0.0)
			if (iObj.argVerb)
				Errors::function_errors(PULSE_VPEAK_ZERO, tokens[1]);
		timeDelay = modifier(tokens[2]);
		timeRise = modifier(tokens[3]);
		timeFall = modifier(tokens[4]);
		pulseWidth = modifier(tokens[5]);
		if (pulseWidth == 0.0)
			if (iObj.argVerb)
				Errors::function_errors(PULSE_WIDTH_ZERO, tokens[5]);
		pulseRepeat = modifier(tokens[6]);
		if (pulseRepeat == 0.0)
			if (iObj.argVerb)
				Errors::function_errors(PULSE_REPEAT, tokens[6]);
		int PR = pulseRepeat / iObj.transSim.prstep;
		int TD = timeDelay / iObj.transSim.prstep;
		std::vector<double> timesteps, values;
		double timestep;
		for (int i = 0; i < ((iObj.transSim.simsize() - TD) / PR); i++) {
			timestep = timeDelay + (pulseRepeat * i);
			if (timestep < iObj.transSim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(0.0);
			timestep = timeDelay + (pulseRepeat * i) + timeRise;
			if (timestep < iObj.transSim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(vPeak);
			timestep = timeDelay + (pulseRepeat * i) + timeRise + pulseWidth;
			if (timestep < iObj.transSim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(vPeak);
			timestep =
				timeDelay + (pulseRepeat * i) + timeRise + pulseWidth + timeFall;
			if (timestep < iObj.transSim.tstop)
				timesteps.push_back(timestep);
			else
				break;
			values.push_back(0.0);
		}
		double startpoint, endpoint, value;
		for (int i = 1; i < timesteps.size(); i++) {
			startpoint = ceil(timesteps.at(i - 1) / iObj.transSim.prstep);
			endpoint = ceil(timesteps[i] / iObj.transSim.prstep);
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
		double VO = 0.0, VA = 0.0, TD = 0.0, FREQ = 1/iObj.transSim.tstop, THETA = 0.0;
		VO = modifier(tokens[0]);
		VA = modifier(tokens[1]);
		if (VA == 0.0)
			if (iObj.argVerb)
				Errors::function_errors(SIN_VA_ZERO, tokens[1]);
		if (tokens.size() == 5) {
			FREQ = modifier(tokens[2]);
			TD = modifier(tokens[3]);
			THETA = modifier(tokens[4]);
		}
		else if (tokens.size() == 4) {
			FREQ = modifier(tokens[2]);
			TD = modifier(tokens[3]);
		}
		else if (tokens.size() == 3) {
			FREQ = modifier(tokens[2]);
		}
		double currentTimestep, value;
		int beginTime;
		beginTime = TD / iObj.transSim.prstep;
		for (int i = beginTime; i < iObj.transSim.simsize(); i++) {
			currentTimestep = i * iObj.transSim.prstep;
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
		TS = modifier(tokens[1]);
		SF = modifier(tokens[2]);
		if (SF == 0.0)
			if (iObj.argVerb)
				Errors::function_errors(CUS_SF_ZERO, tokens[2]);
		IM = stoi(tokens[3]);
		if (tokens.size() == 6) {
			TD = modifier(tokens[4]);
			PER = stoi(tokens[5]);
		}
		else if (tokens.size() == 5) {
			TD = modifier(tokens[4]);
		}
		std::ifstream wffile(WFline);
		if (wffile.good()) getline(wffile, WFline);
		else Errors::function_errors(CUS_WF_NOT_FOUND, WFline);
		wffile.close();
		WF = tokenize_delimeter(WFline, " ,;");
		std::vector<double> timesteps, values;
		for (int i = 0; i < WF.size(); i++) {
			values.push_back(modifier(WF[i]) * SF);
			timesteps.push_back(TD + i * TS);
		}
		if(TS < iObj.transSim.prstep) TS = iObj.transSim.prstep;
		double functionSize = (iObj.transSim.tstop - TD)/TS;
		if(PER == 1) {
			double repeats = functionSize / values.size();
			double lastTimestep = timesteps.back();
			for(int j = 0; j < repeats; j++) {
				lastTimestep = timesteps.back() + TS;
				for (int i = 0; i < WF.size(); i++) {
					values.push_back(modifier(WF[i]) * SF);
					timesteps.push_back(lastTimestep + i * TS);
				}
			}
		}
		double startpoint, endpoint, value = 0.0;
		for (int i = 1; i < timesteps.size(); i++) {
			startpoint = ceil(timesteps.at(i - 1) / iObj.transSim.prstep);
			endpoint = ceil(timesteps[i] / iObj.transSim.prstep);
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

std::string Misc::stringSubtract(const std::string& src, const std::string& comp)
{
	int srcIt = src.size() - 1, compIt = comp.size() - 1;
	std::string rslt = src;
	return rslt;
}

int Misc::numDigits(int number) {
    int digits = 0;
    if (number <= 0) digits = 1;
    while (number) {
        number /= 10;
        digits++;
    }
    return digits;
}
