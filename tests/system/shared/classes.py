#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

import operator

# for easier re-usage (because Python hasn't an enum type)
class Targets:
    DESKTOP_474_GCC = 1
    DESKTOP_480_GCC = 2
    SIMULATOR = 4
    MAEMO5 = 8
    HARMATTAN = 16
    EMBEDDED_LINUX = 32
    DESKTOP_480_MSVC2010 = 64
    DESKTOP_501_DEFAULT = 128

    @staticmethod
    def desktopTargetClasses():
        desktopTargets = Targets.DESKTOP_474_GCC | Targets.DESKTOP_480_GCC | Targets.DESKTOP_501_DEFAULT
        if platform.system() in ('Windows', 'Microsoft'):
            desktopTargets |= Targets.DESKTOP_480_MSVC2010
        return desktopTargets

    @staticmethod
    def getStringForTarget(target):
        if target == Targets.DESKTOP_474_GCC:
            return "Desktop 474 GCC"
        if target == Targets.DESKTOP_480_GCC:
            return "Desktop 480 GCC"
        elif target == Targets.MAEMO5:
            return "Fremantle"
        elif target == Targets.SIMULATOR:
            return "Qt Simulator"
        elif target == Targets.HARMATTAN:
            return "Harmattan"
        elif target == Targets.EMBEDDED_LINUX:
            return "Embedded Linux"
        elif target == Targets.DESKTOP_480_MSVC2010:
            return "Desktop 480 MSVC2010"
        elif target == Targets.DESKTOP_501_DEFAULT:
            return "Desktop 501 default"
        else:
            return None

    @staticmethod
    def getTargetsAsStrings(targets):
        if not isinstance(targets, (tuple,list)):
            test.fatal("Wrong usage... This function handles only tuples or lists.")
            return None
        result = map(Targets.getStringForTarget, targets)
        if None in result:
            test.fatal("You've passed at least one unknown target!")
        return result

    @staticmethod
    def intToArray(targets):
        available = [Targets.DESKTOP_474_GCC, Targets.DESKTOP_480_GCC, Targets.SIMULATOR, Targets.MAEMO5,
                     Targets.HARMATTAN, Targets.EMBEDDED_LINUX, Targets.DESKTOP_480_MSVC2010,
                     Targets.DESKTOP_501_DEFAULT]
        return filter(lambda x: x & targets == x, available)

    @staticmethod
    def arrayToInt(targetArr):
        return reduce(operator.or_, targetArr, 0)

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME = 0
    EDIT = 1
    DESIGN = 2
    DEBUG = 3
    PROJECTS = 4
    ANALYZE = 5
    HELP = 6
    # always adjust the following to the highest value of the available ViewConstants when adding new
    LAST_AVAILABLE = HELP

    # this function returns a regex of the tooltip of the FancyTabBar elements
    # this is needed because the keyboard shortcut is OS specific
    # if the provided argument does not match any of the ViewConstants it returns None
    @staticmethod
    def getToolTipForViewTab(viewTab):
        if viewTab == ViewConstants.WELCOME:
            return ur'Switch to <b>Welcome</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)1</span>'
        elif viewTab == ViewConstants.EDIT:
            return ur'Switch to <b>Edit</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)2</span>'
        elif viewTab == ViewConstants.DESIGN:
            return ur'Switch to <b>Design</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)3</span>'
        elif viewTab == ViewConstants.DEBUG:
            return ur'Switch to <b>Debug</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)4</span>'
        elif viewTab == ViewConstants.PROJECTS:
            return ur'Switch to <b>Projects</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)5</span>'
        elif viewTab == ViewConstants.ANALYZE:
            return ur'Switch to <b>Analyze</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)6</span>'
        elif viewTab == ViewConstants.HELP:
            return ur'Switch to <b>Help</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)7</span>'
        else:
            return None

class SubprocessType:
    QT_WIDGET=0
    QT_QUICK_APPLICATION=1
    QT_QUICK_UI=2
    USER_DEFINED=3

    @staticmethod
    def getWindowType(subprocessType):
        if subprocessType == SubprocessType.QT_WIDGET:
            return "QMainWindow"
        if subprocessType == SubprocessType.QT_QUICK_APPLICATION:
            return "QmlApplicationViewer"
        if subprocessType == SubprocessType.QT_QUICK_UI:
            return "QDeclarativeViewer"
        if subprocessType == SubprocessType.USER_DEFINED:
            return "user-defined"
        test.fatal("Could not determine the WindowType for SubprocessType %s" % subprocessType)
        return None

class QtInformation:
    QT_VERSION = 0
    QT_BINPATH = 1
    QT_LIBPATH = 2
