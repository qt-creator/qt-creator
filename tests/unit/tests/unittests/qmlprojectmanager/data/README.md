# Test Set Information

This document contains information about the purpose of each test sets.

## Getter/Setter test data

* **testfile-1.qmlproject**: QmlProject file with properly filled out object
* **testfile-2.qmlproject**: QmlProject file with empty objects

## Converter test data

Test functions iterate over the "test-set-*" folders and run the tests by using the files inside them.

* **testfile.qmlproject**: Original QmlProject file that'll be converted
* **testfile.qmltojson**: JSON-converted version of the .qmlproject file
* **testfile.jsontoqml**: QmlProject-converted version of the .qmltojson file

### test-set-1

* **purpose**: testing complex qmlproject file convertion
* **origin**: custom project

### test-set-2

* **purpose**: testing fileselectors
* **origin**: file selectors example from playground

### test-set-3

* **purpose**: testing `QDS.` prefixes
* **origin**: copy of test-set-1


## File Filters test data

Test data contains an example project folders that file filters will be initialized and tested.

* **filelist.txt**: List of the files need to be found by the file filters.
