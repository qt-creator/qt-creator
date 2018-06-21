/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangbatchfileprocessor.h"

#include "clangautomationutils.h"

#include <clangcodemodel/clangeditordocumentprocessor.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cpptoolstestcase.h>
#include <cpptools/modelmanagertesthelper.h>
#include <cpptools/projectinfo.h>
#include <projectexplorer/projectexplorer.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/executeondestruction.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSharedPointer>
#include <QString>
#include <QThread>

using namespace ClangBackEnd;
using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace ProjectExplorer;

static Q_LOGGING_CATEGORY(debug, "qtc.clangcodemodel.batch");

static int timeOutFromEnvironmentVariable()
{
    const QByteArray timeoutAsByteArray = qgetenv("QTC_CLANG_BATCH_TIMEOUT");

    bool isConversionOk = false;
    const int intervalAsInt = timeoutAsByteArray.toInt(&isConversionOk);
    if (!isConversionOk) {
        qCDebug(debug, "Environment variable QTC_CLANG_BATCH_TIMEOUT is not set, assuming 30000.");
        return 30000;
    }

    return intervalAsInt;
}

static int timeOutInMs()
{
    static int timeOut = timeOutFromEnvironmentVariable();
    return timeOut;
}

namespace {

class BatchFileLineTokenizer
{
public:
    BatchFileLineTokenizer(const QString &line);

    QString nextToken();

private:
    const QChar *advanceToTokenBegin();
    const QChar *advanceToTokenEnd();

    bool atEnd() const;
    bool atWhiteSpace() const;
    bool atQuotationMark() const;

private:
    bool m_isWithinQuotation = false;
    QString m_line;
    const QChar *m_currentChar;
};

BatchFileLineTokenizer::BatchFileLineTokenizer(const QString &line)
    : m_line(line)
    , m_currentChar(m_line.unicode())
{
}

QString BatchFileLineTokenizer::nextToken()
{
    if (const QChar *tokenBegin = advanceToTokenBegin()) {
        if (const QChar *tokenEnd = advanceToTokenEnd()) {
            const int length = tokenEnd - tokenBegin;
            return QString(tokenBegin, length);
        }
    }

    return QString();
}

const QChar *BatchFileLineTokenizer::advanceToTokenBegin()
{
    m_isWithinQuotation = false;

    forever {
        if (atEnd())
            return 0;

        if (atQuotationMark()) {
            m_isWithinQuotation = true;
            ++m_currentChar;
            return m_currentChar;
        }

        if (!atWhiteSpace())
            return m_currentChar;

        ++m_currentChar;
    }
}

const QChar *BatchFileLineTokenizer::advanceToTokenEnd()
{
    forever {
        if (m_isWithinQuotation) {
            if (atEnd()) {
                qWarning("ClangBatchFileProcessor: error: unfinished quotation.");
                return 0;
            }

            if (atQuotationMark())
                return m_currentChar++;

        } else if (atWhiteSpace() || atEnd()) {
            return m_currentChar;
        }

        ++m_currentChar;
    }
}

bool BatchFileLineTokenizer::atEnd() const
{
    return *m_currentChar == QLatin1Char('\0');
}

bool BatchFileLineTokenizer::atWhiteSpace() const
{
    return *m_currentChar == ' '
        || *m_currentChar == '\t'
        || *m_currentChar == '\n';
}

bool BatchFileLineTokenizer::atQuotationMark() const
{
    return *m_currentChar == '"';
}

struct CommandContext {
    QString filePath;
    int lineNumber = -1;
};

class Command
{
public:
    using Ptr = QSharedPointer<Command>;

public:
    Command(const CommandContext &context) : m_commandContext(context) {}
    virtual ~Command() {}

    const CommandContext &context() const { return m_commandContext; }
    virtual bool run() { return true; }

private:
    const CommandContext m_commandContext;
};

class OpenProjectCommand : public Command
{
public:
    OpenProjectCommand(const CommandContext &context,
                       const QString &projectFilePath);

    bool run() override;

    static Command::Ptr parse(BatchFileLineTokenizer &arguments,
                              const CommandContext &context);

private:
    QString m_projectFilePath;
};

OpenProjectCommand::OpenProjectCommand(const CommandContext &context,
                                       const QString &projectFilePath)
    : Command(context)
    , m_projectFilePath(projectFilePath)
{
}

bool OpenProjectCommand::run()
{
    qCDebug(debug) << "line" << context().lineNumber << "OpenProjectCommand" << m_projectFilePath;

    const ProjectExplorerPlugin::OpenProjectResult openProjectSucceeded
            = ProjectExplorerPlugin::openProject(m_projectFilePath);
    QTC_ASSERT(openProjectSucceeded, return false);

    Project *project = openProjectSucceeded.project();
    project->configureAsExampleProject({});

    return CppTools::Tests::TestCase::waitUntilCppModelManagerIsAwareOf(project, timeOutInMs());
}

Command::Ptr OpenProjectCommand::parse(BatchFileLineTokenizer &arguments,
                                       const CommandContext &context)
{
    const QString projectFilePath = arguments.nextToken();
    if (projectFilePath.isEmpty()) {
        qWarning("%s:%d: error: No project file path given.",
                  qPrintable(context.filePath),
                  context.lineNumber);
        return Command::Ptr();
    }

    const QString absoluteProjectFilePath = QFileInfo(projectFilePath).absoluteFilePath();

    return Command::Ptr(new OpenProjectCommand(context, absoluteProjectFilePath));
}

class OpenDocumentCommand : public Command
{
public:
    OpenDocumentCommand(const CommandContext &context,
                        const QString &documentFilePath);

    bool run() override;

    static Command::Ptr parse(BatchFileLineTokenizer &arguments, const CommandContext &context);

private:
    QString m_documentFilePath;
};

OpenDocumentCommand::OpenDocumentCommand(const CommandContext &context,
                                         const QString &documentFilePath)
    : Command(context)
    , m_documentFilePath(documentFilePath)
{
}

class WaitForUpdatedCodeWarnings : public QObject
{
    Q_OBJECT

public:
    WaitForUpdatedCodeWarnings(ClangEditorDocumentProcessor *processor);

    bool wait(int timeOutInMs) const;

private:
    void onCodeWarningsUpdated() { m_gotResults = true; }

private:

    bool m_gotResults = false;
};

WaitForUpdatedCodeWarnings::WaitForUpdatedCodeWarnings(ClangEditorDocumentProcessor *processor)
{
    connect(processor,
            &ClangEditorDocumentProcessor::codeWarningsUpdated,
            this, &WaitForUpdatedCodeWarnings::onCodeWarningsUpdated);
}

bool WaitForUpdatedCodeWarnings::wait(int timeOutInMs) const
{
    QTime time;
    time.start();

    forever {
        if (time.elapsed() > timeOutInMs) {
            qWarning("WaitForUpdatedCodeWarnings: timeout of %d ms reached.", timeOutInMs);
            return false;
        }

        if (m_gotResults)
            return true;

        QCoreApplication::processEvents();
        QThread::msleep(20);
    }
}

bool OpenDocumentCommand::run()
{
    qCDebug(debug) << "line" << context().lineNumber << "OpenDocumentCommand" << m_documentFilePath;

    const bool openEditorSucceeded = Core::EditorManager::openEditor(m_documentFilePath);
    QTC_ASSERT(openEditorSucceeded, return false);

    auto *processor = ClangEditorDocumentProcessor::get(m_documentFilePath);
    QTC_ASSERT(processor, return false);

    WaitForUpdatedCodeWarnings waiter(processor);
    return waiter.wait(timeOutInMs());
}

Command::Ptr OpenDocumentCommand::parse(BatchFileLineTokenizer &arguments,
                                        const CommandContext &context)
{
    const QString documentFilePath = arguments.nextToken();
    if (documentFilePath.isEmpty()) {
        qWarning("%s:%d: error: No document file path given.",
                  qPrintable(context.filePath),
                  context.lineNumber);
        return Command::Ptr();
    }

    const QString absoluteDocumentFilePath = QFileInfo(documentFilePath).absoluteFilePath();

    return Command::Ptr(new OpenDocumentCommand(context, absoluteDocumentFilePath));
}

class CloseAllDocuments : public Command
{
public:
    CloseAllDocuments(const CommandContext &context);

    bool run() override;

    static Command::Ptr parse(BatchFileLineTokenizer &arguments, const CommandContext &context);
};

CloseAllDocuments::CloseAllDocuments(const CommandContext &context)
    : Command(context)
{
}

bool CloseAllDocuments::run()
{
    qCDebug(debug) << "line" << context().lineNumber << "CloseAllDocuments";

    return Core::EditorManager::closeAllEditors(/*askAboutModifiedEditors=*/ false);
}

Command::Ptr CloseAllDocuments::parse(BatchFileLineTokenizer &arguments,
                                      const CommandContext &context)
{
    const QString argument = arguments.nextToken();
    if (!argument.isEmpty()) {
        qWarning("%s:%d: error: Unexpected argument.",
                 qPrintable(context.filePath),
                 context.lineNumber);
        return Command::Ptr();
    }

    return Command::Ptr(new CloseAllDocuments(context));
}

class InsertTextCommand : public Command
{
public:
    // line and column are 1-based
    InsertTextCommand(const CommandContext &context, const QString &text);

    bool run() override;

    static Command::Ptr parse(BatchFileLineTokenizer &arguments,
                              const CommandContext &context);

private:
    const QString m_textToInsert;
};

InsertTextCommand::InsertTextCommand(const CommandContext &context, const QString &text)
    : Command(context)
    , m_textToInsert(text)
{
}

TextEditor::BaseTextEditor *currentTextEditor()
{
    return qobject_cast<TextEditor::BaseTextEditor*>(Core::EditorManager::currentEditor());
}

bool InsertTextCommand::run()
{
    qCDebug(debug) << "line" << context().lineNumber << "InsertTextCommand" << m_textToInsert;

    TextEditor::BaseTextEditor *editor = currentTextEditor();
    QTC_ASSERT(editor, return false);
    const QString documentFilePath = editor->document()->filePath().toString();
    auto *processor = ClangEditorDocumentProcessor::get(documentFilePath);
    QTC_ASSERT(processor, return false);

    editor->insert(m_textToInsert);

    WaitForUpdatedCodeWarnings waiter(processor);
    return waiter.wait(timeOutInMs());
}

Command::Ptr InsertTextCommand::parse(BatchFileLineTokenizer &arguments,
                                      const CommandContext &context)
{
    const QString textToInsert = arguments.nextToken();
    if (textToInsert.isEmpty()) {
        qWarning("%s:%d: error: No text to insert given.",
                  qPrintable(context.filePath),
                  context.lineNumber);
        return Command::Ptr();
    }

    return Command::Ptr(new InsertTextCommand(context, textToInsert));
}

class CompleteCommand : public Command
{
public:
    CompleteCommand(const CommandContext &context);

    bool run() override;

    static Command::Ptr parse(BatchFileLineTokenizer &arguments,
                              const CommandContext &context);
};

CompleteCommand::CompleteCommand(const CommandContext &context)
    : Command(context)
{
}

bool CompleteCommand::run()
{
    qCDebug(debug) << "line" << context().lineNumber << "CompleteCommand";

    TextEditor::BaseTextEditor *editor = currentTextEditor();
    QTC_ASSERT(editor, return false);

    const QString documentFilePath = editor->document()->filePath().toString();
    auto *processor = ClangEditorDocumentProcessor::get(documentFilePath);
    QTC_ASSERT(processor, return false);

    return completionResults(editor, QStringList(), timeOutInMs());
}

Command::Ptr CompleteCommand::parse(BatchFileLineTokenizer &arguments,
                                    const CommandContext &context)
{
    Q_UNUSED(arguments)
    Q_UNUSED(context)

    return Command::Ptr(new CompleteCommand(context));
}

class SetCursorCommand : public Command
{
public:
    // line and column are 1-based
    SetCursorCommand(const CommandContext &context, int line, int column);

    bool run() override;

    static Command::Ptr parse(BatchFileLineTokenizer &arguments,
                              const CommandContext &context);

private:
    int m_line;
    int m_column;
};

SetCursorCommand::SetCursorCommand(const CommandContext &context, int line, int column)
    : Command(context)
    , m_line(line)
    , m_column(column)
{
}

bool SetCursorCommand::run()
{
    qCDebug(debug) << "line" << context().lineNumber << "SetCursorCommand" << m_line << m_column;

    TextEditor::BaseTextEditor *editor = currentTextEditor();
    QTC_ASSERT(editor, return false);

    editor->gotoLine(m_line, m_column - 1);

    return true;
}

Command::Ptr SetCursorCommand::parse(BatchFileLineTokenizer &arguments,
                                     const CommandContext &context)
{
    // Process line
    const QString line = arguments.nextToken();
    if (line.isEmpty()) {
        qWarning("%s:%d: error: No line number given.",
                  qPrintable(context.filePath),
                  context.lineNumber);
        return Command::Ptr();
    }
    bool converted = false;
    const int lineNumber = line.toInt(&converted);
    if (!converted) {
        qWarning("%s:%d: error: Invalid line number.",
                  qPrintable(context.filePath),
                  context.lineNumber);
        return Command::Ptr();
    }

    // Process column
    const QString column = arguments.nextToken();
    if (column.isEmpty()) {
        qWarning("%s:%d: error: No column number given.",
                  qPrintable(context.filePath),
                  context.lineNumber);
        return Command::Ptr();
    }
    converted = false;
    const int columnNumber = column.toInt(&converted);
    if (!converted) {
        qWarning("%s:%d: error: Invalid column number.",
                  qPrintable(context.filePath),
                  context.lineNumber);
        return Command::Ptr();
    }

    return Command::Ptr(new SetCursorCommand(context, lineNumber, columnNumber));
}

class ProcessEventsCommand : public Command
{
public:
    ProcessEventsCommand(const CommandContext &context, int durationInMs);

    bool run() override;

    static Command::Ptr parse(BatchFileLineTokenizer &arguments,
                              const CommandContext &context);

private:
    int m_durationInMs;
};

ProcessEventsCommand::ProcessEventsCommand(const CommandContext &context,
                                           int durationInMs)
    : Command(context)
    , m_durationInMs(durationInMs)
{
}

bool ProcessEventsCommand::run()
{
    qCDebug(debug) << "line" << context().lineNumber << "ProcessEventsCommand" << m_durationInMs;

    QTime time;
    time.start();

    forever {
        if (time.elapsed() > m_durationInMs)
            return true;

        QCoreApplication::processEvents();
        QThread::msleep(20);
    }
}

Command::Ptr ProcessEventsCommand::parse(BatchFileLineTokenizer &arguments,
                                         const CommandContext &context)
{
    const QString durationInMsText = arguments.nextToken();
    if (durationInMsText.isEmpty()) {
        qWarning("%s:%d: error: No duration given.",
                 qPrintable(context.filePath),
                 context.lineNumber);
        return Command::Ptr();
    }

    bool converted = false;
    const int durationInMs = durationInMsText.toInt(&converted);
    if (!converted) {
        qWarning("%s:%d: error: Invalid duration given.",
                 qPrintable(context.filePath),
                 context.lineNumber);
        return Command::Ptr();
    }

    return Command::Ptr(new ProcessEventsCommand(context, durationInMs));
}

class BatchFileReader
{
public:
    BatchFileReader(const QString &filePath);

    bool isFilePathValid() const;

    QString read() const;

private:
    const QString m_batchFilePath;
};

BatchFileReader::BatchFileReader(const QString &filePath)
    : m_batchFilePath(filePath)
{
}

bool BatchFileReader::isFilePathValid() const
{
    QFileInfo fileInfo(m_batchFilePath);

    return !m_batchFilePath.isEmpty()
        && fileInfo.isFile()
        && fileInfo.isReadable();
}

QString BatchFileReader::read() const
{
    QFile file(m_batchFilePath);
    QTC_CHECK(file.open(QFile::ReadOnly | QFile::Text));

    return QString::fromLocal8Bit(file.readAll());
}

class BatchFileParser
{
public:
    BatchFileParser(const QString &filePath,
                    const QString &commands);

    bool parse();
    QVector<Command::Ptr> commands() const;

private:
    bool advanceLine();
    QString currentLine() const;
    bool parseLine(const QString &line);

private:
    using ParseFunction = Command::Ptr (*)(BatchFileLineTokenizer &, const CommandContext &);
    using CommandToParseFunction = QHash<QString, ParseFunction>;
    CommandToParseFunction m_commandParsers;

    int m_currentLineIndex = -1;
    CommandContext m_context;
    QStringList m_lines;
    QVector<Command::Ptr> m_commands;
};

BatchFileParser::BatchFileParser(const QString &filePath,
                                 const QString &commands)
    : m_lines(commands.split('\n'))
{
    m_context.filePath = filePath;

    m_commandParsers.insert("openProject", &OpenProjectCommand::parse);
    m_commandParsers.insert("openDocument", &OpenDocumentCommand::parse);
    m_commandParsers.insert("closeAllDocuments", &CloseAllDocuments::parse);
    m_commandParsers.insert("setCursor", &SetCursorCommand::parse);
    m_commandParsers.insert("insertText", &InsertTextCommand::parse);
    m_commandParsers.insert("complete", &CompleteCommand::parse);
    m_commandParsers.insert("processEvents", &ProcessEventsCommand::parse);
}

bool BatchFileParser::parse()
{
    while (advanceLine()) {
        const QString line = currentLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (!parseLine(line))
            return false;
    }

    return true;
}

QVector<Command::Ptr> BatchFileParser::commands() const
{
    return m_commands;
}

bool BatchFileParser::advanceLine()
{
    ++m_currentLineIndex;
    m_context.lineNumber = m_currentLineIndex + 1;
    return m_currentLineIndex < m_lines.size();
}

QString BatchFileParser::currentLine() const
{
    return m_lines[m_currentLineIndex];
}

bool BatchFileParser::parseLine(const QString &line)
{
    BatchFileLineTokenizer tokenizer(line);
    QString command = tokenizer.nextToken();
    QTC_CHECK(!command.isEmpty());

    if (const ParseFunction parseFunction = m_commandParsers.value(command)) {
        if (Command::Ptr cmd = parseFunction(tokenizer, m_context)) {
            m_commands.append(cmd);
            return true;
        }

        return false;
    }

    qWarning("%s:%d: error: Unknown command \"%s\".",
             qPrintable(m_context.filePath),
             m_context.lineNumber,
             qPrintable(command));

    return false;
}

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {

static QString applySubstitutions(const QString &filePath, const QString &text)
{
    const QString dirPath = QFileInfo(filePath).absolutePath();

    QString result = text;
    result.replace("${PWD}", dirPath);

    return result;
}

bool runClangBatchFile(const QString &filePath)
{
    qWarning("ClangBatchFileProcessor: Running \"%s\".", qPrintable(filePath));

    BatchFileReader reader(filePath);
    QTC_ASSERT(reader.isFilePathValid(), return false);
    const QString fileContent = reader.read();
    const QString fileContentWithSubstitutionsApplied = applySubstitutions(filePath, fileContent);

    BatchFileParser parser(filePath, fileContentWithSubstitutionsApplied);
    QTC_ASSERT(parser.parse(), return false);
    const QVector<Command::Ptr> commands = parser.commands();

    Utils::ExecuteOnDestruction closeAllEditors([](){
        qWarning("ClangBatchFileProcessor: Finished, closing all documents.");
        QTC_CHECK(Core::EditorManager::closeAllEditors(/*askAboutModifiedEditors=*/ false));
    });

    foreach (const Command::Ptr &command, commands) {
        const bool runSucceeded = command->run();
        QCoreApplication::processEvents(); // Update GUI

        if (!runSucceeded) {
            const CommandContext context = command->context();
            qWarning("%s:%d: Failed to run.",
                     qPrintable(context.filePath),
                     context.lineNumber);
            return false;
        }
    }

    return true;
}

} // namespace Internal
} // namespace ClangCodeModel

#include "clangbatchfileprocessor.moc"
