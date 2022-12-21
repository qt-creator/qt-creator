# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

def main():
    startQC(["-customwizard-verbose"])
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
