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

source("../../shared/qtcreator.py")

# test New Qt Gui Application build and run for release and debug option
def main():
    # Start Creator with built-in code model, to avoid having
    # warnings from the clang code model in "issues" view
    startCreator(False)
    if not startedWithoutPluginError():
        return
    checkedTargets = createProject_Qt_GUI(tempDir(), "SampleApp")
    # run project for debug and release and verify results
    runVerify(checkedTargets)
    #close Qt Creator
    invokeMenuItem("File", "Exit")
