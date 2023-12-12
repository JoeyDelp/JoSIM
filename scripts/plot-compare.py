#!/usr/bin/env python
# Import relevant packages
import os, math, sys, argparse
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import pandas as pd
import plotly

# Main function
def main():
  # Version info
  vers = "JoSIM Comapre Plot - 1.0 - CSV/DAT plot comparison script"

  # Initiate the parser
  parser = argparse.ArgumentParser(description=vers)

  # Add possible parser arguments
  parser.add_argument("input1", help="the first CSV input file")
  parser.add_argument("input2", help="the second CSV input file")
  parser.add_argument("-o", "--output", help="the output file name with supported extensions: png, jpeg, webp, svg, eps, pdf")
  parser.add_argument("-x", "--html", help="save the output as an html file for later viewing")
  parser.add_argument("-s", "--subset", nargs='+', help="subset of traces to plot. specify list of column names (as shown in csv file header), ie. \"V(1)\" \"V(2)\". Default: None")
  parser.add_argument("-c", "--color", help="set the output plot color scheme to one of the following: light, dark, presentation. Default: dark", default='dark')
  parser.add_argument("-w", "--title", help="set plot title to the provided string")
  parser.add_argument("-V", "--version", action='version', help="show script version", version=vers)

  # Read arguments from the command line
  args = parser.parse_args()

  outformats = [".png", ".jpeg", ".webp", ".svg", ".eps", ".pdf"]

  if (os.path.splitext(args.input1)[1] == ".csv"):
    df = pd.read_csv(args.input1, sep=',')
  elif (os.path.splitext(args.input1)[1] == ".dat"):
    df = pd.read_csv(args.input1, delim_whitespace=True)
  else:
    print("Invalid first input file specified: " + args.input1)
    print("Please provide either .csv (comma seperated) or .dat (space seperated) file")
    sys.exit()

  if (os.path.splitext(args.input2)[1] == ".csv"):
    df2 = pd.read_csv(args.input2, sep=',')
  elif (os.path.splitext(args.input2)[1] == ".dat"):
    df2 = pd.read_csv(args.input2, delim_whitespace=True)
  else:
    print("Invalid second input file specified: " + args.input2)
    print("Please provide either .csv (comma seperated) or .dat (space seperated) file")
    sys.exit()  

  fig = stacked_layout(df, df2, args.subset)

  if(args.color == 'light'):
    template = 'plotly_white'
  elif(args.color == 'dark'):
    template = 'plotly_dark'
  elif(args.color == 'presentation'):
    template = 'presentation'
  else:
    print("Invalid plot color specified: " + args.color)
    print("Please provide either light, dark or presentation as color theme")
    sys.exit()
    
  if(args.title == None):
    title=os.path.splitext(os.path.basename(args.input1))[0] + " vs. " + os.path.splitext(os.path.basename(args.input2))[0]
  else:
    title=args.title

  fig.update_layout(
    title=title,
    title_font_size=30,
    template=template
  )
  config = dict({
    'scrollZoom': True,
    'displaylogo': False
  })

  if(args.output == None and args.html == None):
    fig.show(config=config)
  elif(args.html != None and args.output == None):
    fig.write_html(args.html)
  elif(args.html == None and os.path.splitext(args.output)[1] in outformats):
    fig.write_image(args.output, width=3508, height=2480)
  else:
    print("Unknown file format for output file specified.")
    print("Please use: png, jpeg, webp, svg, eps or pdf")  

# Function that sets the Y-axis title relevant to the data
def y_axis_title(figLabel):
  if figLabel[0] == 'V':
    return "Voltage (volts)"
  elif figLabel[0] == 'I':
    return "Current (ampere)"
  elif figLabel[0] == 'P':
    return "Phase (radians)"
  else:
    return "Unknown"

# Return a stack of plots
def stacked_layout(df, df2, subset):
  plots = df.columns[1:].tolist() if subset == None else subset
  fig = make_subplots(
    rows=len(plots), 
    cols=1,
    subplot_titles=plots,
    vertical_spacing=0.2/math.ceil(len(plots)/2),
    x_title='Time (seconds)')
  # Add the traces
  for i in range(0, len(plots)):
    fig.add_trace(go.Scatter(
      x=df.iloc[:,0], y=df.loc[:,plots[i]],
      mode='lines',
      name=plots[i]),
      row=i + 1,
      col=1
    )
    if plots[i] in df2:
      fig.add_trace(go.Scatter(
        x=df.iloc[:,0], y=df2.loc[:,plots[i]],
        mode='lines',
        name=plots[i]),
        row=i + 1,
        col=1
      )
    fig.layout.annotations[i].x = 0.985
    if i == 0:
      fig['layout']['yaxis']['title']=y_axis_title(plots[i])
    else:
      fig['layout']['yaxis' + str(i+1)]['title']=y_axis_title(plots[i])
  return fig

if __name__ == '__main__':
  main()
