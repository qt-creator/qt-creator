// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands.h"
#include "mcpservertr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/outputwindow.h>
#include <coreplugin/session.h>

#include <mcp/server/toolregistry.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/filepathinfo.h>
#include <utils/filesearch.h>
#include <utils/id.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>
#include <utils/storekey.h>

#include <QAbstractButton>
#include <QApplication>
#include <QCursor>
#include <QBuffer>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QAction>
#include <QLoggingCategory>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPixmap>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QUrl>

using namespace Utils;

static Q_LOGGING_CATEGORY(mcpCommands, "qtc.mcpserver.commands", QtWarningMsg)

namespace Mcp::Internal {

// Turns a urlish tool argument into a FilePath. A "file://" URL denotes a local
// file; routed through fromUrl() it becomes a scheme-less local path. Remote paths
// such as "ssh://user@host/path" or "docker://id/path" keep their scheme via
// fromUserInput(), which is how Qt Creator parses such device paths.
static FilePath urlishToFilePath(const QString &path)
{
    if (path.startsWith("file:"))
        return FilePath::fromUrl(QUrl(path));
    return FilePath::fromUserInput(path);
}

// Runs a blocking function on a worker thread and delivers its result to the callback
// on the main thread, so remote file access does not block the GUI thread. The shutdown
// guard owns the watcher, so a pending callback is cancelled at controlled plugin
// shutdown rather than firing into a torn-down state.
static void runFileOpOffThread(
    const std::function<QJsonObject()> &work, const std::function<void(const QJsonObject &)> &callback)
{
    QFuture<QJsonObject> future = Utils::asyncRun(work);
    Utils::onResultReady(future, Utils::shutdownGuard(), [callback](const QJsonObject &result) {
        callback(result);
    });
}

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

bool McpCommands::openFile(const QString &path, int line, int column)
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

    if (line > 0)
        Core::EditorManager::openEditorAt({filePath, line, column > 0 ? column - 1 : 0});
    else
        Core::EditorManager::openEditor(filePath);

    return true;
}

static QString sliceLines(const QString &text, int startLine, int endLine)
{
    if (startLine <= 0 && endLine <= 0)
        return text;
    const QStringList lines = text.split('\n');
    const int first = startLine > 0 ? startLine - 1 : 0;
    const int last = endLine > 0 ? qMin(endLine, lines.size()) : lines.size();
    if (first >= lines.size())
        return QString();
    return QStringList(lines.mid(first, last - first)).join('\n');
}

QString McpCommands::getFilePlainText(const QString &path, int startLine, int endLine)
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

    QString text;
    if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath))
        text = doc->plainText();
    else {
        MimeType mime = mimeTypeForFile(filePath);
        if (!mime.inherits("text/plain")) {
            qCDebug(mcpCommands) << "File is not a plain text document:" << path
                                 << "MIME type:" << mime.name();
            return QString();
        }

        Result<QByteArray> contents = filePath.fileContents();
        if (!contents.has_value()) {
            qCDebug(mcpCommands) << "Failed to read file contents:" << path
                                 << "Error:" << contents.error();
            return QString();
        }
        text = Core::EditorManager::defaultTextEncoding().decode(*contents);
    }

    return sliceLines(text, startLine, endLine);
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

QJsonObject McpCommands::writeFileContentsBase64(const QString &path, const QString &base64)
{
    // Binary write path: decode the base64 payload and write the raw bytes to
    // disk. Unlike setFilePlainText() this does not route through the editor or
    // guard against non-text MIME types, since the caller explicitly asked for a
    // byte-exact write (e.g. of a binary artifact).
    auto result = [](bool success, const QString &reason, const QString &message) {
        return QJsonObject{{"success", success}, {"reason", reason}, {"message", message}};
    };

    if (path.isEmpty())
        return result(false, "empty_path", "File path is empty.");

    const QByteArray data = QByteArray::fromBase64(base64.toLatin1());
    const Result<qint64> writeResult = urlishToFilePath(path).writeFileContents(data);
    if (!writeResult) {
        return result(false, "write_failed",
                      QStringLiteral("Failed to write to disk: %1").arg(writeResult.error()));
    }
    return result(true, "ok_disk_write", QStringLiteral("File written to disk successfully."));
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
        response["exit_code"] = process->exitCode();
        response["exit_message"] = process->verboseExitMessage();
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

    const QList<Core::IDocument *> documents = Core::DocumentModel::openedDocuments();
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

static QString pluginStateName(ExtensionSystem::PluginSpec::State state)
{
    switch (state) {
    case ExtensionSystem::PluginSpec::Invalid: return "invalid";
    case ExtensionSystem::PluginSpec::Read: return "read";
    case ExtensionSystem::PluginSpec::Resolved: return "resolved";
    case ExtensionSystem::PluginSpec::Loaded: return "loaded";
    case ExtensionSystem::PluginSpec::Initialized: return "initialized";
    case ExtensionSystem::PluginSpec::Running: return "running";
    case ExtensionSystem::PluginSpec::Stopped: return "stopped";
    case ExtensionSystem::PluginSpec::Deleted: return "deleted";
    }
    return "unknown";
}

// ---- Widget resolution ------------------------------------------------
//
// A semantic addressing layer over the running UI: scenarios say "the OK
// button in the Execute Extension Command dialog", not screen coordinates.
// A query is a conjunction of the fields below; resolveWidgets() walks every
// live widget and returns the matches. The action tools treat more than one
// match as an error - an ambiguous query is a bug in the scenario, not a
// coin toss.

struct WidgetQuery
{
    QString objectName;
    QString text;
    QString className;
    QString windowTitle;
    bool includeInvisible = false;
};

static WidgetQuery widgetQueryFromJson(const QJsonObject &p)
{
    WidgetQuery q;
    q.objectName = p.value("object_name").toString();
    q.text = p.value("text").toString();
    q.className = p.value("class_name").toString();
    q.windowTitle = p.value("window_title").toString();
    q.includeInvisible = p.value("include_invisible").toBool(false);
    return q;
}

static bool widgetQueryIsEmpty(const WidgetQuery &q)
{
    return q.objectName.isEmpty() && q.text.isEmpty() && q.className.isEmpty()
           && q.windowTitle.isEmpty();
}

// The visible, human-readable text of a widget, used both to match a query
// and to describe a resolved widget in the generated tutorial. Accelerator
// markers ('&') are stripped so a query text of "OK" matches a "&OK" button.
static QString widgetVisibleText(const QWidget *w)
{
    QString text;
    if (auto b = qobject_cast<const QAbstractButton *>(w))
        text = b->text();
    else if (auto l = qobject_cast<const QLabel *>(w))
        text = l->text();
    else if (auto c = qobject_cast<const QComboBox *>(w))
        text = c->currentText();
    else if (auto g = qobject_cast<const QGroupBox *>(w))
        text = g->title();
    else if (auto le = qobject_cast<const QLineEdit *>(w))
        text = le->text();
    return text.remove('&');
}

// The text of the QLabel this widget is a buddy of (via setBuddy or an
// Alt-mnemonic), i.e. the caption next to an input such as a QLineEdit. Lets a
// query address "the field labelled X" when the input has no text of its own.
// Empty if no label points at the widget.
static QString buddyText(const QWidget *w)
{
    const QWidget *win = w->window();
    if (!win)
        return {};
    for (QLabel *label : win->findChildren<QLabel *>()) {
        if (label->buddy() == w)
            return QString(label->text()).remove('&');
    }
    return {};
}

static bool widgetMatches(const QWidget *w, const WidgetQuery &q)
{
    if (!q.includeInvisible && !w->isVisible())
        return false;
    if (!q.objectName.isEmpty() && w->objectName() != q.objectName)
        return false;
    if (!q.className.isEmpty()
        && QString::fromLatin1(w->metaObject()->className()) != q.className
        && !w->inherits(q.className.toLatin1().constData())) {
        return false;
    }
    if (!q.text.isEmpty()) {
        const QString want = q.text.trimmed();
        // Match the widget's own text, or - for a labelled input - its buddy
        // label's text. A label that captions another widget (has a buddy) is
        // NOT matched by its own text: that text identifies the buddy field,
        // so "the field labelled X" resolves to the field, not its caption.
        const auto label = qobject_cast<const QLabel *>(w);
        const bool ownTextMatches = !(label && label->buddy())
                                    && widgetVisibleText(w).trimmed() == want;
        if (!ownTextMatches && buddyText(w).trimmed() != want)
            return false;
    }
    if (!q.windowTitle.isEmpty()) {
        const QWidget *win = w->window();
        if (!win || !win->windowTitle().contains(q.windowTitle, Qt::CaseInsensitive))
            return false;
    }
    return true;
}

static QList<QWidget *> resolveWidgets(const WidgetQuery &q)
{
    QList<QWidget *> result;
    for (QWidget *w : QApplication::allWidgets()) {
        if (widgetMatches(w, q))
            result.append(w);
    }
    return result;
}

static QJsonObject describeWidget(QWidget *w)
{
    QWidget *win = w->window();
    const QPoint topLeft = w->mapToGlobal(QPoint(0, 0));
    QJsonObject result{
        {"class", QString::fromLatin1(w->metaObject()->className())},
        {"object_name", w->objectName()},
        {"text", widgetVisibleText(w)},
        {"visible", w->isVisible()},
        {"enabled", w->isEnabled()},
        {"x", topLeft.x()},
        {"y", topLeft.y()},
        {"width", w->width()},
        {"height", w->height()},
        {"window_title", win ? win->windowTitle() : QString()},
        // winId() would force-create a native handle on an unmapped window,
        // so only report it for a window that is actually on screen.
        {"window_id", (win && win->isVisible()) ? double(win->winId()) : 0}};
    if (const QString buddy = buddyText(w); !buddy.isEmpty())
        result.insert("buddy_text", buddy);
    return result;
}

static QString describeWidgetShort(QWidget *w)
{
    return QString("%1(object_name=\"%2\", text=\"%3\")")
        .arg(QString::fromLatin1(w->metaObject()->className()), w->objectName(),
             widgetVisibleText(w));
}

// Resolves a query to exactly one widget or explains why it could not: empty
// query, no match, or - the case that keeps tests honest - more than one
// match.
static Result<QWidget *> resolveSingleWidget(const WidgetQuery &q)
{
    if (widgetQueryIsEmpty(q)) {
        return ResultError(QString("Empty widget query; specify at least one of "
                                   "object_name, text, class_name, window_title."));
    }
    const QList<QWidget *> matches = resolveWidgets(q);
    if (matches.isEmpty())
        return ResultError(QString("No widget matched the query."));
    if (matches.size() > 1) {
        QStringList desc;
        for (QWidget *w : matches)
            desc << describeWidgetShort(w);
        return ResultError(
            QString("Ambiguous widget query: %1 matches [%2]. Narrow it with "
                    "object_name, class_name or window_title.")
                .arg(matches.size())
                .arg(desc.join(", ")));
    }
    return matches.first();
}

// Resolves a query to the widget that should receive a key event. Unlike
// resolveSingleWidget this accepts a window-level query (e.g. window_title
// alone, which matches every widget in that window): the keys then go to that
// window's focused widget - "press Enter in the About dialog" - rather than
// failing as ambiguous. Still an error if the matches span several windows.
static Result<QWidget *> resolveKeyTarget(const WidgetQuery &q)
{
    if (widgetQueryIsEmpty(q)) {
        if (QWidget *focus = QApplication::focusWidget())
            return focus;
        return ResultError(QString("No target widget: give a query or focus one."));
    }
    const QList<QWidget *> matches = resolveWidgets(q);
    if (matches.isEmpty())
        return ResultError(QString("No widget matched the query."));
    if (matches.size() == 1)
        return matches.first();
    QWidgetList windows;
    for (QWidget *w : matches) {
        if (QWidget *win = w->window(); win && !windows.contains(win))
            windows.append(win);
    }
    if (windows.size() != 1) {
        return ResultError(QString("Query matches %1 widgets across %2 windows; narrow it.")
                               .arg(matches.size())
                               .arg(windows.size()));
    }
    QWidget *window = windows.first();
    return window->focusWidget() ? window->focusWidget() : window;
}

// Prefer the widget's own behaviour over synthesising input: a button's
// click() runs its logic directly, avoiding the popup/dropdown pitfalls of
// posting raw mouse events. Other widgets do get a synthetic left click at
// their centre, delivered in-process so no XTEST or X server round trip is
// needed.
static void clickWidget(QWidget *w)
{
    if (auto b = qobject_cast<QAbstractButton *>(w)) {
        b->click();
        return;
    }
    const QPointF center = w->rect().center();
    const QPointF global = w->mapToGlobal(w->rect().center());
    QMouseEvent press(
        QEvent::MouseButtonPress, center, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(
        QEvent::MouseButtonRelease, center, global, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(w, &press);
    QApplication::sendEvent(w, &release);
}

// Delivers text as key events so widgets that react to typing (line edits,
// text editors) update as if the user typed. Sent synchronously so the
// widget state is settled before the tool returns.
static void typeText(QWidget *target, const QString &text)
{
    for (const QChar &ch : text) {
        QKeyEvent press(QEvent::KeyPress, 0, Qt::NoModifier, QString(ch));
        QKeyEvent release(QEvent::KeyRelease, 0, Qt::NoModifier, QString(ch));
        QApplication::sendEvent(target, &press);
        QApplication::sendEvent(target, &release);
    }
}

// Keeps only the last maxLines lines of text (all of it when maxLines <= 0),
// so a caller reading a large output pane can ask for just the recent tail.
static QString lastLines(const QString &text, int maxLines)
{
    if (maxLines <= 0)
        return text;
    const QStringList lines = text.split('\n');
    if (lines.size() <= maxLines)
        return text;
    return QStringList(lines.mid(lines.size() - maxLines)).join('\n');
}

void McpCommands::registerCommands()
{
    using namespace Mcp::Schema;
    using ExtensionSystem::PluginManager;
    using ExtensionSystem::PluginSpec;

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
            .description("Open a file in Qt Creator, optionally jumping to a specific line and column.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Absolute path of the file to open"}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line number to jump to (optional)"}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column number to jump to (optional, requires line)"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            const int line = p.value("line").toInt(0);
            const int column = p.value("column").toInt(0);
            bool ok = commands.openFile(path, line, column);
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("file_plain_text")
            .title("file plain text")
            .description(
                "Returns the content of the file as plain text. Optionally restrict to a line "
                "range via start_line/end_line (1-based, inclusive). For binary content use "
                "read_file_bytes instead. Local and remote files are both supported via Qt "
                "Creator urlish paths such as ssh://user@host/path or docker://id/path; remote "
                "access is transparent.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description",
                             "Path of the file. May be a local path or a remote urlish path "
                             "(ssh://user@host/path, docker://id/path)."}})
                    .addProperty(
                        "start_line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "First line to return, 1-based inclusive (optional)"}})
                    .addProperty(
                        "end_line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Last line to return, 1-based inclusive (optional)"}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("text", QJsonObject{{"type", "string"}})
                    .addRequired("text")),
        wrap([](const QJsonObject &p) {
            const QString path = p.value("path").toString();
            const int startLine = p.value("start_line").toInt(0);
            const int endLine = p.value("end_line").toInt(0);
            const QString text = commands.getFilePlainText(path, startLine, endLine);
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
                "For binary content use write_file_bytes instead. Also supports files "
                "on remote devices with URIs like docker://... or ssh:// and others.")
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
                        "plain_text",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Text to write into the file."}})
                    .addRequired("path")
                    .addRequired("plain_text"))
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
            const QString text = p.value("plain_text").toString();
            return commands.setFilePlainText(path, text);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("read_file_bytes")
            .title("Read raw bytes from a file")
            .description(
                "Reads raw file contents and returns them base64-encoded - use this for binary "
                "files; for text use file_plain_text. Optionally restrict to a byte range via "
                "offset/length. Local and remote files are both supported via Qt Creator urlish "
                "paths such as ssh://user@host/path or docker://id/path; remote access is "
                "transparent.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Path of the file (local or remote urlish path)."}})
                    .addProperty(
                        "offset",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Byte offset to start reading from (optional)."}})
                    .addProperty(
                        "length",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Maximum number of bytes to read (optional)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("base64", QJsonObject{{"type", "string"}})
                    .addProperty("size", QJsonObject{{"type", "integer"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const qint64 offset = qint64(p.value("offset").toDouble(0));
            const qint64 length = qint64(p.value("length").toDouble(-1));
            runFileOpOffThread(
                [path, offset, length]() -> QJsonObject {
                    const Utils::Result<QByteArray> contents
                        = urlishToFilePath(path).fileContents(length, offset);
                    if (!contents)
                        return QJsonObject{{"success", false}, {"error", contents.error()}};
                    return QJsonObject{
                        {"success", true},
                        {"base64", QString::fromLatin1(contents->toBase64())},
                        {"size", double(contents->size())},
                    };
                },
                callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("write_file_bytes")
            .title("Write raw bytes to a file")
            .description(
                "Writes base64-decoded raw bytes to a file, creating or overwriting it - use this "
                "for binary files; for text use set_file_plain_text. This writes directly to disk "
                "and does not route through the editor. Local and remote files are both supported "
                "via Qt Creator urlish paths such as ssh://user@host/path or docker://id/path.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Path of the file (local or remote urlish path)."}})
                    .addProperty(
                        "base64",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Raw bytes to write, base64-encoded."}})
                    .addRequired("path")
                    .addRequired("base64"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "reason",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"ok_disk_write", "empty_path", "write_failed"}}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const QString base64 = p.value("base64").toString();
            runFileOpOffThread(
                [path, base64]() -> QJsonObject {
                    return commands.writeFileContentsBase64(path, base64);
                },
                callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_directory")
            .title("List a directory")
            .description(
                "Lists directory entries via Utils::FilePath, with name, path, type, size, "
                "modification time and executable bit. Optionally filter by glob name patterns and "
                "recurse. Local and remote directories are both supported via Qt Creator urlish "
                "paths such as ssh://user@host/path or docker://id/path; remote access is "
                "transparent.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Path of the directory (local or remote urlish path)."}})
                    .addProperty(
                        "name_filters",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description", "Glob patterns to match, e.g. ['*.txt'] (optional)."}})
                    .addProperty(
                        "recursive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Recurse into subdirectories (optional)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("entries", QJsonObject{{"type", "array"}})
                    .addRequired("entries")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            QStringList nameFilters;
            for (const QJsonValue &v : p.value("name_filters").toArray())
                nameFilters.append(v.toString());
            const bool recursive = p.value("recursive").toBool(false);
            runFileOpOffThread(
                [path, nameFilters, recursive]() -> QJsonObject {
                    const FilePath dir = urlishToFilePath(path);
                    const FileFilter filter(
                        nameFilters,
                        DirFilterFlag::AllEntries | DirFilterFlag::NoDotAndDotDot,
                        recursive ? DirIteratorFlag::Subdirectories
                                  : DirIteratorFlag::NoIteratorFlags);
                    QJsonArray entries;
                    for (const FilePath &entry : dir.dirEntries(filter)) {
                        const FilePathInfo info = entry.filePathInfo();
                        entries.append(QJsonObject{
                            {"name", entry.fileName()},
                            {"path", entry.toUrlishString()},
                            {"isDir", bool(info.fileFlags & FilePathInfo::DirectoryType)},
                            {"size", double(info.fileSize)},
                            {"lastModified", info.lastModified.toString(Qt::ISODate)},
                            {"isExecutable", bool(info.fileFlags & FilePathInfo::ExeOwnerPerm)},
                        });
                    }
                    return QJsonObject{{"entries", entries}};
                },
                callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("file_info")
            .title("Get metadata for a path")
            .description(
                "Returns metadata for a path via Utils::FilePath: existence, type, size, "
                "modification time, executable bit and permissions. Local and remote paths are "
                "both supported via Qt Creator urlish paths such as ssh://user@host/path or "
                "docker://id/path; remote access is transparent.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Path to inspect (local or remote urlish path)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("exists", QJsonObject{{"type", "boolean"}})
                    .addRequired("exists")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            runFileOpOffThread(
                [path]() -> QJsonObject {
                    const FilePath fp = urlishToFilePath(path);
                    const FilePathInfo info = fp.filePathInfo();
                    const bool exists = bool(info.fileFlags & FilePathInfo::ExistsFlag);
                    return QJsonObject{
                        {"exists", exists},
                        {"isFile", bool(info.fileFlags & FilePathInfo::FileType)},
                        {"isDir", bool(info.fileFlags & FilePathInfo::DirectoryType)},
                        {"size", double(info.fileSize)},
                        {"lastModified", info.lastModified.toString(Qt::ISODate)},
                        {"isExecutable", bool(info.fileFlags & FilePathInfo::ExeOwnerPerm)},
                        {"permissions", int(info.fileFlags & FilePathInfo::PermsMask)},
                    };
                },
                callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("make_directory")
            .title("Create a directory")
            .description(
                "Creates a directory and any missing parents via "
                "Utils::FilePath::ensureWritableDir(). Local and remote paths are both supported "
                "via Qt Creator urlish paths such as ssh://user@host/path or docker://id/path.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description",
                             "Path of the directory to create (local or remote urlish path)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            runFileOpOffThread(
                [path]() -> QJsonObject {
                    const Utils::Result<> res = urlishToFilePath(path).ensureWritableDir();
                    if (!res)
                        return QJsonObject{{"success", false}, {"error", res.error()}};
                    return QJsonObject{{"success", true}};
                },
                callback);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("remove_path")
            .title("Remove a file or directory")
            .description(
                "Removes a file, or a directory when 'recursive' is true, via "
                "Utils::FilePath::removeFile() / removeRecursively(). Local and remote paths are "
                "both supported via Qt Creator urlish paths such as ssh://user@host/path or "
                "docker://id/path.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description", "Path to remove (local or remote urlish path)."}})
                    .addProperty(
                        "recursive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Remove directories and their contents (optional)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const bool recursive = p.value("recursive").toBool(false);
            runFileOpOffThread(
                [path, recursive]() -> QJsonObject {
                    const FilePath fp = urlishToFilePath(path);
                    const Utils::Result<> res = recursive ? fp.removeRecursively() : fp.removeFile();
                    if (!res)
                        return QJsonObject{{"success", false}, {"error", res.error()}};
                    return QJsonObject{{"success", true}};
                },
                callback);
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
                        "case_sensitive",
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
            const bool caseSensitive = p.value("case_sensitive").toBool(false);
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
                        "case_sensitive",
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
            const bool caseSensitive = p.value("case_sensitive").toBool(false);
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
                        "case_sensitive",
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
            const bool caseSensitive = p.value("case_sensitive").toBool(false);
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
                        "case_sensitive",
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
            const bool caseSensitive = p.value("case_sensitive").toBool(false);
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
                        "open_files",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("open_files")),
        wrap([](const QJsonObject &) {
            const QStringList files = commands.listOpenFiles();
            QJsonArray arr;
            for (const QString &f : files)
                arr.append(f);
            return QJsonObject{{"open_files", arr}};
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
                        "visible_files",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("visible_files")),
        wrap([](const QJsonObject &) {
            const QStringList files = commands.listVisibleFiles();
            QJsonArray arr;
            for (const QString &f : files)
                arr.append(f);
            return QJsonObject{{"visible_files", arr}};
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
                        "session_name",
                        QJsonObject{
                            {"type", "string"}, {"description", "Name of the session to load"}})
                    .addRequired("session_name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString name = p.value("session_name").toString();
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
                        "working_dir",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Directory in which the command is executed"}}))
            .outputSchema(
                Tool::OutputSchema()
                    .addRequired("exit_code")
                    .addProperty(
                        "exit_code",
                        QJsonObject{{"type", "integer"}, {"description", "Exit code of the command"}})
                    .addProperty(
                        "exit_message",
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
                p["working_dir"].toString(),
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

    ToolRegistry::registerTool(
        Tool{}
            .name("list_plugins")
            .title("List the installed plugins")
            .description("Lists all installed plugins together with their current run state and "
                         "whether they can be loaded at runtime without a restart.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "plugins",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "The installed plugins"}})
                    .addRequired("plugins")),
        wrap([](const QJsonObject &) {
            QJsonArray plugins;
            for (PluginSpec *spec : PluginManager::plugins()) {
                plugins.append(QJsonObject{
                    {"name", spec->name()},
                    {"version", spec->version()},
                    {"state", pluginStateName(spec->state())},
                    {"running", spec->state() == PluginSpec::Running},
                    {"softLoadable", spec->isEffectivelySoftloadable()}});
            }
            return QJsonObject{{"plugins", plugins}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("load_plugin")
            .title("Load a plugin at runtime")
            .description("Soft-loads a plugin, and its soft-loadable dependencies, into the running "
                         "Qt Creator without a restart. Only works for plugins marked as "
                         "soft-loadable; there is no matching unload.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the plugin to load, as reported by "
                                            "list_plugins"}})
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("state", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString name = p.value("name").toString();
            PluginSpec *spec = Utils::findOrDefault(PluginManager::plugins(), [&name](PluginSpec *s) {
                return s->name().compare(name, Qt::CaseInsensitive) == 0;
            });
            if (!spec) {
                return QJsonObject{
                    {"success", false}, {"message", QString("No plugin named \"%1\".").arg(name)}};
            }
            if (spec->state() == PluginSpec::Running)
                return QJsonObject{{"success", true}, {"state", pluginStateName(spec->state())}};
            if (!spec->isEffectivelySoftloadable()) {
                return QJsonObject{
                    {"success", false},
                    {"state", pluginStateName(spec->state())},
                    {"message", QString("Plugin \"%1\" is not soft-loadable.").arg(name)}};
            }
            // A plugin is only loaded up to the Loaded state if it is effectively enabled, so
            // enable it first. Unlike the About Plugins dialog, this deliberately does not
            // persist the change via PluginManager::writeSettings(): loading a plugin from an
            // MCP client exercises it in the running session without permanently rewriting the
            // user's plugin configuration. Use save_plugin_settings to make it stick.
            spec->setEnabledBySettings(true);
            PluginManager::loadPluginsAtRuntime({spec});
            return QJsonObject{
                {"success", spec->state() == PluginSpec::Running},
                {"state", pluginStateName(spec->state())}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("save_plugin_settings")
            .title("Persist the enabled plugins")
            .description("Writes the current enabled/disabled state of all plugins to disk so it "
                         "survives a restart. Use after load_plugin to make a runtime-loaded "
                         "plugin load again on the next start.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) {
            PluginManager::writeSettings();
            return QJsonObject{{"success", true}};
        }));

    // Generic access to aspect-backed settings pages (the Preferences dialog).
    const auto findOptionsPage = [](const QString &id) -> Core::IOptionsPage * {
        for (Core::IOptionsPage *p : Core::IOptionsPage::allOptionsPages()) {
            if (p->id().toString() == id)
                return p;
        }
        return nullptr;
    };

    // Resolve a settings "page" id to a live AspectContainer from three sources:
    // an aspect-backed IOptionsPage; the "activeRunConfiguration" anchor (the
    // active project's active run config, whose aspects include e.g. "Run in
    // terminal" and are not reachable as an IOptionsPage); or, in test builds,
    // any registered AspectContainer addressed by its id().
    struct ResolvedSettings
    {
        AspectContainer *container = nullptr;
        QString name;
        QString reason;
    };
    const auto resolveContainer = [findOptionsPage](const QString &id) -> ResolvedSettings {
        if (Core::IOptionsPage *page = findOptionsPage(id)) {
            const std::optional<AspectContainer *> ac = page->aspects();
            if (ac && *ac)
                return {*ac, page->displayName(), "ok"};
            return {nullptr, {}, "not_aspect_backed"};
        }
        if (id == "activeRunConfiguration") {
            using namespace ProjectExplorer;
            if (Project *pr = ProjectManager::startupProject())
                if (Target *t = pr->activeTarget())
                    if (RunConfiguration *rc = t->activeRunConfiguration())
                        return {rc, QString("Active run configuration"), "ok"};
            return {nullptr, {}, "no_active_run_config"};
        }
#ifdef WITH_TESTS
        for (AspectContainer *c : AspectContainer::registeredContainers()) {
            if (!c->id().toString().isEmpty() && c->id().toString() == id)
                return {c, id, "ok"};
        }
#endif
        return {nullptr, {}, "page_not_found"};
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("list_settings_pages")
            .title("List aspect-backed settings pages")
            .description(
                "Lists Qt Creator preference pages whose settings are exposed as aspects, "
                "so they can be read with get_settings and changed with set_setting. Pages "
                "with hand-rolled widgets (no aspect container) are omitted. Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}.addProperty("pages", QJsonObject{{"type", "array"}})),
        wrap([](const QJsonObject &) {
            QJsonArray pages;
            for (Core::IOptionsPage *p : Core::IOptionsPage::allOptionsPages()) {
                const std::optional<AspectContainer *> ac = p->aspects();
                if (!ac || !*ac)
                    continue;
                pages.append(QJsonObject{
                    {"id", p->id().toString()},
                    {"displayName", p->displayName()},
                    {"category", p->category().toString()},
                    {"settingCount", int((*ac)->aspects().size())}});
            }
            // Per-instance anchor: the active run configuration's aspects (e.g.
            // "Run in terminal"), which are an AspectContainer but not an
            // IOptionsPage. Addressable as page id "activeRunConfiguration".
            {
                using namespace ProjectExplorer;
                if (Project *pr = ProjectManager::startupProject())
                    if (Target *t = pr->activeTarget())
                        if (RunConfiguration *rc = t->activeRunConfiguration())
                            pages.append(QJsonObject{
                                {"id", "activeRunConfiguration"},
                                {"displayName", "Active run configuration"},
                                {"category", "ProjectExplorer"},
                                {"settingCount", int(rc->aspects().size())}});
            }
            return QJsonObject{{"pages", pages}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_settings")
            .title("Read the settings of a settings page")
            .description(
                "Returns the individual settings (aspects) of a preference page: key, "
                "label, current value and default value. Use the page id from "
                "list_settings_pages. Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "page",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Settings page id (see list_settings_pages)."}})
                    .addRequired("page"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("page", QJsonObject{{"type", "string"}})
                    .addProperty("settings", QJsonObject{{"type", "array"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})),
        wrap([resolveContainer](const QJsonObject &p) -> QJsonObject {
            const ResolvedSettings r = resolveContainer(p.value("page").toString());
            if (!r.container)
                return {{"reason", r.reason},
                        {"message", "No aspect-backed settings for that page id (see "
                                    "list_settings_pages; try \"activeRunConfiguration\")."}};
            QJsonArray settings;
            for (BaseAspect *a : r.container->aspects()) {
                if (a->settingsKey().isEmpty())
                    continue;
                settings.append(QJsonObject{
                    {"key", stringFromKey(a->settingsKey())},
                    {"id", a->id().toString()},
                    {"label", a->labelText()},
                    {"value", QJsonValue::fromVariant(a->variantValue())},
                    {"defaultValue", QJsonValue::fromVariant(a->defaultVariantValue())}});
            }
            return {{"page", r.name},
                    {"settings", settings},
                    {"reason", "ok"},
                    {"message", "ok"}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("set_setting")
            .title("Change a setting on a settings page")
            .description(
                "Sets a single aspect-based setting to a new value and persists it. The "
                "change takes effect immediately (the aspect emits its change signal), so "
                "this is the programmatic equivalent of toggling the setting in the "
                "Preferences dialog. Identify the setting by its 'key' (settingsKey from "
                "get_settings); the value is coerced to the setting's current type.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "page",
                        QJsonObject{{"type", "string"}, {"description", "Settings page id."}})
                    .addProperty(
                        "key",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "settingsKey of the setting (see get_settings)."}})
                    .addProperty(
                        "value",
                        QJsonObject{
                            {"description",
                             "New value (bool/number/string), coerced to the setting's type."}})
                    .addRequired("page")
                    .addRequired("key")
                    .addRequired("value"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("applied", QJsonObject{{"type", "boolean"}})
                    .addProperty("key", QJsonObject{{"type", "string"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})),
        wrap([resolveContainer](const QJsonObject &p) -> QJsonObject {
            const ResolvedSettings r = resolveContainer(p.value("page").toString());
            if (!r.container)
                return {{"applied", false},
                        {"reason", r.reason},
                        {"message", "No aspect-backed settings for that page id (see "
                                    "list_settings_pages; try \"activeRunConfiguration\")."}};
            const QString key = p.value("key").toString();
            BaseAspect *target = nullptr;
            for (BaseAspect *a : r.container->aspects()) {
                if (stringFromKey(a->settingsKey()) == key) {
                    target = a;
                    break;
                }
            }
            if (!target)
                return {{"applied", false},
                        {"reason", "key_not_found"},
                        {"message", QString("No setting with key '%1' on this page.").arg(key)}};

            QVariant newValue = p.value("value").toVariant();
            const QVariant cur = target->variantValue();
            if (cur.isValid() && newValue.metaType() != cur.metaType()) {
                if (!newValue.convert(cur.metaType()))
                    return {{"applied", false},
                            {"reason", "type_mismatch"},
                            {"message",
                             QString("Value could not be coerced to the setting's type (%1).")
                                 .arg(QString::fromUtf8(cur.typeName()))}};
            }
            target->setVariantValue(newValue);
            r.container->writeSettings();
            return {{"applied", true},
                    {"key", key},
                    {"value", QJsonValue::fromVariant(target->variantValue())},
                    {"reason", "ok"},
                    {"message", QString("Set '%1' on page '%2'.").arg(key, r.name)}};
        }));

    // The four query fields shared by every widget-addressing tool. A tool's
    // effective query is the conjunction of the fields the caller provides.
    const auto addWidgetQueryProps = [](Tool::InputSchema schema) {
        return schema
            .addProperty(
                "object_name",
                QJsonObject{
                    {"type", "string"},
                    {"description", "Exact objectName(). The most robust selector: stable "
                                    "across translation and layout changes."}})
            .addProperty(
                "text",
                QJsonObject{
                    {"type", "string"},
                    {"description", "Visible text (button/label/combo/groupbox), or, for an "
                                    "input like a line edit, its buddy label's text. Matched "
                                    "exactly after trimming and stripping '&' accelerators. "
                                    "Readable but translation-sensitive."}})
            .addProperty(
                "class_name",
                QJsonObject{
                    {"type", "string"},
                    {"description", "Meta-object class name, e.g. \"QPushButton\". Matches the "
                                    "exact class or any subclass (QAbstractButton matches "
                                    "QPushButton)."}})
            .addProperty(
                "window_title",
                QJsonObject{
                    {"type", "string"},
                    {"description", "Restrict to widgets whose top-level window title contains "
                                    "this (case-insensitive), e.g. to disambiguate an OK button "
                                    "by its dialog."}})
            .addProperty(
                "include_invisible",
                QJsonObject{
                    {"type", "boolean"},
                    {"description", "Also match hidden widgets (default false)."}});
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("find_widgets")
            .title("Find widgets in the running UI")
            .description(
                "Resolves a semantic widget query against the live Qt Creator UI by walking "
                "all widgets (including dialogs and popups). Returns every match with its "
                "class, objectName, visible text, enabled/visible state, geometry in root "
                "coordinates and top-level window id. This is the addressing layer for "
                "click_widget / type_text / select_combo_item: use it to discover selectors "
                "and to check that a query is unambiguous before acting on it. Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(addWidgetQueryProps(Tool::InputSchema{}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("count", QJsonObject{{"type", "integer"}})
                    .addProperty("widgets", QJsonObject{{"type", "array"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("count")
                    .addRequired("widgets")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const WidgetQuery q = widgetQueryFromJson(p);
            if (widgetQueryIsEmpty(q)) {
                return {{"count", 0},
                        {"widgets", QJsonArray{}},
                        {"error", "Empty widget query; specify at least one of object_name, "
                                  "text, class_name, window_title."}};
            }
            QJsonArray arr;
            for (QWidget *w : resolveWidgets(q))
                arr.append(describeWidget(w));
            return {{"count", arr.size()}, {"widgets", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("click_widget")
            .title("Click a widget")
            .description(
                "Clicks the single widget matching the query. A button is clicked via its own "
                "click() slot; any other widget receives a synthetic left click at its centre. "
                "The query must resolve to exactly one visible widget - zero or multiple matches "
                "are an error, so ambiguity never silently picks a widget.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(addWidgetQueryProps(Tool::InputSchema{})),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const WidgetQuery q = widgetQueryFromJson(params.argumentsAsObject());
            const Utils::Result<QWidget *> w = resolveSingleWidget(q);
            if (!w)
                return ResultError(w.error());
            if (!(*w)->isEnabled())
                return ResultError(QString("Widget is disabled: %1.").arg(describeWidgetShort(*w)));
            clickWidget(*w);
            return CallToolResult{}.isError(false).structuredContent(describeWidget(*w));
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("select_combo_item")
            .title("Select an item in a combo box")
            .description(
                "Selects an item by its text in the single QComboBox matching the query, via "
                "setCurrentIndex(). This avoids the pitfall that pressing Return on a focused "
                "combo box opens its dropdown instead of choosing. The query must resolve to "
                "exactly one QComboBox.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                addWidgetQueryProps(Tool::InputSchema{})
                    .addProperty(
                        "item",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Exact text of the item to select."}})
                    .addRequired("item")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const Utils::Result<QWidget *> w = resolveSingleWidget(widgetQueryFromJson(p));
            if (!w)
                return ResultError(w.error());
            auto combo = qobject_cast<QComboBox *>(*w);
            if (!combo) {
                return ResultError(
                    QString("Widget is not a QComboBox: %1.").arg(describeWidgetShort(*w)));
            }
            const QString item = p.value("item").toString();
            const int index = combo->findText(item);
            if (index < 0) {
                QStringList items;
                for (int i = 0; i < combo->count(); ++i)
                    items << QString("\"%1\"").arg(combo->itemText(i));
                return ResultError(QString("Combo box has no item \"%1\". Items: [%2].")
                                       .arg(item, items.join(", ")));
            }
            combo->setCurrentIndex(index);
            return CallToolResult{}.isError(false).structuredContent(describeWidget(combo));
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("type_text")
            .title("Type text into a widget")
            .description(
                "Types text by delivering key events, so widgets that react to typing (line "
                "edits, text editors) update as if the user typed. If widget query fields are "
                "given they select and focus the target (which must resolve to exactly one "
                "widget); otherwise the current focus widget receives the input.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                addWidgetQueryProps(Tool::InputSchema{})
                    .addProperty(
                        "input",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "The text to type."}})
                    .addRequired("input")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const QString input = p.value("input").toString();
            const WidgetQuery q = widgetQueryFromJson(p);
            const Utils::Result<QWidget *> t = resolveKeyTarget(q);
            if (!t)
                return ResultError(t.error());
            QWidget *target = *t;
            if (QWidget *window = target->window())
                window->activateWindow();
            target->setFocus(Qt::OtherFocusReason);
            typeText(target, input);
            return CallToolResult{}.isError(false).structuredContent(describeWidget(target));
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("press_keys")
            .title("Press a key or keyboard shortcut")
            .description(
                "Sends a key chord parsed with QKeySequence (e.g. \"Ctrl+K\", \"Escape\", "
                "\"Return\", \"Ctrl+Shift+P\", \"Down\") to the focused widget, or to the single "
                "widget matching the query. Use it for keys a widget handles directly (Return, "
                "Escape, Tab, arrows) and to demonstrate a shortcut being pressed. To reliably "
                "trigger an action's effect regardless of focus, prefer call_action - a synthetic "
                "key event does not always drive application-wide shortcuts.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                addWidgetQueryProps(Tool::InputSchema{})
                    .addProperty(
                        "keys",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Key sequence, e.g. \"Ctrl+K\" or \"Escape\"."}})
                    .addRequired("keys")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const QString keys = p.value("keys").toString();
            if (keys.isEmpty())
                return ResultError(QString("No key sequence given."));
            const QKeySequence seq(keys, QKeySequence::PortableText);
            if (seq.isEmpty())
                return ResultError(QString("Could not parse key sequence \"%1\".").arg(keys));
            const QKeyCombination combo = seq[0];
            const Qt::Key key = combo.key();
            const Qt::KeyboardModifiers mods = combo.keyboardModifiers();

            const WidgetQuery q = widgetQueryFromJson(p);
            const Utils::Result<QWidget *> t = resolveKeyTarget(q);
            if (!t)
                return ResultError(t.error());
            QWidget *target = *t;
            if (QWidget *window = target->window())
                window->activateWindow();
            target->setFocus(Qt::OtherFocusReason);

            // Printable, unmodified keys carry their text so line edits insert them;
            // chords and named keys deliver an empty text and rely on key+modifiers.
            QString text;
            if (!(mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
                && key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde) {
                QChar ch{char16_t(key)};
                if (!(mods & Qt::ShiftModifier))
                    ch = ch.toLower();
                text = QString(ch);
            }
            QKeyEvent press(QEvent::KeyPress, key, mods, text);
            QKeyEvent release(QEvent::KeyRelease, key, mods, text);
            QApplication::sendEvent(target, &press);
            QApplication::sendEvent(target, &release);
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"keys", seq.toString(QKeySequence::PortableText)},
                            {"target", describeWidget(target)}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("find_menu_item")
            .title("Locate a menu bar entry or open-menu item")
            .description(
                "Returns the geometry, in root coordinates, of a menu bar entry (e.g. "
                "\"Help\") or of an item in a currently-open menu (e.g. \"About Qt Creator\"), "
                "matched by visible text ('&' and a trailing \"...\" are ignored). Menu items "
                "are QActions, not addressable widgets, so this is how a scenario drives menus "
                "with the cursor. Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "title",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Visible text of the menu or item."}})
                    .addRequired("title"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("found", QJsonObject{{"type", "boolean"}})
                    .addProperty("source", QJsonObject{{"type", "string"}})
                    .addRequired("found")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const auto norm = [](QString t) {
                t.remove('&');
                while (t.endsWith('.'))
                    t.chop(1);
                return t.trimmed();
            };
            const QString title = norm(p.value("title").toString());
            if (title.isEmpty())
                return {{"found", false}};
            const auto entry = [&](QWidget *w, QAction *a, const QRect &r, const QString &src) {
                const QPoint tl = w->mapToGlobal(r.topLeft());
                return QJsonObject{{"found", true},
                                   {"source", src},
                                   {"text", norm(a->text())},
                                   {"x", tl.x()},
                                   {"y", tl.y()},
                                   {"width", r.width()},
                                   {"height", r.height()}};
            };
            for (QWidget *w : QApplication::allWidgets()) {
                auto menuBar = qobject_cast<QMenuBar *>(w);
                if (!menuBar || !menuBar->isVisible())
                    continue;
                for (QAction *a : menuBar->actions()) {
                    if (!a->isSeparator()
                        && norm(a->text()).compare(title, Qt::CaseInsensitive) == 0) {
                        return entry(menuBar, a, menuBar->actionGeometry(a), "menubar");
                    }
                }
            }
            for (QWidget *w : QApplication::allWidgets()) {
                auto menu = qobject_cast<QMenu *>(w);
                if (!menu || !menu->isVisible())
                    continue;
                for (QAction *a : menu->actions()) {
                    if (!a->isSeparator()
                        && norm(a->text()).compare(title, Qt::CaseInsensitive) == 0) {
                        return entry(menu, a, menu->actionGeometry(a), "menu");
                    }
                }
            }
            return {{"found", false}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("widget_exists")
            .title("Check whether a widget exists")
            .description(
                "Reports whether the widget query matches any live widget, and how many. Use it "
                "as an assertion (e.g. \"the preview opened\") without failing on zero matches "
                "the way click_widget does. Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(addWidgetQueryProps(Tool::InputSchema{}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("exists", QJsonObject{{"type", "boolean"}})
                    .addProperty("count", QJsonObject{{"type", "integer"}})
                    .addRequired("exists")
                    .addRequired("count")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const WidgetQuery q = widgetQueryFromJson(p);
            if (widgetQueryIsEmpty(q)) {
                return {{"exists", false},
                        {"count", 0},
                        {"error", "Empty widget query; specify at least one of object_name, "
                                  "text, class_name, window_title."}};
            }
            const QList<QWidget *> matches = resolveWidgets(q);
            QJsonObject result{{"exists", !matches.isEmpty()}, {"count", matches.size()}};
            if (!matches.isEmpty())
                result["first"] = describeWidget(matches.first());
            return result;
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("list_windows")
            .title("List top-level windows")
            .description(
                "Lists the top-level windows of the running Qt Creator - the main window and any "
                "open dialogs or popups - each with its class, objectName, title, geometry, "
                "window id, and whether it is active or modal. Use it to see which dialog is up "
                "before addressing widgets inside it. Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}.addProperty(
                    "include_invisible",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Also list hidden windows (default false)."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("count", QJsonObject{{"type", "integer"}})
                    .addProperty("windows", QJsonObject{{"type", "array"}})
                    .addRequired("count")
                    .addRequired("windows")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const bool includeInvisible = p.value("include_invisible").toBool(false);
            QJsonArray windows;
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (!w->isWindow())
                    continue;
                if (!includeInvisible && !w->isVisible())
                    continue;
                QJsonObject o = describeWidget(w);
                o["active"] = w->isActiveWindow();
                o["modal"] = w->isModal();
                windows.append(o);
            }
            return {{"count", windows.size()}, {"windows", windows}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("read_pane")
            .title("Read the text of an output pane")
            .description(
                "Returns the plain text of an output pane (e.g. \"Application Output\", "
                "\"General Messages\", \"Compile Output\"), identified by its display name. "
                "This is the text the user sees in the pane, distinct from get_application_output "
                "which returns Qt Creator's own log stream. Call without a name (or with an "
                "unknown one) to get the list of available panes. Panes that are not plain-text "
                "(e.g. Issues) report 'pane_has_no_text_output'. Read-only.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Display name of the pane (see available_panes)."}})
                    .addProperty(
                        "max_lines",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Return only the last N lines (optional)."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("text", QJsonObject{{"type", "string"}})
                    .addProperty("pane", QJsonObject{{"type", "string"}})
                    .addProperty("available_panes", QJsonObject{{"type", "array"}})
                    .addRequired("reason")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const QString name = p.value("name").toString();
            const int maxLines = p.value("max_lines").toInt(0);
            QJsonArray available;
            Core::IOutputPane *match = nullptr;
            for (QObject *object : ExtensionSystem::PluginManager::allObjects()) {
                auto pane = qobject_cast<Core::IOutputPane *>(object);
                if (!pane)
                    continue;
                available.append(pane->displayName());
                if (!name.isEmpty() && pane->displayName().compare(name, Qt::CaseInsensitive) == 0)
                    match = pane;
            }
            if (!match) {
                return {{"reason", name.isEmpty() ? "no_name_given" : "pane_not_found"},
                        {"available_panes", available},
                        {"message", "Pick one of available_panes as 'name'."}};
            }
            const QList<Core::OutputWindow *> outputWindows = match->outputWindows();
            if (outputWindows.isEmpty()) {
                return {{"reason", "pane_has_no_text_output"},
                        {"pane", match->displayName()},
                        {"available_panes", available},
                        {"message", "This pane is not a plain-text output pane."}};
            }
            QStringList parts;
            for (Core::OutputWindow *window : outputWindows)
                parts << window->toPlainText();
            return {{"reason", "ok"},
                    {"pane", match->displayName()},
                    {"text", lastLines(parts.join('\n'), maxLines)}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("screenshot")
            .title("Capture a window as a PNG")
            .description(
                "Captures a window and returns it as a PNG. If widget query fields are given, "
                "the target's top-level window is captured (e.g. window_title of a dialog); "
                "otherwise the active window, falling back to the main window. Rendering is done "
                "in-process via QWidget::grab(), so the image is deterministic and never blank - "
                "no compositor or retry needed, unlike an external screen grab. Pass 'path' to "
                "also save the PNG to disk; the base64 is embedded in the result only when no "
                "path is given (or embed=true).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                addWidgetQueryProps(Tool::InputSchema{})
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Optional file path to save the PNG to."}})
                    .addProperty(
                        "embed",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Embed base64 PNG in the result (default: true unless "
                                            "a path is given)."}})),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const WidgetQuery q = widgetQueryFromJson(p);
            QWidget *target = nullptr;
            if (!widgetQueryIsEmpty(q)) {
                // The query names a window, not a single widget, so collapse
                // the matches to their distinct top-level windows: capturing
                // is unambiguous as long as they all live in the same window
                // (e.g. window_title matches every widget in a dialog).
                QWidgetList windows;
                for (QWidget *w : resolveWidgets(q)) {
                    if (QWidget *win = w->window(); win && !windows.contains(win))
                        windows.append(win);
                }
                if (windows.isEmpty())
                    return ResultError(QString("No window matched the query."));
                if (windows.size() > 1) {
                    return ResultError(QString("Query spans %1 windows; narrow it with "
                                               "window_title.").arg(windows.size()));
                }
                target = windows.first();
            } else {
                target = QApplication::activeWindow();
                if (!target)
                    target = Core::ICore::mainWindow();
            }
            if (!target)
                return ResultError(QString("No window to capture."));
            const QPixmap pixmap = target->grab();
            if (pixmap.isNull())
                return ResultError(QString("Captured image is null."));
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            pixmap.save(&buffer, "PNG");

            const QString path = p.value("path").toString();
            const bool embed = p.value("embed").toBool(path.isEmpty());
            QJsonObject out{
                {"width", pixmap.width()},
                {"height", pixmap.height()},
                {"window_title", target->windowTitle()}};
            if (embed)
                out["base64_png"] = QString::fromLatin1(bytes.toBase64());
            if (!path.isEmpty()) {
                const Utils::Result<qint64> written = urlishToFilePath(path).writeFileContents(bytes);
                out["saved"] = written.has_value();
                out["path"] = path;
                if (!written)
                    out["error"] = written.error();
            }
            return CallToolResult{}.isError(false).structuredContent(out);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("move_cursor")
            .title("Move the mouse cursor")
            .description(
                "Warps the real mouse pointer to a root coordinate via QCursor::setPos, so a "
                "screen recording shows the cursor. By default it glides over a few steps; pass "
                "steps=1 to jump. This only moves the pointer - it does not click.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("x", QJsonObject{{"type", "integer"}})
                    .addProperty("y", QJsonObject{{"type", "integer"}})
                    .addProperty(
                        "steps",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Interpolation steps for the glide (default 20)."}})
                    .addRequired("x")
                    .addRequired("y")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const QPoint target(p.value("x").toInt(), p.value("y").toInt());
            const int steps = qMax(1, p.value("steps").toInt(20));
            const QPoint start = QCursor::pos();
            for (int i = 1; i <= steps; ++i) {
                QCursor::setPos(start + (target - start) * i / steps);
                if (steps > 1 && i < steps)
                    QThread::msleep(12);
            }
            return {{"x", target.x()}, {"y", target.y()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("activate_menu_item")
            .title("Open or trigger a menu item")
            .description(
                "Finds a menu bar entry or an item in a currently-open menu by visible text and "
                "activates it via the menu API: a submenu is shown (QMenu::popup) so a scenario "
                "can navigate into it, and a leaf item is triggered. The trigger is posted "
                "asynchronously, so this does not block even when it opens a modal dialog. Pair "
                "with find_menu_item + move_cursor to drive a menu with the cursor.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "title",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Visible text of the menu or item."}})
                    .addRequired("title"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("opened", QJsonObject{{"type", "boolean"}})
                    .addProperty("triggered", QJsonObject{{"type", "boolean"}})),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const auto norm = [](QString t) {
                t.remove('&');
                while (t.endsWith('.'))
                    t.chop(1);
                return t.trimmed();
            };
            const QString title = norm(params.argumentsAsObject().value("title").toString());
            if (title.isEmpty())
                return ResultError(QString("No menu title given."));

            QAction *action = nullptr;
            QWidget *owner = nullptr;
            QRect rect;
            for (QWidget *w : QApplication::allWidgets()) {
                auto menuBar = qobject_cast<QMenuBar *>(w);
                if (!menuBar || !menuBar->isVisible())
                    continue;
                for (QAction *a : menuBar->actions()) {
                    if (!a->isSeparator() && norm(a->text()).compare(title, Qt::CaseInsensitive) == 0) {
                        action = a;
                        owner = menuBar;
                        rect = menuBar->actionGeometry(a);
                    }
                }
            }
            if (!action) {
                for (QWidget *w : QApplication::allWidgets()) {
                    auto menu = qobject_cast<QMenu *>(w);
                    if (!menu || !menu->isVisible())
                        continue;
                    for (QAction *a : menu->actions()) {
                        if (!a->isSeparator()
                            && norm(a->text()).compare(title, Qt::CaseInsensitive) == 0) {
                            action = a;
                            owner = menu;
                            rect = menu->actionGeometry(a);
                        }
                    }
                }
            }
            if (!action)
                return ResultError(QString("No menu item \"%1\".").arg(title));

            if (QMenu *submenu = action->menu()) {
                // A menu bar entry drops its menu below it; a submenu inside an
                // open menu opens to the right.
                const bool inBar = qobject_cast<QMenuBar *>(owner) != nullptr;
                const QPoint pos = owner->mapToGlobal(inBar ? rect.bottomLeft() : rect.topRight());
                submenu->popup(pos);
                return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"opened", true}});
            }
            // Leaf: trigger asynchronously so a modal dialog does not block this
            // call, and close any open menu so it does not linger on screen.
            QMetaObject::invokeMethod(action, "trigger", Qt::QueuedConnection);
            for (QWidget *w : QApplication::allWidgets()) {
                if (auto menu = qobject_cast<QMenu *>(w); menu && menu->isVisible())
                    menu->hide();
            }
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"triggered", true}});
        });
}

} // namespace Mcp::Internal
