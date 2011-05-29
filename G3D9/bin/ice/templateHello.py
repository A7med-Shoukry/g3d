# templateHello.py
#
from __future__ import print_function

from .utils import *
from .variables import *
import datetime

defaultMainCppContents = """
/** \file main.cpp
 */
#include <stdio.h>

int main(int argc, const char** argv) {
    printf("Hello World!\\n");
    return 0;
}
"""

defaultMainpage = """
/** \file mainpage.dox
\mainpage

This is the main page of documentation for your project.  Edit
<code>mainpage.dox</code> to change it.  Documentation for
your classes and methods will automatically be generated by
Doxygen and added to these pages.  

This file and the comments in your may use any commands from the
<a href="http://www.stack.nl/~dimitri/doxygen/commands.html">Doxygen manual</a>
and the custom commands:
\verbatim
\cite source
\maintainer name
\created date
\edited date
\units units
\thumbnail{imagefilename}
\thumbnail{imagefilename,caption}
\endverbatim

See your \link Journal Development Journal \endlink.
*/
"""

defaultJournal = """
/** \file journal/journal.dox
\page Journal Development Journal

<i>Newest entries are at the top</i>

<hr><h2>""" + datetime.date.today().strftime('%a %b %d, %Y') + """: Project started</h2>
This file was created today.

You can embed images in this file using the Doxygen commands:
\verbatim
\thumbnail{imagefilename}
\thumbnail{imagefilename, caption}
\image html imagefilename
\endverbatim

The <code>imagefilename</code> does not need to include a path.  Put the images
in the <code>journal/</code> or <code>doc-files/</code> directories.  They will
automatically be copied to the <code>build/doc</code> directory when you run Doxygen.
*/
"""

""" Generates an empty project. """
def generateStarterFiles(state):
    mkdir('build')
    mkdir('source')
    mkdir('doc-files')
    mkdir('journal')

    writeFile('mainpage.dox', defaultMainpage)
    writeFile('journal/journal.dox', defaultJournal)

    if not isLibrary(state.binaryType):
        mkdir('data-files')
        writeFile('source/main.cpp', defaultMainCppContents)
    else:
        mkdir('include')
        incDir = pathConcat('include', state.projectName)
        mkdir(incDir)
        writeFile(pathConcat(incDir, state.projectName + '.h'), _headerContents(state))


""" Generates the contents of the master header file for a library """
def _headerContents(state):
    guard = state.projectName.upper() + '_H'
    return """
/** @file """ + state.projectName + """.h
 */

#ifndef """ + guard + """
#define """ + guard + """

#endif
"""
