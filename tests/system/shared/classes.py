############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

import __builtin__
import operator

# for easier re-usage (because Python hasn't an enum type)
class Targets:
    ALL_TARGETS = map(lambda x: 2 ** x , range(6))

    (DESKTOP_474_GCC,
     DESKTOP_480_DEFAULT,
     EMBEDDED_LINUX,
     DESKTOP_531_DEFAULT,
     DESKTOP_541_GCC,
     DESKTOP_561_DEFAULT) = ALL_TARGETS

    @staticmethod
    def desktopTargetClasses():
        desktopTargets = (sum(Targets.ALL_TARGETS) & ~Targets.EMBEDDED_LINUX)
        if platform.system() == 'Darwin':
            desktopTargets &= ~Targets.DESKTOP_541_GCC
        return desktopTargets

    @staticmethod
    def qt4Classes():
        return (Targets.DESKTOP_474_GCC | Targets.DESKTOP_480_DEFAULT
                | Targets.EMBEDDED_LINUX)

    @staticmethod
    def getStringForTarget(target):
        if target == Targets.DESKTOP_474_GCC:
            return "Desktop 474 GCC"
        elif target == Targets.DESKTOP_480_DEFAULT:
            if platform.system() in ('Windows', 'Microsoft'):
                return "Desktop 480 MSVC2010"
            else:
                return "Desktop 480 GCC"
        elif target == Targets.EMBEDDED_LINUX:
            return "Embedded Linux"
        elif target == Targets.DESKTOP_531_DEFAULT:
            return "Desktop 531 default"
        elif target == Targets.DESKTOP_541_GCC:
            return "Desktop 541 GCC"
        elif target == Targets.DESKTOP_561_DEFAULT:
            return "Desktop 561 default"
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
        return Targets.DESKTOP_531_DEFAULT

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME, EDIT, DESIGN, DEBUG, PROJECTS, HELP = range(6)
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

class Qt5Path:
    DOCS = 0
    EXAMPLES = 1

    @staticmethod
    def getPaths(pathSpec):
        qt5targets = [Targets.DESKTOP_531_DEFAULT, Targets.DESKTOP_561_DEFAULT]
        if platform.system() != 'Darwin':
            qt5targets.append(Targets.DESKTOP_541_GCC)
        if pathSpec == Qt5Path.DOCS:
            return map(lambda target: Qt5Path.docsPath(target), qt5targets)
        elif pathSpec == Qt5Path.EXAMPLES:
            return map(lambda target: Qt5Path.examplesPath(target), qt5targets)
        else:
            test.fatal("Unknown pathSpec given: %s" % str(pathSpec))
            return []

    @staticmethod
    def __preCheckAndExtractQtVersionStr__(target):
        if target not in Targets.ALL_TARGETS:
            raise Exception("Unexpected target '%s'" % str(target))

        matcher = re.match("^Desktop (5\\d{2}).*$", Targets.getStringForTarget(target))
        if matcher is None:
            raise Exception("Currently this is supported for Desktop Qt5 only, got target '%s'"
                            % str(Targets.getStringForTarget(target)))
        return matcher.group(1)

    @staticmethod
    def __createPlatformQtPath__(qt5Minor):
        # special handling for Qt5.2
        if qt5Minor == 2:
            if platform.system() in ('Microsoft', 'Windows'):
                return "C:/Qt/Qt5.2.1/5.2.1/msvc2010"
            elif platform.system() == 'Linux':
                if __is64BitOS__():
                    return os.path.expanduser("~/Qt5.2.1/5.2.1/gcc_64")
                else:
                    return os.path.expanduser("~/Qt5.2.1/5.2.1/gcc")
            else:
                return os.path.expanduser("~/Qt5.2.1/5.2.1/clang_64")
        # Qt5.3+
        if platform.system() in ('Microsoft', 'Windows'):
            return "C:/Qt/Qt5.%d.1" % qt5Minor
        else:
            return os.path.expanduser("~/Qt5.%d.1" % qt5Minor)

    @staticmethod
    def examplesPath(target):
        qtVersionStr = Qt5Path.__preCheckAndExtractQtVersionStr__(target)
        qtMinorVersion = __builtin__.int(qtVersionStr[1])
        if qtMinorVersion == 2:
            path = "examples"
        else:
            path = "Examples/Qt-5.%d" % qtMinorVersion

        return os.path.join(Qt5Path.__createPlatformQtPath__(qtMinorVersion), path)

    @staticmethod
    def docsPath(target):
        qtVersionStr = Qt5Path.__preCheckAndExtractQtVersionStr__(target)
        qtMinorVersion = __builtin__.int(qtVersionStr[1])
        if qtMinorVersion == 2:
            path = "doc"
        else:
            path = "Docs/Qt-5.%d" % qtMinorVersion

        return os.path.join(Qt5Path.__createPlatformQtPath__(qtMinorVersion), path)
