# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

source("../../shared/qtcreator.py")

# test search in help mode and advanced search
searchKeywordDictionary = { "compass":True, "deplmint":False, "QODBC":True, "bldx":False }
urlDictionary = {"compass":"qthelp://org.qt-project.qtdoc.5141/qtdoc/mobiledevelopment.html",
                 "QODBC":"qthelp://org.qt-project.qtsql.5141/qtsql/sql-driver.html" }


def __getSelectedText__():
    try:
        return str(getHelpViewer().selectedText())
    except:
        test.warning("Could not get highlighted text.")
        return str("")

def __getUrl__():
    try:
        url = getHelpViewer().url()
    except:
        return ""
    return str(url.scheme) + "://" + str(url.host) + str(url.path)

def getHighlightsInHtml(htmlCode):
    pattern = re.compile('color:#ff0000;">(.*?)</span>')
    res = ""
    for curr in pattern.finditer(htmlCode):
        if curr.group(1) in res:
            continue
        res += "%s " % curr.group(1)
    return res

def verifySelection(expected):
    selText = str(__getSelectedText__())
    if test.verify(selText, "Verify that there is a selection"):
        # verify if search keyword is found in results
        test.verify(expected.lower() in selText.lower(),
                    "'%s' search result can be found" % expected)

def verifyUrl(expected):
    return test.compare(expected, __getUrl__(), "Expected URL loaded?")

def main():
    noMatch = "Your search did not match any documents."
    startQC()
    if not startedWithoutPluginError():
        return
    docFiles = ["qtdoc.qch", "qtsql.qch"]
    docFiles = [os.path.join(QtPath.docsPath(Targets.DESKTOP_5_14_1_DEFAULT), file) for file in docFiles]
    addHelpDocumentation(docFiles)
    # switch to help mode
    switchViewTo(ViewConstants.HELP)
    # verify that search widget is accessible
    mouseClick(waitForObjectItem(":Qt Creator_Core::Internal::CommandComboBox", "Search"))
    snooze(1) # Looks like searching is still available for an instant
    test.verify(checkIfObjectExists("{type='QHelpSearchQueryWidget' unnamed='1' visible='1' "
                                    "window=':Qt Creator_Core::Internal::MainWindow'}",
                                    timeout=600000),
                "Verifying search widget visibility.")
    # try to search empty string
    clickButton(waitForObject("{text='Search' type='QPushButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    resultWidget = waitForObject(':Hits_QResultWidget', 5000)
    test.verify(waitFor("noMatch in "
                        "str(resultWidget.plainText)", 2000),
                        "Verifying if search did not match anything.")
    # try to search keyword from list
    searchLineEdit = getChildByClass(waitForObject("{type='QHelpSearchQueryWidget' unnamed='1' visible='1' "
                                                   "window=':Qt Creator_Core::Internal::MainWindow'}"),
                                     "QLineEdit")
    for searchKeyword,shouldFind in searchKeywordDictionary.items():
        mouseClick(searchLineEdit)
        replaceEditorContent(searchLineEdit, searchKeyword)
        type(searchLineEdit, "<Return>")
        progressBarWait(warn=False)
        if shouldFind:
            test.verify(waitFor("re.match('[1-9]\d* - [1-9]\d* of [1-9]\d* Hits',"
                                "str(findObject(':Hits_QLabel').text))", 2000),
                                "Verifying if search results found with 1+ hits for: " + searchKeyword)
            selText = __getSelectedText__()
            url = __getUrl__()
            # click in the widget, tab to first item and press enter
            mouseClick(resultWidget)
            type(resultWidget, "<Tab>")
            type(resultWidget, "<Return>")
            waitFor("__getUrl__() != url or selText != __getSelectedText__()", 20000)
            verifySelection(searchKeyword)
            verifyUrl(urlDictionary[searchKeyword])
        else:
            test.verify(waitFor("noMatch in "
                                "str(resultWidget.plainText)", 1000),
                                "Verifying if search did not match anything for: " + searchKeyword)
    # exit
    invokeMenuItem("File", "Exit")
