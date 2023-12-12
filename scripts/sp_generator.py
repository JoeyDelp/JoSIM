#!/usr/bin/env python
import pandas as pd
import math
import os, sys, argparse
from tabulate import tabulate

# Main function
def main():
    vers = "SP file generator - 0.1.0 - Specification file generation script"

    # Initiate the parser
    parser = argparse.ArgumentParser(description=vers)

    # Add possible parser arguments
    parser.add_argument("input", help="the CSV input file")
    parser.add_argument("-o", "--output", help="the sp file output name. Default: input with sp extension")
    parser.add_argument("-t", "--threshold", help="the phase jump threshold as fraction of 2pi. Default: 0.8", type=float)
    parser.add_argument("-s", "--stability", help="time in picoseconds to wait for stability after phase jump. Default: 20E-12", type=float)
    parser.add_argument("-v", "--verbose", help="display sp values in command line", action="store_true")
    parser.add_argument("-V", "--version", action='version', help="show script version", version=vers)

    # Read arguments from the command line
    args = parser.parse_args()

    # Check for output
    if not args.output:
        args.output = os.path.splitext(args.input)[0]+'.sp'

    # Check for threshold
    if not args.threshold:
        args.threshold = 0.8

    # Check for stability time
    if not args.stability:
        args.stability = 20E-12

    # The Pandas data frame where the read in file is stored
    csvfile = pd.read_csv(args.input, delimiter = ',')

    # Extract the column header names from the CSV file (time, P(B1), P(B2), etc)
    columnHeaders = list(csvfile.columns)
    # Strip the "P(" and ")" from the header names, leaving only time, B1, B2, etc
    for i in range(1, csvfile.columns.size):
        columnHeaders[i] = csvfile.columns[i][csvfile.columns[i].find("(")+1:csvfile.columns[i].find(")")]


    # Set the offset start point to 0
    offseprstart = 0
    # Create a dictionary in which we will store the initial phase offset
    phaseOffset = {}
    # Create a list of rows which will eventually populate the SP file
    data = []
    datarow = []
    # Average timestep size
    avgTimestep = 0
    # Loop through each row (timestep) in the CSV file
    for i in range(0, csvfile.shape[0]):
        # If the timestep reaches stability (usually around 20ps)
        if(math.isclose(csvfile.loc[i, 'time'], 20.0E-12)):
            # Take note of the position (counter) where the offset was found
            offseprstart = i
            # Store the timestep at this point (we know it's 20ps but we want it from the source)
            datarow = [csvfile.loc[i, 'time']]
            # Loop through each phase column
            for j in range(1, csvfile.columns.size):
                # Store the phase offset for each of the junctions
                phaseOffset[csvfile.columns[j]] = csvfile.loc[i, csvfile.columns[j]]
                # Append to the data row the phase offset - the offset divided by 2pi, this should always be 0
                datarow.append(int((csvfile.loc[i, csvfile.columns[j]] - phaseOffset[csvfile.columns[j]]) / (2 * math.pi)))
            # Calculate the average timestep
            avgTimestep = csvfile.loc[i, 'time'] / i
            # Break out of the for loop to stop unnecessary continuation
            break
    # Append this data row to the list of rows
    data.append(datarow)

    # We need to wait an specified time for a pulse to stabilize before we can count it
    stabilityCount = math.ceil(args.stability / avgTimestep)
    # Mage a copy of the last row create (20E-12 0 0 0 0 etc)
    rowPrevious = datarow.copy()
    # Create a dictionary of flags to set if a jump was identified
    jumpFlags = {}
    # Create a counter dictionary to count to a stabilized phase value after jump is detected
    stabilityCounter = {}
    # Set the counters to 0 and flags to False
    for j in range(1, csvfile.columns.size):
        stabilityCounter[csvfile.columns[j]] = 0
        jumpFlags[csvfile.columns[j]] = False
    # Start looping through each timestep from the point where offset was idenitified
    for i in range(offseprstart, csvfile.shape[0]):
        # Set the time value for this jump value
        dataRow = [csvfile.loc[i, 'time']]
        # Loop through each column to see if a jump was detected
        for j in range(1, csvfile.columns.size):
            # Calculate the jump value truncated to the integer (5.8 = 5, -0.3 = 0, -19.8 = -19)
            currJump = cust_round((csvfile.loc[i, csvfile.columns[j]] - phaseOffset[csvfile.columns[j]]) / (2 * math.pi), args.threshold)
            # Set jump value for this junction
            dataRow.append(currJump)
            # If the current jump value is greater than the previous jump value
            if (abs(currJump) > abs(rowPrevious[j])):
                # Set a flag to indicate a jump happened
                jumpFlags[csvfile.columns[j]] = True
        # Flag if value should be stored
        shouldStore = True
        # Loop through the column names
        for j in range(1, csvfile.columns.size):
            # If any of the counters reached stability count
            if stabilityCounter[csvfile.columns[j]] == stabilityCount:
                # Make sure the rest are at either stability count or 0
                for k in range(1, csvfile.columns.size):
                    if 0 < stabilityCounter[csvfile.columns[k]] < stabilityCount:
                        shouldStore = False
                # If all other values are either stability count or 0, store the value
                if shouldStore:
                    data.append(dataRow)
                # Reset the flag and counter
                stabilityCounter[csvfile.columns[j]] = 0
                jumpFlags[csvfile.columns[j]] = False
        # Loop through the column names again
        for j in range(1, csvfile.columns.size):
            # If the jump flag is set, increment the counter
            if jumpFlags[csvfile.columns[j]]:
                stabilityCounter[csvfile.columns[j]] = stabilityCounter[csvfile.columns[j]] + 1
        # Set the previous row variable to the current row
        rowPrevious = dataRow.copy()
    # Create a Pandas dataframe from the list of data rows and stripped column names
    sp = pd.DataFrame(data, columns = columnHeaders)
    # Remove any duplicate stored time rows
    sp = sp.drop_duplicates(['time'], keep='last')

    # Print sp if verbos
    if args.verbose:
        print(sp)

    # Tabulate the contents for a more pleasant viewing experience
    content = tabulate(sp.values.tolist(), list(sp.columns), tablefmt="plain")
    # Write the content to output file
    open(args.output, "w").write(content)

# Custom threshold rounding function
def cust_round(number, threshold):
    # If the absolute value of the number minus the absolute value of the integer part is larger than the threshold
    if (abs(number) - abs(math.trunc(number)) > threshold):
        # If positive return the integer number + 1
        if number > 0:
            return math.trunc(number) + 1
        # If negative return the integer number - 1
        else:
            return math.trunc(number) - 1
    # If not then just return the integer number
    else:
        return math.trunc(number)

if __name__ == '__main__':
    main()