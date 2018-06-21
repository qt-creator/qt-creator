############################################################################
#
# Copyright (C) 2017 The Qt Company Ltd.
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

def main():
    startApplication("qtcreator" + SettingsPath + " -customwizard-verbose")
    if not startedWithoutPluginError():
        return

    openGeneralMessages()
    messages = waitForObject(":Qt Creator_Core::OutputWindow")
    output = str(messages.plainText)
    test.log("General Messages Output:\n%s" % output)
    if not len(output.strip()):
        test.fatal("Failed to retrieve any output from General Messages pane.")
    # check for parse errors, creation errors, no JSON object at all
    for errMssg, description in (("Failed to parse", "Parse failures"),
                                 ("Failed to create", "Creation failures"),
                                 ("Did not find a JSON object", "Missing JSON objects")):
        if not test.verify(errMssg not in output, "Checking for %s" % description):
            test.log("%s\n%s"
                     % (description, "\n".join(l for l in output.splitlines() if errMssg in l)))

    # check for invalid version
    invalidVersionRegex = "^\* Version .* not supported\."
    if not test.verify(not re.search(invalidVersionRegex, output, re.MULTILINE), "Version check"):
        test.log("Found unsupported version:\n%s"
                 % "\n".join(l for l in output.splitlines() if re.search(invalidVersionRegex, l)))
    invokeMenuItem("File", "Exit")
