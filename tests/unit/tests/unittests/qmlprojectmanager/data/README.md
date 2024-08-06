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

### Qt for MCUs test sets

These tests define the requirements for Qt for MCUs (QUL) projects. They are maintained by the Qt for MCUs
team. Please do not make changes to these test sets before consulting them. These tests help make sure
that QDS and QUL are aligned on the qmlproject format and that new features in QDS does not break
qmlproject files for MCU projects. The test set will be tested in the Qt for MCUs repositories
to make sure both the original and the converted qmlprojects build correctly.

The test set also includes some dummy qmlproject files to test that the converter correctly picks up
qmlproject modules in the same way Qt for MCUs does.

The qmlproject files in the test set aim to cover all the possible contents of a Qt for MCUs qmlproject,
but since new features are added with every release, it is not guaranteed to be exhaustive.

Some main points for qmlproject files for MCU projects:

* Unknown content in the qmlproject file will cause errors in the QUL tooling
* Any node or property with the `QDS`-prefix is ignored by QUL. When adding new properties,
  these must either be prefixed or communicated with the MCU team to whitelist them in the tooling.
* Plain `Files` nodes are ignored by QUL, it needs to know the node type. The node contents will be processed
  by different tools depending on which type of files it includes. The node types used by QUL are:
  + `QmlFiles`
  + `ImageFiles`
  + `FontFiles`
  + `ModuleFiles`
  + `InterfaceFiles`
  + `TranslationFiles`
* In addition to adding files to the project, any of the `*Files` nodes listed can also contain properties to
  apply to the file group added by the node.
* MCU projects may have MCU-specific configuration nodes: `MCU.Config` and `MCU.Module`.
* A new version of Qt for MCUs may add new properties to any node, including `Project`. For this reason
  it is not possible to define an exact set of properties to support which is valid across all versions.
* Qt for MCUs developers must still edit and maintain qmlproject files manually. The converted format
  of the qmlproject file should be easy to read and edit by hand

## File Filters test data

Test data contains an example project folders that file filters will be initialized and tested.

* **filelist.txt**: List of the files need to be found by the file filters.
