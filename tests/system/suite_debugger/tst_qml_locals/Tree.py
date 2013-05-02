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

import os

# Helper class to create a tree structure
class Tree:
    def __init__(self, name=None, value=None, children=None):
        self.__name__ = name
        self.__value__ = value
        self.__children__ = children

    # getter functions
    def getChild(self, name, occurrence=1):
        if self.__children__:
            for ch in self.__children__:
                if ch.__name__ == name:
                    if occurrence == 1:
                        return ch
                    occurrence -= 1
        return None

    def getChildren(self):
        return self.__children__

    def getName(self):
        return self.__name__

    def getValue(self):
        return self.__value__

    # setter function
    def setChildren(self, children):
        self.__children__ = children

    def setName(self, name):
        self.__name__ = name

    def setValue(self, value):
        self.__value__ = value

    # other functions
    def addChild(self, child):
        if self.__children__ == None:
            self.__children__ = []
        self.__children__.append(child)

    def childrenCount(self):
        if self.__children__:
            return len(self.__children__)
        return 0

    def countChildOccurrences(self, name):
        if not self.__children__:
            return 0
        return len(filter(lambda x: x.getName() == name, self.__children__))

    # internal functions
    def __repr__(self):
        return self.__str__()

    def __strIndented__(self, indent):
        result = "%s%s (%s)" % (" " * indent, self.__name__, self.__value__)
        if self.__children__:
            for ch in self.__children__:
                if isinstance(ch, Tree):
                    result += os.linesep + ch.__strIndented__(indent + 2)
                else:
                    result += os.linesep + " " * (indent + 2) + str(ch)
        return result

    def __str__(self):
        return self.__strIndented__(1)
