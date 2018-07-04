// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_MISC_H
#define J_MISC_H
#include "j_std_include.hpp"
class InputFile;

/*
  Returns the filename from a path
*/
std::string
file_from_path(std::string path);
/*
  Returns true if the file has the specified suffix
*/
bool
has_suffix(const std::string& str, const std::string& suffix);
/*
  Tests wether a string strarts with a specified character ignoring prepended
  spaces
*/
bool
starts_with(std::string input, char test);
/*
  Display circuit stats, VERBOSE makes full 1, else minimal
*/
void
circuit_stats(int full, InputFile& iFile);
/*
  Returns a vector of tokens for a given string
*/
std::vector<std::string>
tokenize_space(std::string c);
/*
  Returns a vector of tokens for a given string given delimeters
*/
std::vector<std::string>
tokenize_delimeter(std::string c, std::string d);
/*
  Returns the number of occurences of a value within a map (not the key)
*/
int
map_value_count(std::unordered_map<std::string, int> map, int value);
/*
  Returns the double value of a given string with the correct modifier
*/
double
modifier(std::string value);
/*
  Unique vector push. Only pushes if item being pushed is unique
*/
void
unique_push(std::vector<std::string>& vector, std::string string);
/*
  Fetch index of value in vector, if not found return -1
*/
int
index_of(std::vector<std::string> vector, std::string value);
/*
  Substring from after specified string
*/
std::string
substring_after(std::string str, std::string whatpart);
/*
Substring from before specified string
*/
std::string
substring_before(std::string str, std::string whatpart);
/*
Function parser. Turns a string of pwl, pulse, sin, cos into a vector of values
for the duration of the simulation.
*/
std::vector<double>
function_parse(std::string str, InputFile& iFile);
/*
Helper function for finding the depth of subcircuits in the design
*/
bool
findX(std::vector<std::string>& segment, std::string& theLine);
/*
Function that finds the depth of the subcircuits in the design
*/
int
subCircuitDepth(std::vector<std::string> segment,
                InputFile& iFile,
                int& thisDepth,
                int& overallDepth);
/*
Template function that swaps the key and value of a pair
*/
template<typename A, typename B>
std::pair<B, A>
flip_pair(const std::pair<A, B>& p)
{
  return std::pair<B, A>(p.second, p.first);
}
template<typename A, typename B>
std::map<B, A>
flip_map(const std::unordered_map<A, B>& src)
{
  std::map<B, A> dst;
  std::transform(
    src.begin(), src.end(), std::inserter(dst, dst.begin()), flip_pair<A, B>);
  return dst;
}
/*
Template function that does to_string but with precision
*/
template<typename T>
std::string
precise_to_string(const T a_value, const int n = 30)
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(n) << a_value;
  return out.str();
}
/*
  Compare two strings. Return difference
*/
std::string stringSubtract(std::string src, std::string comp);
#endif