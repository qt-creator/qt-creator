// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands.h"
#include "mcpservertr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/session.h>

#include <mcp/server/toolregistry.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filesearch.h>
#include <utils/id.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>
#include <utils/savefile.h>

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>
#include <QThread>
#include <QTimer>

using namespace Utils;

Q_LOGGING_CATEGORY(mcpCommands, "qtc.mcpserver.commands", QtWarningMsg)

namespace Mcp::Internal {

McpCommands::McpCommands(QObject *parent)
    : QObject(parent)
{}

Mcp::Schema::Tool::OutputSchema McpCommands::searchResultsSchema()
{
    static Mcp::Schema::Tool::OutputSchema cachedSchema = [] {
        QFile schemaFile(":/mcpserver/schemas/search-results-schema.json");
        if (!schemaFile.open(QIODevice::ReadOnly)) {
            qCWarning(mcpCommands)
                << "Failed to open schemas/search-results-schema.json from resources:"
                << schemaFile.errorString();
            return Mcp::Schema::Tool::OutputSchema{};
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(schemaFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(mcpCommands)
                << "Failed to parse search-results-schema.json:" << parseError.errorString();
            return Mcp::Schema::Tool::OutputSchema{};
        }

        QJsonObject obj = doc.object();
        Mcp::Schema::Tool::OutputSchema schema;
        //if (obj.contains("properties"))
        //    schema._properties = obj["properties"].toObject();
        if (obj.contains("required") && obj["required"].isArray()) {
            QStringList req;
            for (const QJsonValue &v : obj["required"].toArray())
                req.append(v.toString());
            schema._required = req;
        }
        return schema;
    }();

    return cachedSchema;
}

QString McpCommands::getVersion()
{
    return QCoreApplication::applicationVersion();
}

bool McpCommands::openFile(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    if (!filePath.exists()) {
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return false;
    }

    qCDebug(mcpCommands) << "Opening file:" << path;

    Core::EditorManager::openEditor(filePath);

    return true;
}

QString McpCommands::getFilePlainText(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return QString();
    }

    FilePath filePath = FilePath::fromUserInput(path);

    if (!filePath.exists()) {
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return QString();
    }

    if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath))
        return doc->plainText();

    MimeType mime = mimeTypeForFile(filePath);
    if (!mime.inherits("text/plain")) {
        qCDebug(mcpCommands) << "File is not a plain text document:" << path
                             << "MIME type:" << mime.name();
        return QString();
    }

    Result<QByteArray> contents = filePath.fileContents();
    if (contents.has_value())
        return Core::EditorManager::defaultTextEncoding().decode(*contents);

    qCDebug(mcpCommands) << "Failed to read file contents:" << path << "Error:" << contents.error();
    return QString();
}

QJsonObject McpCommands::setFilePlainText(const QString &path, const QString &contents)
{
    // Structured-result helper. AI consumers branch on `reason` to decide
    // what to do next (e.g. on "file_open_with_unsaved_changes", ask the
    // user to save and retry; on "not_text_file", give up). Including a
    // human-readable `message` keeps the failure self-describing without
    // forcing every caller to maintain a reason → message map.
    auto result = [](bool success, const QString &reason, const QString &message) {
        return QJsonObject{{"success", success}, {"reason", reason}, {"message", message}};
    };

    if (path.isEmpty())
        return result(false, "empty_path", "File path is empty.");

    FilePath filePath = FilePath::fromUserInput(path);

    if (!filePath.exists())
        return result(false, "file_not_found",
                      QStringLiteral("File does not exist: %1").arg(path));

    // If the file is open in an editor, route through the editor's
    // document so the user's view updates in-place. CRITICAL: refuse if
    // the buffer is dirty — overwriting unsaved edits is silent data loss.
    if (auto *doc = Core::DocumentModel::documentForFilePath(filePath)) {
        if (auto *textDoc = qobject_cast<TextEditor::TextDocument *>(doc)) {
            if (textDoc->isModified()) {
                return result(
                    false, "file_open_with_unsaved_changes",
                    QStringLiteral(
                        "The file is currently open in a Qt Creator editor with "
                        "unsaved changes. Refusing to overwrite to avoid losing "
                        "the user's edits. Ask the user to save the file (or "
                        "call save_file) and retry."));
            }
            if (const auto r = textDoc->setPlainText(contents); !r) {
                return result(false, "write_failed",
                              QStringLiteral("Content too large for editor: %1").arg(r.error()));
            }
            textDoc->document()->setModified(true);
            return result(true, "ok_buffer_updated",
                          QStringLiteral(
                              "Updated the editor buffer in-place. The change "
                              "is visible to the user but not yet on disk — "
                              "call save_file to persist."));
        }
    }

    // Otherwise write to disk directly. Guard against non-text files to
    // avoid corrupting binary content.
    MimeType mime = mimeTypeForFile(filePath);
    if (!mime.inherits("text/plain")) {
        qCDebug(mcpCommands) << "File is not a plain text document:" << path
                             << "MIME type:" << mime.name();
        return result(false, "not_text_file",
                      QStringLiteral("File is not plain text (MIME: %1). Refusing to "
                                     "overwrite binary content.").arg(mime.name()));
    }

    qCDebug(mcpCommands) << "Setting plain text for file:" << path;

    Result<qint64> writeResult = filePath.writeFileContents(
        Core::EditorManager::defaultTextEncoding().encode(contents));

    if (!writeResult) {
        qCDebug(mcpCommands) << "Failed to write file contents:" << path
                             << "Error:" << writeResult.error();
        return result(false, "write_failed",
                      QStringLiteral("Failed to write to disk: %1").arg(writeResult.error()));
    }
    return result(true, "ok_disk_write",
                  QStringLiteral("File written to disk successfully."));
}

bool McpCommands::saveFile(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    auto doc = Core::DocumentModel::documentForFilePath(filePath);

    if (!doc) {
        qCDebug(mcpCommands) << "No document found for file:" << path;
        return false;
    }

    if (!doc->isModified()) {
        qCDebug(mcpCommands) << "Document is not modified, no need to save:" << path;
        return true;
    }

    qCDebug(mcpCommands) << "Saving file:" << path;

    Result<> res = doc->save();
    if (!res)
        qCDebug(mcpCommands) << "Failed to save document:" << path << "Error:" << res.error();

    return res.has_value();
}

bool McpCommands::closeFile(const QString &path)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    auto doc = Core::DocumentModel::documentForFilePath(filePath);

    if (!doc) {
        qCDebug(mcpCommands) << "No document found for file:" << path;
        return false;
    }

    // Refuse to silently discard unsaved edits. The Core::EditorManager
    // variant we call below has askAboutModifiedEditors=false, so there
    // is no user prompt — and an MCP caller can't respond to one anyway.
    // Caller must save_file (or revert) before closing.
    if (doc->isModified()) {
        qCWarning(mcpCommands) << "Refusing to close modified document without save:" << path
                               << "- call save_file first";
        return false;
    }

    qCDebug(mcpCommands) << "Closing file:" << path;

    bool closed = Core::EditorManager::closeDocuments({doc}, false);
    if (!closed)
        qCDebug(mcpCommands) << "Failed to close document:" << path;

    return closed;
}

static FindFlags findFlags(bool regex, bool caseSensitive)
{
    FindFlags flags;
    if (regex)
        flags |= FindRegularExpression;
    if (caseSensitive)
        flags |= FindCaseSensitively;
    return flags;
}

static void findInFiles(
    FileContainer fileContainer,
    bool regex,
    bool caseSensitive,
    const QString &pattern,
    QObject *guard,
    const McpCommands::ResponseCallback &callback)
{
    const QFuture<SearchResultItems> future = Utils::findInFiles(
        pattern,
        fileContainer,
        findFlags(regex, caseSensitive),
        TextEditor::TextDocument::openedTextDocumentContents());
    Utils::onFinished(future, guard, [callback](const QFuture<SearchResultItems> &future) {
        QJsonArray resultsArray;
        for (Utils::SearchResultItems results : future.results()) {
            for (const SearchResultItem &item : results) {
                QJsonObject resultObj;
                const Text::Range range = item.mainRange();
                const QString lineText = item.lineText();
                const int startCol = range.begin.column;
                const int endCol = range.end.column;
                const QString matchedText = lineText.mid(startCol, endCol - startCol);

                resultObj["file"] = item.path().value(0, QString());
                resultObj["line"] = range.begin.line;
                resultObj["column"] = startCol + 1; // Convert 0-based to 1-based
                resultObj["text"] = matchedText;
                resultsArray.append(resultObj);
            }
        }

        QJsonObject response;
        response["results"] = resultsArray;
        callback(response);
    });
}

void McpCommands::searchInFile(
    const QString &path,
    const QString &pattern,
    bool regex,
    bool caseSensitive,
    const ResponseCallback &callback)
{
    const FilePath filePath = FilePath::fromUserInput(path);
    if (!filePath.exists()) {
        callback({});
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return;
    }

    TextEncoding encoding;
    if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath))
        encoding = doc->encoding();

    if (!encoding.isValid())
        encoding = Core::EditorManager::defaultTextEncoding();

    FileListContainer fileContainer({filePath}, {encoding});

    findInFiles(fileContainer, regex, caseSensitive, pattern, this, callback);
}

void McpCommands::searchInDirectory(
    const QString directory,
    const QString &pattern,
    bool regex,
    bool caseSensitive,
    const ResponseCallback &callback)
{
    const FilePath dirPath = FilePath::fromUserInput(directory);
    if (!dirPath.exists() || !dirPath.isDir()) {
        callback({});
        qCDebug(mcpCommands) << "Directory does not exist or is not a directory:" << directory;
        return;
    }

    SubDirFileContainer fileContainer({dirPath}, {}, {}, {});

    findInFiles(fileContainer, regex, caseSensitive, pattern, this, callback);
}

static void replace(
    FileContainer fileContainer,
    bool regex,
    bool caseSensitive,
    const QString &pattern,
    const QString &replacement,
    QObject *guard,
    const McpCommands::ResponseCallback &callback)
{
    const QFuture<SearchResultItems> future = Utils::findInFiles(
        pattern,
        fileContainer,
        findFlags(regex, caseSensitive),
        TextEditor::TextDocument::openedTextDocumentContents());
    Utils::onFinished(future, guard, [callback, replacement](const QFuture<SearchResultItems> &future) {
        QJsonObject response;
        bool success = true;

        TextEditor::PlainRefactoringFileFactory changes;

        QHash<Utils::FilePath, TextEditor::RefactoringFilePtr> refactoringFiles;
        for (const SearchResultItems &results : future.results()) {
            for (const SearchResultItem &item : results) {
                Text::Range range = item.mainRange();
                if (range.begin >= range.end)
                    continue;
                const FilePath filePath = FilePath::fromUserInput(item.path().value(0));
                if (filePath.isEmpty())
                    continue;
                TextEditor::RefactoringFilePtr refactoringFile = refactoringFiles.value(filePath);
                if (!refactoringFile)
                    refactoringFile
                        = refactoringFiles.insert(filePath, changes.file(filePath)).value();
                const int start = refactoringFile->position(range.begin);
                const int end = refactoringFile->position(range.end);
                ChangeSet changeSet = refactoringFile->changeSet();
                changeSet.replace(ChangeSet::Range(start, end), replacement);
                refactoringFile->setChangeSet(changeSet);
            }
        }

        for (auto refactoringFile : refactoringFiles) {
            if (!refactoringFile->apply()) {
                qCDebug(mcpCommands) << "Failed to apply changes for file:"
                                     << refactoringFile->filePath().toUserOutput();
                success = false;
            }
        }

        response["ok"] = success;
        callback(response);
    });
}

void McpCommands::replaceInFile(
    const QString &path,
    const QString &pattern,
    const QString &replacement,
    bool regex,
    bool caseSensitive,
    const ResponseCallback &callback)
{
    const FilePath filePath = FilePath::fromUserInput(path);
    if (!filePath.exists()) {
        callback({});
        qCDebug(mcpCommands) << "File does not exist:" << path;
        return;
    }

    TextEncoding encoding;
    if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath))
        encoding = doc->encoding();

    if (!encoding.isValid())
        encoding = Core::EditorManager::defaultTextEncoding();

    FileListContainer fileContainer({filePath}, {encoding});

    replace(fileContainer, regex, caseSensitive, pattern, replacement, this, callback);
}

void McpCommands::replaceInDirectory(
    const QString directory,
    const QString &pattern,
    const QString &replacement,
    bool regex,
    bool caseSensitive,
    const ResponseCallback &callback)
{
    const FilePath dirPath = FilePath::fromUserInput(directory);
    if (!dirPath.exists() || !dirPath.isDir()) {
        callback({});
        qCDebug(mcpCommands) << "Directory does not exist or is not a directory:" << directory;
        return;
    }

    SubDirFileContainer fileContainer({dirPath}, {}, {}, {});

    replace(fileContainer, regex, caseSensitive, pattern, replacement, this, callback);
}

bool McpCommands::quit()
{
    Core::ICore::exit();
    return true;
}

void McpCommands::executeCommand(
    const QString &command,
    const QString &arguments,
    const QString &workingDirectory,
    const ResponseCallback &callback)
{
    CommandLine cmd(FilePath::fromUserInput(command), arguments, CommandLine::Raw);
    auto process = new Process(this);
    connect(process, &Process::done, this, [process, callback]() {
        QJsonObject response;
        response["exitCode"] = process->exitCode();
        response["exitMessage"] = process->verboseExitMessage();
        response["stdout"] = process->readAllStandardOutput();
        response["stderr"] = process->readAllStandardError();
        callback(response);
        process->deleteLater();
    });
    process->setCommand(cmd);
    if (!workingDirectory.isEmpty())
        process->setWorkingDirectory(FilePath::fromUserInput(workingDirectory));
    process->start();
}

QStringList McpCommands::listOpenFiles()
{
    QStringList files;

    QList<Core::IDocument *> documents = Core::DocumentModel::openedDocuments();
    for (Core::IDocument *doc : documents) {
        files.append(doc->filePath().toUserOutput());
    }

    qCDebug(mcpCommands) << "Open files:" << files;

    return files;
}

QStringList McpCommands::listVisibleFiles()
{
    QStringList files;

    const QList<Core::IEditor *> editors = Core::EditorManager::visibleEditors();
    for (Core::IEditor *editor : editors) {
        if (auto doc = editor->document()) {
            if (editor == Core::EditorManager::currentEditor())
                files.prepend(doc->filePath().toUserOutput());
            else
                files.append(doc->filePath().toUserOutput());
        }
    }

    qCDebug(mcpCommands) << "Visible files:" << files;

    return files;
}

QStringList McpCommands::listSessions()
{
    QStringList sessions = Core::SessionManager::sessions();
    qCDebug(mcpCommands) << "Available sessions:" << sessions;
    return sessions;
}

QString McpCommands::getCurrentSession()
{
    QString session = Core::SessionManager::activeSession();
    qCDebug(mcpCommands) << "Current session:" << session;
    return session;
}

bool McpCommands::loadSession(const QString &sessionName)
{
    return Core::SessionManager::loadSession(sessionName);
}

bool McpCommands::saveSession()
{
    qCDebug(mcpCommands) << "Saving current session";

    bool successB = Core::SessionManager::saveSession();
    if (successB) {
        qCDebug(mcpCommands) << "Successfully saved session";
    } else {
        qCDebug(mcpCommands) << "Failed to save session";
    }

    return successB;
}

bool McpCommands::createNewFile(const QString &path, const QString &text)
{
    if (path.isEmpty()) {
        qCDebug(mcpCommands) << "Empty file path provided";
        return false;
    }

    FilePath filePath = FilePath::fromUserInput(path);

    if (filePath.exists()) {
        qCDebug(mcpCommands) << "File already exists:" << path;
        return false;
    }

    // Create parent directories if needed
    const FilePath parentDir = filePath.parentDir();
    if (!parentDir.exists()) {
        if (!parentDir.createDir()) {
            qCDebug(mcpCommands) << "Failed to create parent directories:"
                                 << parentDir.toUserOutput();
            return false;
        }
    }

    Result<qint64> result = filePath.writeFileContents(
        Core::EditorManager::defaultTextEncoding().encode(text));
    if (!result) {
        qCDebug(mcpCommands) << "Failed to create file:" << path << "Error:" << result.error();
        return false;
    }
    return true;
}

bool McpCommands::reformatFile(const QString &path)
{
    if (path.isEmpty())
        return false;

    const FilePath filePath = FilePath::fromUserInput(path);

    // If not already open, open it
    Core::IEditor *editor = Core::EditorManager::openEditor(filePath);
    if (!editor)
        return false;

    auto *textEditor = qobject_cast<TextEditor::TextEditorWidget *>(editor->widget());
    if (!textEditor)
        return false;

    // Select all text and reformat
    QTextCursor cursor = textEditor->textCursor();
    cursor.select(QTextCursor::Document);
    textEditor->setTextCursor(cursor);

    // Trigger the TextEditor.ReformatFile action
    Core::Command *cmd = Core::ActionManager::command(Utils::Id("TextEditor.ReformatFile"));
    if (cmd && cmd->action() && cmd->action()->isEnabled()) {
        cmd->action()->trigger();
        return true;
    }

    // Fallback: use auto-indent on the whole document
    textEditor->autoIndent();
    return true;
}

void McpCommands::registerCommands()
{
    using namespace Mcp::Schema;

    static McpCommands commands;

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;

    static const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    using Callback = std::function<void(const QJsonObject &response)>;
    using SimplifiedAsyncCallback = std::function<void(const QJsonObject &, const Callback &)>;
    static const auto wrapAsync =
        [](SimplifiedAsyncCallback asyncFunc) -> Mcp::Server::ToolInterfaceCallback {
        return [asyncFunc](
                   const Schema::CallToolRequestParams &params,
                   const ToolInterface &toolInterface) -> Utils::Result<> {
            asyncFunc(params.argumentsAsObject(), [toolInterface](QJsonObject result) {
                toolInterface.finish(CallToolResult{}.isError(false).structuredContent(result));
            });
            return ResultOk;
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("open_file")
            .title("Open a file in Qt Creator")
            .description("Open a file in Qt Creator")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file to open"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            bool ok = commands.openFile(path);
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("file_plain_text")
            .title("file plain text")
            .description(
                "Returns the content of the file as plain text. This also supports files on remote "
                "devices with uris like docker://... or ssh:// and others.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("text", QJsonObject{{"type", "string"}})
                    .addRequired("text")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            const QString text = commands.getFilePlainText(path);
            return QJsonObject{{"text", text}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("set_file_plain_text")
            .title("Overwrite the contents of a text file")
            .description(
                "Overwrite the file's text content with the provided string. "
                "Behavior depends on whether the file is currently open in a Qt "
                "Creator editor:\n"
                "  - Not open: writes directly to disk.\n"
                "  - Open with an unchanged buffer: updates the editor's in-memory "
                "    buffer (visible to the user immediately). The change is NOT "
                "    persisted to disk until save_file is called.\n"
                "  - Open with unsaved changes: REFUSED with reason "
                "    'file_open_with_unsaved_changes' to avoid silently "
                "    overwriting the user's edits. Caller should ask the user to "
                "    save (or call save_file) and retry.\n"
                "Also supports files on remote devices with URIs like "
                "docker://... or ssh:// and others.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file"}})
                    .addProperty(
                        "plainText",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Text to write into the file."}})
                    .addRequired("path")
                    .addRequired("plainText"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "reason",
                        QJsonObject{
                            {"type", "string"},
                            {"enum",
                             QJsonArray{"ok_buffer_updated", "ok_disk_write",
                                        "empty_path", "file_not_found",
                                        "file_open_with_unsaved_changes",
                                        "not_text_file", "write_failed"}},
                            {"description",
                             "Outcome category. Success values: "
                             "'ok_buffer_updated' (file open, buffer overwritten "
                             "— call save_file to persist) or 'ok_disk_write' "
                             "(file not open, written to disk). Failure values "
                             "explain why the write was refused or failed."}})
                    .addProperty(
                        "message",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Human-readable detail for the reason."}})
                    .addRequired("success")
                    .addRequired("reason")
                    .addRequired("message")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            const QString text = p.value("plainText").toString();
            return commands.setFilePlainText(path, text);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("save_file")
            .title("Save a file in Qt Creator")
            .description("Save a file in Qt Creator")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file to save"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            bool ok = commands.saveFile(path);
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("close_file")
            .title("Close a file in Qt Creator")
            .description("Close a file in Qt Creator")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file to close"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            bool ok = commands.closeFile(path);
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("search_in_file")
            .title("Search for pattern in a single file")
            .description(
                "Search for a text pattern in a single file and return all matches with "
                "line, column, and matched text")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file to search"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "caseSensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("path")
                    .addRequired("pattern"))
            .outputSchema(McpCommands::searchResultsSchema()),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            commands.searchInFile(path, pattern, isRegex, caseSensitive, callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("search_in_directory")
            .title("Search for pattern in a directory")
            .description(
                "Search for a text pattern recursively in all files within a directory "
                "and return all matches")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "directory",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the directory to search in"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "caseSensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("directory")
                    .addRequired("pattern"))
            .outputSchema(McpCommands::searchResultsSchema()),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString directory = p.value("directory").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            commands.searchInDirectory(directory, pattern, isRegex, caseSensitive, callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("replace_in_file")
            .title("Replace pattern in a single file")
            .description(
                "Replace all matches of a text pattern in a single file with replacement text")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file to modify"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "replacement",
                        QJsonObject{{"type", "string"}, {"description", "Replacement text"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "caseSensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("path")
                    .addRequired("pattern")
                    .addRequired("replacement"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("ok", QJsonObject{{"type", "boolean"}})
                    .addRequired("ok")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            commands.replaceInFile(path, pattern, replacement, isRegex, caseSensitive, callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("replace_in_directory")
            .title("Replace pattern in a directory")
            .description(
                "Replace all matches of a text pattern recursively in all files within "
                "a directory with replacement text")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "directory",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the directory to search in"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "replacement",
                        QJsonObject{{"type", "string"}, {"description", "Replacement text"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "caseSensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("directory")
                    .addRequired("pattern")
                    .addRequired("replacement"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("ok", QJsonObject{{"type", "boolean"}})
                    .addRequired("ok")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString directory = p.value("directory").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            commands.replaceInDirectory(
                directory, pattern, replacement, isRegex, caseSensitive, callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_open_files")
            .title("List currently open files")
            .description("List currently open files")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "openFiles",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("openFiles")),
        wrap([](const QJsonObject &) {
            const QStringList files = commands.listOpenFiles();
            QJsonArray arr;
            for (const QString &f : files)
                arr.append(f);
            return QJsonObject{{"openFiles", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_visible_files")
            .title("List currently visible files")
            .description("List all files that are currently visible to the user in an editor.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "visibleFiles",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("visibleFiles")),
        wrap([](const QJsonObject &) {
            const QStringList files = commands.listVisibleFiles();
            QJsonArray arr;
            for (const QString &f : files)
                arr.append(f);
            return QJsonObject{{"visibleFiles", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_sessions")
            .title("List available sessions")
            .description("List available sessions")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "sessions",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("sessions")),
        wrap([](const QJsonObject &) {
            const QStringList sessions = commands.listSessions();
            QJsonArray arr;
            for (const QString &s : sessions)
                arr.append(s);
            return QJsonObject{{"sessions", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("load_session")
            .title("Load a specific session")
            .description("Load a specific session")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "sessionName",
                        QJsonObject{
                            {"type", "string"}, {"description", "Name of the session to load"}})
                    .addRequired("sessionName"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString name = p.value("sessionName").toString();
            bool ok = commands.loadSession(name);
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("quit")
            .title("Quit Qt Creator")
            .description("Quit Qt Creator")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) {
            bool ok = commands.quit();
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_current_session")
            .title("Get the currently active session")
            .description("Get the currently active session")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("session", QJsonObject{{"type", "string"}})
                    .addRequired("session")),
        wrap([](const QJsonObject &) {
            QString sess = commands.getCurrentSession();
            return QJsonObject{{"session", sess}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("save_session")
            .title("Save the current session")
            .description("Save the current session")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) {
            bool ok = commands.saveSession();
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool()
            .name("execute_command")
            .title("executes the command")
            .description(
                "executes the command and returns the exit code as well as standard output and "
                "error")
            .inputSchema(
                Tool::InputSchema()
                    .addRequired("command")
                    .addProperty(
                        "command",
                        QJsonObject{{"type", "string"}, {"description", "Command to execute"}})
                    .addProperty(
                        "arguments",
                        QJsonObject{
                            {"type", "string"}, {"description", "Arguments passed to the command"}})
                    .addProperty(
                        "workingDir",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Directory in which the command is executed"}}))
            .outputSchema(
                Tool::OutputSchema()
                    .addRequired("exitCode")
                    .addProperty(
                        "exitCode",
                        QJsonObject{{"type", "integer"}, {"description", "Exit code of the command"}})
                    .addProperty(
                        "exitMessage",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Verbose exit message of the command, useful for error reporting"}})
                    .addProperty(
                        "stdout",
                        QJsonObject{
                            {"type", "string"}, {"description", "Standard output of the command"}})
                    .addProperty(
                        "stderr",
                        QJsonObject{
                            {"type", "string"}, {"description", "Standard error of the command"}})),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            commands.executeCommand(
                p["command"].toString(),
                p["arguments"].toString(),
                p["workingDir"].toString(),
                callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("find_actions")
            .title("Find actions")
            .description("Finds actions matching a query string")
            .inputSchema(
                Tool::InputSchema{}.addProperty(
                    "query",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "String to search for in action names"}}))
            .outputSchema(
                Tool::OutputSchema{}.addProperty(
                    "actions",
                    QJsonObject{
                        {"type", "array"},
                        {"items",
                         QJsonObject{
                             {"type", "object"},
                             {"properties",
                              QJsonObject{
                                  {"id", QJsonObject{{"type", "string"}}},
                                  {"text", QJsonObject{{"type", "string"}}},
                                  {"description", QJsonObject{{"type", "string"}}},
                              }},
                             {"required", QJsonArray{"id", "text"}}}},
                        {"description", "List of matching actions"}})),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QString query = params.argumentsAsObject().value("query").toString();
            QList<Core::Command *> matches;
            for (Core::Command *cmd : Core::ActionManager::commands()) {
                const QString text = cmd->action() ? cmd->action()->text() : QString();
                if (text.contains(query, Qt::CaseInsensitive))
                    matches.append(cmd);
            }
            QJsonArray actions;
            for (const auto &m : matches) {
                const QString text = m->action() ? m->action()->text() : QString();
                const QString description = m->description();
                actions.append(
                    QJsonObject{
                        {"id", m->id().toString()}, {"text", text}, {"description", description}});
            }
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"actions", actions}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("call_action")
            .title("Call an action")
            .description("Calls an action by its ID")
            .inputSchema(
                Tool::InputSchema{}.addProperty(
                    "id",
                    QJsonObject{{"type", "string"}, {"description", "ID of the action to call"}})),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const QString id = p.value("id").toString();
            Core::Command *cmd = Core::ActionManager::command(Id::fromString(id));
            if (!cmd)
                return ResultError(Tr::tr("No action found with ID \"%1\".").arg(id));
            if (!cmd->action())
                return ResultError(Tr::tr("Command \"%1\" has no associated action.").arg(id));
            if (!cmd->action()->isEnabled())
                return ResultError(Tr::tr("Action \"%1\" is disabled.").arg(cmd->action()->text()));

            cmd->action()->trigger();
            return CallToolResult{}.isError(false);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("create_new_file")
            .title("Create a new file")
            .description(
                "Creates a new file at the specified path and optionally populates it with text. "
                "Creates parent directories automatically. Fails if the file already exists.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path where the file should be created"}})
                    .addProperty(
                        "text",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Optional content to write into the new file"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            const QString text = p.value("text").toString();
            bool ok = commands.createNewFile(path, text);
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("reformat_file")
            .title("Reformat a file")
            .description(
                "Reformats a specified file using Qt Creator's code formatting rules. "
                "Opens the file if not already open.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the file to reformat"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            bool ok = commands.reformatFile(path);
            return QJsonObject{{"success", ok}};
        }));
}

} // namespace Mcp::Internal
