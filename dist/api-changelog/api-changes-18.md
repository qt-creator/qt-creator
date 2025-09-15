Qt Creator 18
=============

This document aims to summarize the API changes in selected libraries and
plugins.

| Before                                                           | After                                                                          |
|------------------------------------------------------------------|--------------------------------------------------------------------------------|
|                                                                  |                                                                                |
| **General**                                                      |                                                                                |
| QTextCodec *                                                     | Replaced by Utils::TextEncoding                                                |
|                                                                  |                                                                                |
| **Layouting**                                                    |                                                                                |
| If (*condition*, { *then-case* }, { *else-case* })               | If (*condition*) >> Then { *then-case* } >> Else { *else-case* }               |
|                                                                  |                                                                                |
| **Tasking**                                                      |                                                                                |
| CallDoneIf::Success                                              | Renamed to CallDone::OnSuccess                                                 |
| CallDoneIf::Error                                                | Renamed to CallDone::OnError                                                   |
| CallDoneIf::SuccessOrError                                       | Renamed to CallDone::Always                                                    |
| -                                                                | Added CallDone::OnCancel                                                       |
| MultiBarrier                                                     | Renamed to StoredMultiBarrier                                                  |
| NetworkOperation                                                 | Replaced by QNetworkAccessManager::Operation                                   |
| SharedBarrier                                                    | Renamed to StartedBarrier and derived from Barrier                             |
| SingleBarrier                                                    | Renamed to StoredBarrier                                                       |
| TaskTreeRunner                                                   | Renamed to SingleTaskTreeRunner                                                |
|                                                                  |                                                                                |
| **Utils**                                                        |                                                                                |
| makeWritable                                                     | FilePath::makeWritable                                                         |
| StyleHelper::SpacingTokens                                       | Were renamed and adapted to the new design                                     |
| Text::positionInText                                             | The column parameter is now 0-based                                            |
| toFilePathList                                                   | FilePaths::fromStrings                                                         |
| **Utils::FilePath**                                              |                                                                                |
| fromStrings                                                      | FilePaths::fromStrings                                                         |
| fromSettingsList                                                 | FilePaths::fromSettings                                                        |
| **Utils::FilePathAspect**                                        |                                                                                |
| FilePath baseFileName()                                          | Lazy\<FilePath\> baseFileName()                                                |
| **Utils::FilePaths**                                             |                                                                                |
| alias for QList\<FilePath\>                                      | Custom class derived from QList\<FilePath>                                     |
| **Utils::Link**                                                  |                                                                                |
| targetLine and targetColumn members                              | Merged into one Text::Position member                                          |
| **Utils::PathChooser**                                           |                                                                                |
| FilePath baseDirectory()                                         | Lazy\<FilePath\> baseDirectory()                                               |
| **Utils::TextFileFormat**                                        |                                                                                |
| QStringList-based decode                                         | Removed, use the QString-based method                                          |
| static readFile                                                  | Made a member function instead of returning TextFileFormat as output parameter |
| readFile QString(List) and decodingErrorSample output parameters | Removed, part of ReadResult now                                                |
| static void detect                                               | void detectFromData                                                            |
|                                                                  |                                                                                |
| **ExtensionSystem**                                              |                                                                                |
|                                                                  |                                                                                |
| **Core**                                                         |                                                                                |
| **Core::ExternalToolManager**                                    |                                                                                |
| readSettings and parseDirectory                                  | Removed                                                                        |
| **Core::TextDocument**                                           |                                                                                |
| read QString(List) output parameters                             | ReadResult includes the read contents                                          |
|                                                                  |                                                                                |
| **TextEditor**                                                   |                                                                                |
| **TextEditor::TabSettings**                                      |                                                                                |
| removeTrailingWhitespace(QTextCursor cursor, QTextBlock &block)  | removeTrailingWhitespace(const QTextBlock &block)                              |
|                                                                  |                                                                                |
| **ProjectExplorer**                                              |                                                                                |
| **ProjectExplorer::BuildConfiguration**                          |                                                                                |
| rawBuildDirectory                                                | Removed                                                                        |
| **ProjectExplorer::DetectionSource**                             | Has been added and is used for various detectionSource() getters               |
| **ProjectExplorer::ITaskHandler**                                |                                                                                |
| actionManagerId and createAction                                 | Removed, pass them to the constructor instead                                  |
| **ProjectExplorer::RawProjectPart**                              |                                                                                |
| setIncludeFiles                                                  | Changed to use Utils::FilePaths                                                |
| **ProjectExplorer::Task**                                        |                                                                                |
| Members                                                          | Are replaced by setters and getters                                            |
