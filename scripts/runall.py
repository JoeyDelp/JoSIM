#!/usr/bin/env python3
import os
import fnmatch
import sys

def main():
    if(len(sys.argv) < 2):
        print("Testing script:")
        print("Usage: runall.py <example_files_location> <analysis_type>")
        print("example_files_location: folder containing circuit files with ex_* pattern")
        print("analysis_type: 0 - Voltage, 1 - Phase, 2 - Both voltage and phase")
    else:
        scriptpath = os.path.dirname(os.path.realpath(__file__))
        if os.name == 'nt':
            josimpath = '{0}/../build/Release/josim-cli.exe'.format(scriptpath)
            nullchar = '> NULL 2>&1'
        else:
            josimpath = '{0}/../build/josim-cli'.format(scriptpath)
            nullchar = '&> /dev/null'
        path = '{0}/{1}'.format(os.getcwd(), sys.argv[1]);
        pattern = 'ex_*.cir'

        simulation = 2
        if(len(sys.argv) < 3):
            print("Testing voltage and phase mode")
            print("==============================")
        else:
            if(sys.argv[2] == "0"):
                simulation = 0
                print("Testing voltage mode")
                print("====================")
            elif(sys.argv[2] == "1"):
                simulation = 1
                print("Testing phase mode")
                print("==================")
            elif(sys.argv[2] == "2"):
                simulation = 2
                print("Testing voltage and phase mode")
                print("==============================")

        ex_files = [f for f in fnmatch.filter(os.listdir(path), pattern)]
        ex_files.sort()
        failcounter = 0
        counter = 0
        for f in ex_files:
            file = os.path.join(path, f)
            pre, ext = os.path.splitext(file)
            if(simulation == 0):
                csvfile = pre + '.csv'
                if("_wr" in file):
                    cmd = '{0} -c 1 -o {1} {2} {3}'.format(josimpath, csvfile, file, nullchar)
                else:
                    cmd = '{0} -o {1} {2} {3}'.format(josimpath, csvfile, file, nullchar)
                returned_value = os.system(cmd)
                if(returned_value == 0):
                    counter = counter + 1
                    print('Examples passed: {0}/{1}'.format(counter, len(ex_files)), end='\r')
                else:
                    print('FAIL:', f)
            elif(simulation == 1):
                csvfile = pre + '_phase.csv'
                if("_wr" in file):
                    cmd = '{0} -c 1 -o {1} {2} -a 1 {3}'.format(josimpath, csvfile, file, nullchar)
                else:
                    cmd = '{0} -o {1} {2} -a 1 {3}'.format(josimpath, csvfile, file, nullchar)
                returned_value = os.system(cmd)
                if(returned_value == 0):
                    counter = counter + 1
                    print('Examples passed: {0}/{1}'.format(counter, len(ex_files)), end='\r')
                else:
                    print('FAIL:', f)
            elif(simulation == 2):
                csvfile = pre + '.csv'
                if("_wr" in file):
                    cmd = '{0} -c 1 -o {1} {2} {3}'.format(josimpath, csvfile, file, nullchar)
                else:
                    cmd = '{0} -o {1} {2} {3}'.format(josimpath, csvfile, file, nullchar)
                returned_value = os.system(cmd)
                if(returned_value == 0):
                    counter = counter + 1
                    print('Examples passed: {0}/{1}'.format(counter, len(ex_files)*2), end='\r')
                else:
                    print('FAIL:', f)
                csvfile = pre + '_phase.csv'
                if("_wr" in file):
                    cmd = '{0} -c 1 -o {1} {2} -a 1 {3}'.format(josimpath, csvfile, file, nullchar)
                else:
                    cmd = '{0} -o {1} {2} -a 1 {3}'.format(josimpath, csvfile, file, nullchar)
                returned_value = os.system(cmd)
                if(returned_value == 0):
                    counter = counter + 1
                    print('Examples passed: {0}/{1}'.format(counter, len(ex_files)*2), end='\r')
                else:
                    print('FAIL:', f)
    print()


if __name__== "__main__":
  main()
