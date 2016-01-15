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
