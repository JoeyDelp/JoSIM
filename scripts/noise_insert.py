#!/usr/bin/env python
# Import relevant packages
import os, math, sys, argparse

# Main function
def main():
  # Version info
  vers = "Noise insertion script - v1.0 - Adds white noise to resistors based on temperature"

  # Initiate the parser
  parser = argparse.ArgumentParser(description=vers)

  # Add possible parser arguments
  parser.add_argument("input", help="the CIR input file")
  parser.add_argument("-b", "--bandwidth", help="noise effective bandwidth. Default is 1 THz", default=1E12)
  parser.add_argument("-t", "--temperature", help="temperature at which noise will be calculated. Default is 4.2 Kelvin", default=4.2)
  parser.add_argument("-o", "--output", help="the output file name. Default is <name>_noise.cir")
  parser.add_argument("-v", "--version", action='version', help="show script version", version=vers)

  # Read arguments from the command line
  args = parser.parse_args()

  if(args.output == None):
    args.output = os.path.splitext(args.input)[0] + "_noise" + os.path.splitext(args.input)[1]

  T = float(args.temperature)
  B = float(args.bandwidth)
  Boltzmann = 1.38064852E-23
  
  netlist_file = open(args.input, "r")
  netlist = []

  i = 0
  while True:
    i += 1
    line = netlist_file.readline()
    if not line: 
        break
    line = line.rstrip('\n')
    netlist.append(line)
    tokens = line.split()
    if tokens[0][0] == 'R':
      R = float(tokens[3])
      currentsource = []
      currentsource.append("INOISE_" + tokens[0])
      currentsource.append(tokens[1])
      currentsource.append(tokens[2])
      noise = math.sqrt(4 * Boltzmann * T / R)
      currentsource.append("NOISE(" + str(noise) + " 0 " + str(1/B) + ")")
      netlist.append("\t".join(currentsource))

  netlist_file.close()

  with open(args.output, 'w') as filehandle:
    filehandle.writelines("%s\n" % line for line in netlist)

if __name__ == '__main__':
  main()