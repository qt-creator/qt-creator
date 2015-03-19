#############################################################################
##
## Copyright (C) 2015 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company.  For licensing terms and
## conditions see http://www.qt.io/terms-conditions.  For further information
## use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file.  Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, The Qt Company gives you certain additional
## rights.  These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

import operator

# for easier re-usage (because Python hasn't an enum type)
class Targets:
    ALL_TARGETS = map(lambda x: 2 ** x , range(9))

    (DESKTOP_474_GCC,
     DESKTOP_480_DEFAULT,
     SIMULATOR,
     MAEMO5,
     HARMATTAN,
     EMBEDDED_LINUX,
     DESKTOP_521_DEFAULT,
     DESKTOP_531_DEFAULT,
     DESKTOP_541_GCC) = ALL_TARGETS

    @staticmethod
    def desktopTargetClasses():
        desktopTargets = (sum(Targets.ALL_TARGETS) & ~Targets.SIMULATOR & ~Targets.MAEMO5
                          & ~Targets.HARMATTAN & ~Targets.EMBEDDED_LINUX)
        if platform.system() == 'Darwin':
            desktopTargets &= ~Targets.DESKTOP_541_GCC
        return desktopTargets

    @staticmethod
    def getStringForTarget(target):
        if target == Targets.DESKTOP_474_GCC:
            return "Desktop 474 GCC"
        elif target == Targets.DESKTOP_480_DEFAULT:
            if platform.system() in ('Windows', 'Microsoft'):
                return "Desktop 480 MSVC2010"
            else:
                return "Desktop 480 GCC"
        elif target == Targets.MAEMO5:
            return "Fremantle"
        elif target == Targets.SIMULATOR:
            return "Qt Simulator"
        elif target == Targets.HARMATTAN:
            return "Harmattan"
        elif target == Targets.EMBEDDED_LINUX:
            return "Embedded Linux"
        elif target == Targets.DESKTOP_521_DEFAULT:
            return "Desktop 521 default"
        elif target == Targets.DESKTOP_531_DEFAULT:
            return "Desktop 531 default"
        elif target == Targets.DESKTOP_541_GCC:
            return "Desktop 541 GCC"
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
        return filter(lambda x: x & targets, Targets.ALL_TARGETS)

    @staticmethod
    def arrayToInt(targetArr):
        return reduce(operator.or_, targetArr, 0)

    @staticmethod
    def getDefaultKit():
        return Targets.DESKTOP_521_DEFAULT

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME, EDIT, DESIGN, DEBUG, PROJECTS, ANALYZE, HELP = range(7)
    FIRST_AVAILABLE = 0
    # always adjust the following to the highest value of the available ViewConstants when adding new
    LAST_AVAILABLE = HELP

    # this function returns a regex of the tooltip of the FancyTabBar elements
    # this is needed because the keyboard shortcut is OS specific
    # if the provided argument does not match any of the ViewConstants it returns None
    @staticmethod
    def getToolTipForViewTab(viewTab):
        if viewTab == ViewConstants.WELCOME:
            toolTip = ur'Switch to <b>Welcome</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)%d</span>'
        elif viewTab == ViewConstants.EDIT:
            toolTip = ur'Switch to <b>Edit</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)%d</span>'
        elif viewTab == ViewConstants.DESIGN:
            toolTip = ur'Switch to <b>Design</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)%d</span>'
        elif viewTab == ViewConstants.DEBUG:
            toolTip = ur'Switch to <b>Debug</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)%d</span>'
        elif viewTab == ViewConstants.PROJECTS:
            toolTip = ur'Switch to <b>Projects</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)%d</span>'
        elif viewTab == ViewConstants.ANALYZE:
            toolTip = ur'Switch to <b>Analyze</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)%d</span>'
        elif viewTab == ViewConstants.HELP:
            toolTip = ur'Switch to <b>Help</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)%d</span>'
        else:
            return None
        return toolTip % (viewTab + 1)

class SubprocessType:
    QT_WIDGET=0
    QT_QUICK_APPLICATION=1
    QT_QUICK_UI=2
    USER_DEFINED=3

    @staticmethod
    def getWindowType(subprocessType, qtQuickVersion="1.1"):
        if subprocessType == SubprocessType.QT_WIDGET:
            return "QMainWindow"
        if subprocessType == SubprocessType.QT_QUICK_APPLICATION:
            qqv = "2"
            if qtQuickVersion[0] == "1":
                qqv = "1"
            return "QtQuick%sApplicationViewer" % qqv
        if subprocessType == SubprocessType.QT_QUICK_UI:
            if qtQuickVersion == "1.1":
                return "QDeclarativeViewer"
            else:
                return "QQuickView"
        if subprocessType == SubprocessType.USER_DEFINED:
            return "user-defined"
        test.fatal("Could not determine the WindowType for SubprocessType %s" % subprocessType)
        return None

class QtInformation:
    QT_VERSION = 0
    QT_BINPATH = 1
    QT_LIBPATH = 2

class LibType:
    SHARED = 0
    STATIC = 1
    QT_PLUGIN = 2

    @staticmethod
    def getStringForLib(libType):
        if libType == LibType.SHARED:
            return "Shared Library"
        if libType == LibType.STATIC:
            return "Statically Linked Library"
        if libType == LibType.QT_PLUGIN:
            return "Qt Plugin"
        return None
