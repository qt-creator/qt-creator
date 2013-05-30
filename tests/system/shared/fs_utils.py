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

import stat

# this function modifies all (regular) files inside the given dirPath to have specified permissions
# ATTENTION: it won't process the directory recursively
def changeFilePermissions(dirPath, readPerm, writePerm, excludeFileNames=None):
    permission = 0
    if readPerm:
        permission |= stat.S_IREAD
    if writePerm:
        permission |= stat.S_IWRITE
    if excludeFileNames == None:
        excludeFileNames = []
    elif isinstance(excludeFileNames, (str, unicode)):
        excludeFileNames = [excludeFileNames]
    if not isinstance(excludeFileNames, (tuple, list)):
        test.warning("File names to exclude must be of type str, unicode, list, tuple or None - "
                     "ignoring parameter this time.")
        excludeFileNames = []
    if not os.path.isdir(dirPath):
        test.warning("Could not find directory '%s'." % dirPath)
        return False
    filePaths = [os.path.join(dirPath, fileName) for fileName in os.listdir(dirPath)
                 if fileName not in excludeFileNames]
    result = True
    for filePath in filter(os.path.isfile, filePaths):
        try:
            os.chmod(filePath, permission)
        except:
            t,v = sys.exc_info()[:2]
            test.log("Error: %s(%s)" % (str(t), str(v)))
            result = False
    return result

def isWritable(pathToFile):
    if os.path.exists(pathToFile):
        return os.access(pathToFile, os.W_OK)
    else:
        test.warning("Path to check for writability does not exist.")
        return False
