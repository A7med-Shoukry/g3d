#
# ice/deploy.py
#
# Creates distributable files from the build
#
from __future__ import print_function

from . import utils
import math
from .variables import *
from .utils import *
from .copyifnewer import copyIfNewer
from .library import libraryTable, FRAMEWORK

def _makePList(appname, binaryName):
    return """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>""" + appname + """</string>
	<key>CFBundleGetInfoString</key>
	<string>1.0</string>
	<key>CFBundleIconFile</key>
	<string>icon.icns</string>
	<key>CFBundleIdentifier</key>
	<string>""" + appname + """</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>""" + appname + """</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0.0</string>
	<key>CFBundleSignature</key>
        <string>""" + appname[0:4] + """</string>
	<key>CFBundleVersion</key>
	<string>1</string>
</dict>
</plist>"""


def _createApp(tmpDir, appDir, srcDir, state):
    contents   = appDir + 'Contents/'
    frameworks = contents + 'Frameworks/'
    resources  = contents + 'Resources/'
    macos      = contents + 'MacOS/'

    # Create directories
    mkdir(appDir,     verbosity >= VERBOSE)
    mkdir(contents,   verbosity >= VERBOSE)
    mkdir(frameworks, verbosity >= VERBOSE)
    mkdir(resources,  verbosity >= VERBOSE)
    mkdir(macos,      verbosity >= VERBOSE)

    # Create Info.plist
    if verbosity >= NORMAL: colorPrint('\nWriting Info.plist and PkgInfo', SECTION_COLOR)
    writeFile(contents + 'Info.plist', _makePList(state.projectName, state.binaryName))
    writeFile(contents + 'PkgInfo', 'APPL' + state.binaryName[0:4] + '\n') 

    # Copy binary
    if verbosity >= NORMAL: colorPrint('\nCopying executable', SECTION_COLOR)
    shell('cp ' + state.binaryDir + state.binaryName + ' ' + macos + state.binaryName, verbosity >= VERBOSE) 

    # Copy data-files to Resources
    if verbosity >= NORMAL: colorPrint('\nCopying data files', SECTION_COLOR)
    copyIfNewer('data-files', resources, verbosity >= VERBOSE, verbosity == NORMAL)
    if verbosity >= VERBOSE: colorPrint('Done copying data files', SECTION_COLOR)

    # Copy frameworks
    for libName in state.libList():
        if libName in libraryTable:
            lib = libraryTable[libName]
            if lib.deploy and (lib.type == FRAMEWORK):
                print('Copying ' + lib.name + ' Framework')
                fwk = lib.releaseFramework + '.framework'
                src = '/Library/Frameworks/' + fwk
                copyIfNewer(src, frameworks + fwk, verbosity >= VERBOSE, verbosity == NORMAL)


def _createDmg(tmpDir, dmgName, projectName):
    shell('hdiutil create -fs HFS+ -srcfolder ' + tmpDir + ' ' + dmgName + ' -volname "' + projectName + '"', verbosity >= VERBOSE)

def _deployOSX(state):
    print()
    srcDir = state.binaryDir
    tmpDir = state.tempDir + 'deploy'
    appDir = tmpDir + '/' + state.projectName + '.app/'
    dmgName = state.buildDir + state.projectName + '.dmg'

    rmdir(tmpDir)
    _createApp(tmpDir + '/', appDir, srcDir, state)
    _createDmg(tmpDir, dmgName, state.projectName)
    
    if verbosity >= VERBOSE:
        print()
    print('Deployable archive written to ' + dmgName)


def _deployUnix(state):
    print()
    tarfile = state.buildDir + state.projectName + '.tar'
    shell('tar -cf ' + tarfile +' ' + state.installDir + '*', verbosity >= VERBOSE)
    shell('gzip -f ' + tarfile, verbosity >= VERBOSE)
    rm(tarfile, verbosity >= VERBOSE)
    if verbosity >= VERBOSE:
        print()
    print('Deployable archive written to ' + state.buildDir + tarfile + '.gz')

#
# Create a deployment file
#
def deploy(state):
    maybeColorPrint('Building deployment', SECTION_COLOR)
    if (os.uname()[0] == "Darwin"):
        _deployOSX(state)
    else:
        _deployUnix(state)
        
    
