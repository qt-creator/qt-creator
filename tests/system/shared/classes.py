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
    ALL_TARGETS = tuple(map(lambda x: 2 ** x , range(5)))

    (DESKTOP_4_8_7_DEFAULT,
     EMBEDDED_LINUX,
     DESKTOP_5_4_1_GCC,
     DESKTOP_5_6_1_DEFAULT,
     DESKTOP_5_10_1_DEFAULT) = ALL_TARGETS

    @staticmethod
    def availableTargetClasses():
        availableTargets = list(Targets.ALL_TARGETS)
        if platform.system() in ('Windows', 'Microsoft'):
            availableTargets.remove(Targets.EMBEDDED_LINUX)
        elif platform.system() == 'Darwin':
            availableTargets.remove(Targets.DESKTOP_5_4_1_GCC)
        return availableTargets

    @staticmethod
    def desktopTargetClasses():
        desktopTargets = Targets.availableTargetClasses()
        if Targets.EMBEDDED_LINUX in desktopTargets:
            desktopTargets.remove(Targets.EMBEDDED_LINUX)
        return desktopTargets

    @staticmethod
    def qt4Classes():
        return (Targets.DESKTOP_4_8_7_DEFAULT | Targets.EMBEDDED_LINUX)

    @staticmethod
    def getStringForTarget(target):
        if target == Targets.DESKTOP_4_8_7_DEFAULT:
            return "Desktop 4.8.7 default"
        elif target == Targets.EMBEDDED_LINUX:
            return "Embedded Linux"
        elif target == Targets.DESKTOP_5_4_1_GCC:
            return "Desktop 5.4.1 GCC"
        elif target == Targets.DESKTOP_5_6_1_DEFAULT:
            return "Desktop 5.6.1 default"
        elif target == Targets.DESKTOP_5_10_1_DEFAULT:
            return "Desktop 5.10.1 default"
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
    def getDefaultKit():
        return Targets.DESKTOP_5_6_1_DEFAULT

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
        qt5targets = [Targets.DESKTOP_5_6_1_DEFAULT, Targets.DESKTOP_5_10_1_DEFAULT]
        if platform.system() != 'Darwin':
            qt5targets.append(Targets.DESKTOP_5_4_1_GCC)
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

        matcher = re.match("^Desktop (5\.\\d{1,2}\.\\d{1,2}).*$", Targets.getStringForTarget(target))
        if matcher is None:
            raise Exception("Currently this is supported for Desktop Qt5 only, got target '%s'"
                            % str(Targets.getStringForTarget(target)))
        return matcher.group(1)

    @staticmethod
    def __createPlatformQtPath__(qt5Minor):
        if platform.system() in ('Microsoft', 'Windows'):
            return "C:/Qt/Qt5.%d.1" % qt5Minor
        else:
            return os.path.expanduser("~/Qt5.%d.1" % qt5Minor)

    @staticmethod
    def toVersionTuple(versionString):
        return tuple(map(__builtin__.int, versionString.split(".")))

    @staticmethod
    def getQtMinorAndPatchVersion(target):
        qtVersionStr = Qt5Path.__preCheckAndExtractQtVersionStr__(target)
        versionTuple = Qt5Path.toVersionTuple(qtVersionStr)
        return versionTuple[1], versionTuple[2]

    @staticmethod
    def examplesPath(target):
        qtMinorVersion, qtPatchVersion = Qt5Path.getQtMinorAndPatchVersion(target)
        if qtMinorVersion < 10:
            path = "Examples/Qt-5.%d" % qtMinorVersion
        else:
            path = "Examples/Qt-5.%d.%d" % (qtMinorVersion, qtPatchVersion)

        return os.path.join(Qt5Path.__createPlatformQtPath__(qtMinorVersion), path)

    @staticmethod
    def docsPath(target):
        qtMinorVersion, qtPatchVersion = Qt5Path.getQtMinorAndPatchVersion(target)
        if qtMinorVersion < 10:
            path = "Docs/Qt-5.%d" % qtMinorVersion
        else:
            path = "Docs/Qt-5.%d.%d" % (qtMinorVersion, qtPatchVersion)

        return os.path.join(Qt5Path.__createPlatformQtPath__(qtMinorVersion), path)

class TestSection:
    def __init__(self, description):
        self.description = description

    def __enter__(self):
        test.startSection(self.description)

    def __exit__(self, exc_type, exc_value, traceback):
        test.endSection()
