Qt Creator 20
=============

This document aims to summarize the API changes in selected libraries and
plugins.

| Before                                                    | After                                                                                      |
|-----------------------------------------------------------|--------------------------------------------------------------------------------------------|
|                                                           |                                                                                            |
| **General**                                               |                                                                                            |
|                                                           |                                                                                            |
| **Tasking**                                               |                                                                                            |
| #include \<QtTaskTree/QAbstractTaskTreeRunner\>           | Removed; use the exact runner's include. For example: \<QtTaskTree/QSingleTaskTreeRunner\> |
|                                                           |                                                                                            |
| **Utils**                                                 |                                                                                            |
| Utils::isMainThread()                                     | Removed; use `QThread::isMainThread()` instead                                             |
| Utils::DeviceShell                                        | Removed due to thread-safety/lock-up issues                                                |
| Utils::runEnvironmentItemsDialog()                        | Parameter and return value changed from `EnvironmentItems` to `EnvironmentChanges`         |
| **Utils::AspectList**                                     |                                                                                            |
| AspectList::setItem(Added\|Removed)Callback               | Replaced by public members item(Added\|Removed)Callback                                    |
| **Utils::EnvironmentModel**                               |                                                                                            |
| EnvironmentModel::changes()                               | Replaced by `hasExplicitChanges` and `hasChangesFromFile`                                  |
| EnvironmentModel::userChanges()                           | Removed; use the new `changes()` method                                                    |
| EnvironmentModel::setUserChanges()                        | Parameter was changed from `EnvironmentItems` to `EnvironmentChanges`                      |
| **Utils::FilePath**                                       |                                                                                            |
| FilePath::caseSensitivity()                               | Removed; use comparison methods instead                                                    |
| FilePath::equals()                                        | Removed; use `operator==()` instead                                                        |
| FilePath::startsWith()                                    | Removed; use `FilePath::isChildOf()` where applicable                                      |
| **Utils::HostOsInfo**                                     | Removed the fileNameCaseSensitivity related functions                                      |
| **Utils::IconDisplay**                                    | Renamed to `Utils::QtcIconDisplay` and moved to `qtcwidgets.h`                             |
| **Utils::NameValuesDialog**                               |                                                                                            |
| NameValuesDialog::(set)NameValueItems()                   | Replaced by `NameValuesDialog::(set)EnvChanges()`                                          |
| **Utils::PathChooser**                                    |                                                                                            |
| PathChooser::setDefaultValue(const QString &defaultValue) | Parameter changed to `FilePath`                                                            |
|                                                           |                                                                                            |
| **Core**                                                  |                                                                                            |
| **Core::EditorManager**                                   |                                                                                            |
| addSaveAndCloseEditorActions()                            | Removed; use `addContextMenuActions()` instead                                             |
| addPinEditorActions()                                     | Removed; use `addContextMenuActions()` instead                                             |
| addNativeDirAndOpenWithActions()                          | Removed; use `addContextMenuActions()` instead                                             |
| **Core::IDocumentFactory**                                | Must set `isProjectFactory` to false to be shown by File > Open File                       |
|                                                           |                                                                                            |
| **TextEditor**                                            |                                                                                            |
| **TextEditor::EditorConfiguration**                       |                                                                                            |
| EditorConfiguration::(set)\*Settings() accessors          | Replaced by direct member access                                                           |
| EditorConfiguration::\*SettingsChanged() signals          | Parameter changed from `*Settings` to `*SettingsData`                                      |
| **TextEditor::TextDocument**                              |                                                                                            |
| TextDocument::(set)\*Settings()                           | Changed to use `*SettingsData`                                                             |
| **TextEditor::TextEditorWidget**                          |                                                                                            |
| TextEditorWidget::(set)\*Settings()                       | Changed to use `*SettingsData`                                                             |
|                                                           |                                                                                            |
| **ProjectExplorer**                                       |                                                                                            |
| **ProjectExplorer::BuildConfiguration**                   |                                                                                            |
| BuildConfiguration::(set)UserEnvironmentChanges()         | Changed from `EnvironmentItems` to `EnvironmentChanges`                                    |
| **ProjectExplorer::DeploymentData**                       |                                                                                            |
| DeploymentData::setFileList()                             | Removed; use `addFile()`                                                                   |
| DeploymentData::fileCount() and fileAt()                  | Removed; use `allFiles()`                                                                  |
| **ProjectExplorer::Environment(Kit)Aspect(Widget)**       | `EnvironmentItems` based API was changed to `EnvironmentChanges`                           |
| **ProjectExplorer::Kit**                                  |                                                                                            |
| Kit::toMap()                                              | Changed to `toMap(Utils::Store &data)`                                                     |
| **ProjectExplorer::Project**                              | `EnvironmentItems` based API was changed to `EnvironmentChanges`                           |
| Project::projectImporter()                                | No longer `virtual`; use `setProjectImporter()`                                            |
| **ProjectExplorer::ProjectImporter**                      |                                                                                            |
| ProjectImporter::buildInfoList()                          | Changed to `buildInfo()`                                                                   |
| **ProjectExplorer::Node**                                 |                                                                                            |
| Node::compress()                                          | Replaced by `setCompressable()`                                                            |
| **ProjectExplorer::RunControl**                           |                                                                                            |
| RunControl::projectFilePath()                             | Removed                                                                                    |
| **ProjectExplorer::TreeScanner**                          | The class was replaced by a namespace                                                      |
