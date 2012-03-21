source("../../shared/qtcreator.py")
source("../../shared/suites_qtta.py")
import re

# test search in help mode and advanced search
searchKeywordDictionary={ "deployment":True, "deplmint":False, "build":True, "bld":False }

def __getSelectedText__():
    try:
        selText = findObject(":Qt Creator_Help::Internal::HelpViewer").selectedText
        if className(selText) != 'instancemethod':
            return str(selText)
    except:
        pass
    try:
        hv = findObject(":Qt Creator_Help::Internal::HelpViewer")
        selText = getHighlightsInHtml(str(hv.toHtml()))
    except:
        test.warning("Could not get highlighted text.")
        selText = ''
    return str(selText)

def __handleTextChanged__(obj):
    global textHasChanged
    textHasChanged = True

def getHighlightsInHtml(htmlCode):
    pattern = re.compile('color:#ff0000;">(.*?)</span>')
    res = ""
    for curr in pattern.finditer(htmlCode):
        if curr.group(1) in res:
            continue
        res += "%s " % curr.group(1)
    test.log(res)
    return res

# wait for indexing progress bar to appear and disappear
def progressBarWait():
    checkIfObjectExists("{type='Core::Internal::ProgressBar' unnamed='1'}", True, 2000)
    checkIfObjectExists("{type='Core::Internal::ProgressBar' unnamed='1'}", False, 60000)

def main():
    global textHasChanged
    noMatch = "Your search did not match any documents."
    startApplication("qtcreator" + SettingsPath)
    installLazySignalHandler(":Qt Creator_Help::Internal::HelpViewer", "textChanged()", "__handleTextChanged__")
    addHelpDocumentationFromSDK()
    # switch to help mode
    switchViewTo(ViewConstants.HELP)
    # verify that search widget is accessible
    mouseClick(waitForObjectItem(":Qt Creator_Core::Internal::CommandComboBox_2", "Search"))
    test.verify(checkIfObjectExists(":Qt Creator_QHelpSearchQueryWidget"),
                "Verifying search widget visibility.")
    # try to search empty string
    clickButton(waitForObject(":Qt Creator.Search_QPushButton"))
    progressBarWait()
    test.verify(waitFor("noMatch in "
                        "str(waitForObject(':Hits_QCLuceneResultWidget').plainText)", 2000),
                        "Verifying if search did not match anything.")
    # workaround for "endless waiting cursor"
    mouseClick(waitForObject(":Qt Reference Documentation_QModelIndex"))
    # try to search keyword from list
    for searchKeyword,shouldFind in searchKeywordDictionary.items():
        mouseClick(waitForObject(":Qt Creator.Search for:_QLineEdit"))
        replaceEditorContent(":Qt Creator.Search for:_QLineEdit", searchKeyword)
        type(waitForObject(":Qt Creator.Search for:_QLineEdit"), "<Return>")
        progressBarWait()
        if shouldFind:
            test.verify(waitFor("re.match('[1-9]\d* - [1-9]\d* of [1-9]\d* Hits',"
                                "str(findObject(':Hits_QLabel').text))", 2000),
                                "Verifying if search results found with 1+ hits for: " + searchKeyword)
            textHasChanged = False
            selText = __getSelectedText__()
            # click in the widget, tab to first item and press enter
            mouseClick(waitForObject(":Hits_QCLuceneResultWidget"), 1, 1, 0, Qt.LeftButton)
            type(waitForObject(":Hits_QCLuceneResultWidget"), "<Tab>")
            type(waitForObject(":Hits_QCLuceneResultWidget"), "<Return>")
            waitFor("textHasChanged or selText != __getSelectedText__()")
            # verify if search keyword is found in results
            test.verify(searchKeyword.lower() in __getSelectedText__().lower(),
                        searchKeyword + " search result can be found")
        else:
            test.verify(waitFor("noMatch in "
                                "str(waitForObject(':Hits_QCLuceneResultWidget').plainText)", 1000),
                                "Verifying if search did not match anything for: " + searchKeyword)
    # advanced search - setup
    clickButton(waitForObject(":Qt Creator.+_QToolButton"))
    type(waitForObject(":Qt Creator.words similar to:_QLineEdit"), "deploy")
    type(waitForObject(":Qt Creator.without the words:_QLineEdit"), "bookmark")
    type(waitForObject(":Qt Creator.with exact phrase:_QLineEdit"), "sql in qt")
    type(waitForObject(":Qt Creator.with all of the words:_QLineEdit"), "designer sql")
    type(waitForObject(":Qt Creator.with at least one of the words:_QLineEdit"), "printing")
    # advanced search - do search
    clickButton(waitForObject(":Qt Creator.Search_QPushButton"))
    progressBarWait()
    # verify that advanced search results found
    test.verify(waitFor("re.search('1 - 2 of 2 Hits',"
                        "str(findObject(':Hits_QLabel').text))", 3000),
                        "Verifying if 2 search results found")
    resultsView = waitForObject(":Hits_QCLuceneResultWidget")
    mouseClick(resultsView, 1, 1, 0, Qt.LeftButton)
    type(resultsView, "<Tab>")
    type(resultsView, "<Return>")
    test.verify("printing" in str(findObject(":Qt Creator_Help::Internal::HelpViewer").selectedText).lower(),
                "printing advanced search result can be found")
    for i in range(2):
        type(resultsView, "<Tab>")
    type(resultsView, "<Return>")
    test.verify("sql" in str(findObject(":Qt Creator_Help::Internal::HelpViewer").selectedText).lower(),
                "sql advanced search result can be found")
    # verify if simple search is properly disabled
    test.verify(findObject(":Qt Creator.Search for:_QLineEdit").enabled == False,
                "Verifying if simple search is not active in advanced mode.")
    # exit
    invokeMenuItem("File", "Exit")
# no cleanup needed
