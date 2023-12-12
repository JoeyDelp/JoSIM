#!/usr/bin/env python

# Plot output from JoSIM using the plotly package
# Example call from a Python program:
#   plotscript = /<path>/josim-plot2.py
#   josim_output_file = /<path>/<name>.csv  # or .dat
#   output = subprocess.run([plotscript, josim_output_file, "-t", "combined", "-c", "light", "-j", "2pi", "-w", caption], capture_output=False, text=False)
# Note: Arguments must be provided separately, not as a combined string.

# Import relevant packages
import os, math, sys, argparse
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import pandas as pd
import numpy as np
import plotly

# Main function
def main():
  # Version info
  vers = "JoSIM Trace Plotting Script - 1.3 - CSV/DAT plotting script"

  # Initiate the parser
  parser = argparse.ArgumentParser(description=vers)

  # Add possible parser arguments
  parser.add_argument("input", help="the CSV input file")
  parser.add_argument("-o", "--output", help="the output file name with supported extensions: png, jpeg, webp, svg, eps, pdf")
  parser.add_argument("-d", "--dimensions", help="the dimensions of the output file")
  parser.add_argument("-x", "--html", help="save the output as an html file for later viewing")
  parser.add_argument("-t", "--type", help="type of plot: grid, stacked, combined, square, sep_comb. Default: grid", default="grid")
  parser.add_argument("-s", "--subset", nargs='+', help="subset of traces to plot. specify list of column names (as shown in csv file header), ie. \"V(1)\" \"V(2)\". Default: None")
  parser.add_argument("-c", "--color", help="set the output plot color scheme to one of the following: light, dark, presentation. Default: dark", default='dark')
  parser.add_argument("-w", "--title", help="set plot title to the provided string")
  parser.add_argument("-V", "--version", action='version', help="show script version", version=vers)
  parser.add_argument("-j", "--jump", help="plot in phase jump units: 0.5pi, pi, 2pi", default="rad")

  # Read arguments from the command line
  args = parser.parse_args()

  # List of possible output formats
  outformats = [".png", ".jpeg", ".webp", ".svg", ".eps", ".pdf"]

  # How to handle column seperation. CSV uses comma, DAT uses space.
  if (os.path.splitext(args.input)[1].lower() == ".csv"):
    df = pd.read_csv(args.input, sep=',')
  elif (os.path.splitext(args.input)[1].lower() == ".dat"):
    df = pd.read_csv(args.input, delim_whitespace=True)
  else:
    print("Invalid input file specified: " + args.input)
    print("Please provide either .csv (comma seperated) or .dat (space seperated) file")
    sys.exit()

  # Determine the plot layout.
  if(args.type == "grid"):
    fig = grid_layout(df, args)
  elif(args.type == "stacked"):
    fig = stacked_layout(df, args)
  elif(args.type == "combined"):
    fig = combined_layout(df, args)
  elif(args.type == "square"):
    fig = square_layout(df, args)
  elif(args.type == "sep_comb"):
    fig = seperate_combined_layout(df, args)  
  else:
    print("Invalid plot type specified: " + args.type)
    print("Allowed plot type codes: grid, stacked, combined, square, sep_comb")
    sys.exit()
    
  # Determine the theme to plot with.
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
    
  # Set the title of the plot
  if(args.title == None):
    title=os.path.splitext(os.path.basename(args.input))[0]
  else:
    title=args.title

  # Update the layout based on the settings
  fig.update_layout(
    title=title,
    title_font_size=30,
    template=template
  )

  # Set the mode bar buttons
  config = dict({
    'scrollZoom': True,
    'displaylogo': False,
    'toImageButtonOptions': {
        'format': 'svg',
        'filename': title,
        'height': None,
        'width': None
    },
    'modeBarButtonsToAdd': [
        'toggleSpikelines',
        'hovercompare',
        'v1hovermode'
    ]
  })

  # Determine wether to show the plot or just dump to file
  if(args.output == None and args.html == None):
    fig.show(config=config)
  elif(args.html != None and args.output == None):
    fig.write_html(args.html)
  elif(args.html == None and os.path.splitext(args.output)[1] in outformats):
    w, h = args.dimensions.split("x")
    fig.write_image(args.output, width=w, height=h)
  else:
    print("Unknown file format for output file specified.")
    print("Please use: png, jpeg, webp, svg, eps or pdf")  

# Function to provide the appropriate Y-axis title
def y_axis_title(figLabel, args):
  if figLabel[0] == 'V':
    return "Voltage (V)"
  elif figLabel[0] == 'I':
    return "Current (I)"
  elif figLabel[0] == 'P':
    # Phase or phase jumps
    if args.jump == 'rad':
      y_label_P = "Phase (rad)"
    elif args.jump == '0.5pi':
      y_label_P = "Phase jumps (rad/0.5pi)"
    elif args.jump == 'pi':
      y_label_P = "Phase jumps (rad/pi)"
    elif args.jump == '2pi':
      y_label_P = "Phase jumps (rad/2pi)"
    else:
      y_label_P = "Phase (rad)"
    return y_label_P
  else:
    return "Unknown"

# Function: Phase factor to plot phase or phase jumps
def pfact(jump):
  if jump == 'rad':
    return 1.0
  elif jump == '0.5pi':
    return 1/(0.5*np.pi)
  elif jump == 'pi':
    return 1/(1.0*np.pi)
  elif jump == '2pi':
    return 1/(2.0*np.pi)
  else:
    return 1.0
  
# Return a grid of plots
def grid_layout(df, args):
  plots = df.columns[1:].tolist() if args.subset == None else args.subset
  fig = make_subplots(
    rows=math.ceil(len(plots)/2), 
    cols= 2 if len(plots) > 1 else 1,
    subplot_titles=plots,
    horizontal_spacing=0.075,
    vertical_spacing=0.2/math.ceil(len(plots)/2),
    x_title= 'Time (seconds)')
  # Add the traces
  for i in range(0, len(plots)):
    col = 1 if (i%2 == 0) else 2
    row = int(math.floor(i / 2)) + 1
    fig.add_trace(go.Scatter(
      x=df.iloc[:,0], y=df.loc[:,plots[i]],
      mode='lines',
      name=plots[i]),
      row=row,
      col=col
    )
    fig.layout.annotations[i].x = 0.45 if (i%2 == 0) else 0.985
    if i == 0:
      fig['layout']['yaxis']['title']=y_axis_title(plots[i], args)
    else:
      fig['layout']['yaxis' + str(i+1)]['title']=y_axis_title(plots[i], args)
  return fig

# Return a square of plots
def square_layout(df, args):
  plots = df.columns[1:].tolist() if args.subset == None else args.subset
  square = math.sqrt(len(plots))
  row = int(round(square))
  col = int(math.ceil(square))
  fig = make_subplots(
    rows=row, 
    cols=col,
    subplot_titles=plots,
    horizontal_spacing=(0.25/row),
    vertical_spacing=(0.25/col),
    x_title= 'Time (seconds)')
  row_counter = 1
  col_counter = 1
  # Add the traces
  for i in range(0, len(plots)):
    if(i >= row_counter * math.ceil(square)):
      row_counter += 1
      col_counter = 1
    col = col_counter
    row = row_counter
    fig.add_trace(go.Scatter(
      x=df.iloc[:,0], y=df.loc[:,plots[i]],
      mode='lines',
      name=plots[i]),
      row=row,
      col=col
    )
    an_pos = 1 / math.ceil(square)
    fig.layout.annotations[i].x = an_pos * col_counter - 0.5 * an_pos
    fig.update_yaxes(row=row_counter, col=col_counter, title_standoff=0, ticks="")
    fig.update_layout()
    if i == 0:
      fig['layout']['yaxis']['title']=y_axis_title(plots[i], args)
    else:
      fig['layout']['yaxis' + str(i+1)]['title']=y_axis_title(plots[i], args)
    col_counter += 1
  return fig

# Return a stack of plots
def stacked_layout(df, args):
  plots = df.columns[1:].tolist() if args.subset == None else args.subset
  fig = make_subplots(
    rows=len(plots), 
    cols=1,
    subplot_titles=plots,
    vertical_spacing=0.2/math.ceil(len(plots)/2),
    x_title= 'Time (seconds)')
  # Add the traces
  for i in range(0, len(plots)):
    fig.add_trace(go.Scatter(
      x=df.iloc[:,0], y=df.loc[:,plots[i]],
      mode='lines',
      name=plots[i]),
      row=i + 1,
      col=1
    )
    fig.layout.annotations[i].x = 0.985
    if i == 0:
      fig['layout']['yaxis']['title']=y_axis_title(plots[i], args)
    else:
      fig['layout']['yaxis' + str(i+1)]['title']=y_axis_title(plots[i], args)
  return fig

# Seperate and Combine like plots
def seperate_combined_layout(df, args):
  plots = df.columns[1:].tolist() if args.subset == None else args.subset
  V = []
  P = []
  I = []
  U = []
  for i in range(0, len(plots)):
    if plots[i][0] == 'V':
        V.append(i)
    elif plots[i][0] == 'I':
        I.append(i)
    elif plots[i][0] == 'P':
        P.append(i)
    else:
        U.append(i)
  fig_count = 0
  if len(V) != 0:
      fig_count = fig_count + 1
  if len(P) != 0:
      fig_count = fig_count + 1
  if len(I) != 0:
      fig_count = fig_count + 1
  if len(U) != 0:
      fig_count = fig_count + 1 
  fig = make_subplots(
    rows=fig_count, 
    cols=1,
    vertical_spacing=0.2/math.ceil(fig_count/2),
    x_title= 'Time (seconds)')  
  # Add the traces
  fig_count = 0
  if len(U) != 0:
    fig_count = fig_count + 1
    for i in U:
        fig.add_trace(go.Scatter(
            x=df.iloc[:,0], y=df.loc[:,plots[i]],
            mode='lines',
            name=plots[i]),
            row=fig_count,
            col=1
        )
    fig['layout']['yaxis' + str(fig_count)]['title']=y_axis_title(plots[i], args)
  if len(V) != 0:
    fig_count = fig_count + 1
    for i in V:
        fig.add_trace(go.Scatter(
          x=df.iloc[:,0], y=df.loc[:,plots[i]],
          mode='lines',
          name=plots[i]),
          row=fig_count,
          col=1
        )
    fig['layout']['yaxis' + str(fig_count)]['title']=y_axis_title(plots[i], args)
  if len(P) != 0:
    fig_count = fig_count + 1
    for i in P:
        fig.add_trace(go.Scatter(
          x=df.iloc[:,0], y=df.loc[:,plots[i]]*pfact(args.jump),
          mode='lines',
          name=plots[i]),
          row=fig_count,
          col=1
        )
    fig['layout']['yaxis' + str(fig_count)]['title']=y_axis_title(plots[i], args)
  if len(I) != 0:
    fig_count = fig_count + 1
    for i in I:
        fig.add_trace(go.Scatter(
          x=df.iloc[:,0], y=df.loc[:,plots[i]],
          mode='lines',
          name=plots[i]),
          row=fig_count,
          col=1
        )
    fig['layout']['yaxis' + str(fig_count)]['title']=y_axis_title(plots[i], args)
  return fig

# Combine all the plots
def combined_layout(df, args):
  plots = df.columns[1:].tolist() if args.subset == None else args.subset
  fig = go.Figure()

  fig['layout']['xaxis']['title']= 'Time (seconds)'
  # Y-axis label for combined plot
  count_V = 0
  count_P = 0
  count_I = 0
  count_U = 0
  y_title = ""
  for i in range(0, len(plots)):
    add_title = ""
    if plots[i][0] == 'V':
        if count_V == 0:
          add_title = y_axis_title(plots[i], args)          
          count_V = count_V + 1
    elif plots[i][0] == 'I':
        if count_I == 0:
          add_title = y_axis_title(plots[i], args)
          count_I = count_I + 1
    elif plots[i][0] == 'P':
        if count_P == 0:
          add_title = y_axis_title(plots[i], args)
          count_P = count_P + 1
    else:
        if count_U == 0:
          add_title = y_axis_title(plots[i], args)
          count_U = count_U + 1
    if len(add_title) > 0:
      if len(y_title) == 0:
        y_title = add_title
      else:
        y_title = y_title + " ; " + add_title
  fig['layout']['yaxis']['title']=y_title

  # Add the traces
  for i in range(0, len(plots)):
    if plots[i][0] == 'P':
      fig.add_trace(go.Scatter(
        # Use phase factor: pfact(args.jump)
        x=df.iloc[:,0], y=df.loc[:,plots[i]]*pfact(args.jump),
        mode='lines',
        name=plots[i])
      )
    else:
      fig.add_trace(go.Scatter(
        x=df.iloc[:,0], y=df.loc[:,plots[i]],
        mode='lines',
        name=plots[i])
      )
  return fig

if __name__ == '__main__':
  main()
