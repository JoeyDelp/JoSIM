#!/usr/bin/env python3
import math
import matplotlib
matplotlib.use('Qt5Agg')
import matplotlib.pyplot as pl
from matplotlib import gridspec
import numpy as np
import csv
import sys
import os
import fnmatch

from PyQt5 import QtCore, QtGui, QtWidgets

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure

import itertools

class MultiTabNavTool(QtWidgets.QWidget):
    def __init__(self, canvases, tabs, parent=None):
        QtWidgets.QWidget.__init__(self, parent)
        self.canvases = canvases
        self.tabs = tabs
        self.toolbars = [NavigationToolbar(canvas, parent) for canvas in self.canvases]
        vbox = QtWidgets.QVBoxLayout()
        for toolbar in self.toolbars:
            vbox.addWidget(toolbar)
        self.setLayout(vbox)
        self.switch_toolbar()
        self.tabs.currentChanged.connect(self.switch_toolbar)

    def switch_toolbar(self):
        for toolbar in self.toolbars:
            toolbar.setVisible(False)
        self.toolbars[self.tabs.currentIndex()].setVisible(True)

class MplMultiTab(QtWidgets.QMainWindow):
    #====================================================================================================
    def __init__(self, parent=None, figures=None, labels=None):
        QtWidgets.QMainWindow.__init__(self, parent)

        self.main_frame = QtWidgets.QWidget()
        self.tabWidget = QtWidgets.QTabWidget( self.main_frame )
        self.create_tabs( figures, labels )

        # Create the navigation toolbar, tied to the canvas
        self.mpl_toolbar = MultiTabNavTool(self.canvases, self.tabWidget, self.main_frame)

        self.vbox = vbox = QtWidgets.QVBoxLayout()
        vbox.addWidget(self.mpl_toolbar)
        vbox.addWidget(self.tabWidget)

        self.main_frame.setLayout(vbox)
        self.setCentralWidget(self.main_frame)

    #====================================================================================================
    def create_tabs(self, figures, labels ):

        if labels is None:      labels = []
        figures =  [Figure()] if figures is None else figures     #initialise with empty figure in first tab if no figures provided
        self.canvases = [self.add_tab(fig, lbl)
                            for (fig, lbl) in itertools.zip_longest(figures, labels) ]

    #====================================================================================================
    def add_tab(self, fig=None, name=None):
        '''dynamically add tabs with embedded matplotlib canvas with this function.'''

        # Create the mpl Figure and FigCanvas objects.
        if fig is None:
            fig = Figure()
            ax = fig.add_subplot(111)

        canvas = fig.canvas if fig.canvas else FigureCanvas(fig)
        canvas.setParent(self.tabWidget)
        canvas.setFocusPolicy( QtCore.Qt.ClickFocus )

        #self.tabs.append( tab )
        name = 'Tab %i'%(self.tabWidget.count()+1) if name is None else name
        self.tabWidget.addTab(canvas, name)

        return canvas

def main():
    if(len(sys.argv) < 2):
        print("Plotting script:")
        print("Usage: plotall.py <csv_files_location>")
        print("csv_files_location: folder containing csv files with ex_* pattern")
    else:
        path = '{0}/{1}'.format(os.getcwd(), sys.argv[1]);
        pattern = 'ex_*.csv'
        ex_files = [f for f in fnmatch.filter(os.listdir(path), pattern)]
        ex_files.sort()
        timedic = {}
        datadic = {}
        labeldic = {}
        for f in ex_files:
            timedic[f] = []
            datadic[f] = []
            with open(f, 'r') as csvFile:
                reader = csv.DictReader(csvFile, dialect='excel')
                labeldic[f] = reader.fieldnames
                datadic[f].append([])
                for row in reader:
                    timedic[f].append(float(row[labeldic[f][0]]))
                    for var in range(1, len(labeldic[f])):
                        datadic[f].append([])
                        datadic[f]
                        datadic[f][var].append(float(row[labeldic[f][var]]))
            csvFile.close()
        figures = []
        for f in ex_files:
            N = len(labeldic[f]) - 1
            cols = int(math.ceil(N / 4))
            rows = int(math.ceil(N / cols))
            gs = gridspec.GridSpec(rows, cols)
            fig = pl.figure(num=f)
            for var in range(1,len(labeldic[f])):
                ax = fig.add_subplot(gs[var - 1])
                ax.plot(timedic[f], datadic[f][var], 'k', linewidth=0.75)
                ax.set_xlabel(labeldic[f][0])
                if labeldic[f][var][0] == 'V':
                    ax.set_ylabel("voltage (V)")
                elif labeldic[f][var][0] == 'I':
                    ax.set_ylabel("current (A)")
                elif labeldic[f][var][0] == 'P':
                    ax.set_ylabel("phase (rad)")
                ax.text(.5,.95,labeldic[f][var],
                    horizontalalignment='center',
                    transform=ax.transAxes)
                pl.grid(True)

            fig.set_tight_layout(True)
            figures.append(fig)
            pl.close(fig)

        app = QtWidgets.QApplication(sys.argv)
        ui = MplMultiTab( figures=figures, labels=ex_files )
        ui.show()
        app.exec_()

if __name__== "__main__":
  main()
