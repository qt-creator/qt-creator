# Coco Manual Testing

## Prerequisites

* Squish coco installation with language server support (Version 6 or greater)
* The result of coverage scan
(for example the squish coco tutorial from /path/to/squish/tutorial just run build in that folder and execute hello)
* Start Qt Creator with -load Coco or enable the plugin under Help > About Plugins... in Qt Creator

## Starting Coco from Qt Creator

* Select Analyze > Squish Coco ...
* Insert the path to the CoverageBrowser (/path/to/squish/coveragebrowser)
executable and the coco instrumentation file (*.csmes) of the coverage scan (/path/to/squish/tutorial/hello.csmes)
* Select Open
* In the started CoverageBrowser select File > Load Execution Report... and select
the .csexe for the coverage scan (/path/to/squish/tutorial/hello.csexe)
    * If you want to reuse that execution report make sure to deselect "Delete execution report after loading"

## Tests

* Open a file that was part of the coverage scan (/path/to/squish/tutorial/tutorial.cpp)
* Verify that there are sensible annotations added to editor
* Close the document
* Goto Edit > Preferences > TextEditor > Font & Colors and change some formats of
    * Code Coverage Added Code
    * Partially Covered Code
    * Uncovered Code
    * Fully Covered Code
    * Manually Validated Code
    * Code Coverage Dead Code
    * Code Coverage Execution Count To Low
    * Implicitly Not Covered Code
    * Implicitly Covered Code
    * Implicit Manual Coverage Validation
* Reopen the file and check whether the format changes are applied correctly
