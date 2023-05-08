# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    elif isString(excludeFileNames):
        excludeFileNames = [excludeFileNames]
    if not isinstance(excludeFileNames, (tuple, list)):
        test.warning("File names to exclude must be of type str, list, tuple or None - "
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
            test.log("%s: %s" % (t.__name__, str(v)))
            result = False
    return result

def isWritable(pathToFile):
    if os.path.exists(pathToFile):
        return os.access(pathToFile, os.W_OK)
    else:
        test.warning("Path to check for writability does not exist.")
        return False
