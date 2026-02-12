Qt Creator 19
=============

This document aims to summarize the API changes in selected libraries and
plugins.

| Before                                                                | After                                                                      |
|-----------------------------------------------------------------------|----------------------------------------------------------------------------|
|                                                                       |                                                                            |
| **General**                                                           |                                                                            |
|                                                                       |                                                                            |
| **Layouting**                                                         |                                                                            |
|                                                                       |                                                                            |
| **Tasking**                                                           |                                                                            |
| Qt independent implementation                                         | Switched to a Qt 11 compatible copy, or the version in Qt if available     |
|                                                                       | Includes changed from "solutions/tasking/..." to "QtTaskTree/..."          |
|                                                                       | Include file names and class names changed to the Qt naming style          |
|                                                                       |                                                                            |
| **Utils**                                                             |                                                                            |
| onResultReady(QFuture, R *, void(R::*)(T))                            | Removed                                                                    |
| onFinished(QFuture, R *, void(R::*)(QFuture))                         | Removed                                                                    |
| **Utils::FilePath**                                                   |                                                                            |
| bool ensureReachable                                                  | Result<> ensureReachable                                                   |
| DeviceFileAccess *fileAccess                                          | std::shared_ptr\<DeviceFileAccess\> fileAccess                             |
| **Utils::Process**                                                    |                                                                            |
| runBlocking(seconds timeout, EventLoopMode eventLoopMode)             | runBlocking(seconds timeout), running an event loop is no longer supported |
|                                                                       |                                                                            |
| **ExtensionSystem**                                                   |                                                                            |
| **ExtensionSystem::PluginManager**                                    |                                                                            |
| PluginManager::settings                                               | Utils::userSettings                                                        |
| PluginManager::globalSettings                                         | Utils::installSettings                                                     |
|                                                                       |                                                                            |
| **Core**                                                              |                                                                            |
| **Core::BaseFileWizardFactory**                                       |                                                                            |
| GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) | Utils::Result\<GeneratedFiles\> generateFiles(const QWizard *w)            |
| **Core::IVersionControl**                                             |                                                                            |
| monitorDirectory(FilePath)                                            | monitorDirectory(FilePath, bool)                                           |
| stopMonitoringDirectory(FilePath)                                     | Removed, use monitorDirectory(FilePath, false)                             |
| updateFileStatus                                                      | updateFileState                                                            |
| clearFileStatus                                                       | clearFileState                                                             |
| modificationState                                                     | fileState                                                                  |
|                                                                       |                                                                            |
| **TextEditor**                                                        |                                                                            |
|                                                                       |                                                                            |
| **ProjectExplorer**                                                   |                                                                            |
| DeviceProcessSignalOperation                                          | Removed                                                                    |
| **ProjectExplorer::IDevice**                                          |                                                                            |
| signalOperation                                                       | Replaced by signalOperationRecipe                                          |
| autoDetectDeviceTools                                                 | Replaced by autoDetectDeviceToolsRecipe                                    |
