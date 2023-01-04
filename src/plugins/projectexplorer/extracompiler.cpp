// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extracompiler.h"

#include "buildmanager.h"
#include "kitinformation.h"
#include "session.h"
#include "target.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>

#include <utils/asynctask.h>
#include <utils/expected.h>
#include <utils/qtcprocess.h>

#include <QDateTime>
#include <QFutureInterface>
#include <QThreadPool>
#include <QTimer>

using namespace Utils;

namespace ProjectExplorer {

Q_GLOBAL_STATIC(QThreadPool, s_extraCompilerThreadPool);
Q_GLOBAL_STATIC(QList<ExtraCompilerFactory *>, factories);

class ExtraCompilerPrivate
{
public:
    const Project *project;
    FilePath source;
    FileNameToContentsHash contents;
    Tasks issues;
    QDateTime compileTime;
    Core::IEditor *lastEditor = nullptr;
    QMetaObject::Connection activeBuildConfigConnection;
    QMetaObject::Connection activeEnvironmentConnection;
    bool dirty = false;

    QTimer timer;
    void updateIssues();

    FutureSynchronizer m_futureSynchronizer;
    std::unique_ptr<TaskTree> m_taskTree;
};

ExtraCompiler::ExtraCompiler(const Project *project, const FilePath &source,
                             const FilePaths &targets, QObject *parent) :
    QObject(parent), d(std::make_unique<ExtraCompilerPrivate>())
{
    d->project = project;
    d->source = source;
    for (const FilePath &target : targets)
        d->contents.insert(target, QByteArray());
    d->timer.setSingleShot(true);

    connect(&d->timer, &QTimer::timeout, this, [this] {
        if (d->dirty && d->lastEditor) {
            d->dirty = false;
            compileContent(d->lastEditor->document()->contents());
        }
    });

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &ExtraCompiler::onTargetsBuilt);

    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, [this](Project *project) {
        if (project == d->project)
            deleteLater();
    });

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ExtraCompiler::onEditorChanged);
    connect(editorManager, &Core::EditorManager::editorAboutToClose,
            this, &ExtraCompiler::onEditorAboutToClose);

    // Use existing target files, where possible. Otherwise run the compiler.
    QDateTime sourceTime = d->source.lastModified();
    for (const FilePath &target : targets) {
        if (!target.exists()) {
            d->dirty = true;
            continue;
        }

        QDateTime lastModified = target.lastModified();
        if (lastModified < sourceTime)
            d->dirty = true;

        if (!d->compileTime.isValid() || d->compileTime > lastModified)
            d->compileTime = lastModified;

        const expected_str<QByteArray> contents = target.fileContents();
        QTC_ASSERT_EXPECTED(contents, return);

        setContent(target, *contents);
    }
}

ExtraCompiler::~ExtraCompiler() = default;

const Project *ExtraCompiler::project() const
{
    return d->project;
}

FilePath ExtraCompiler::source() const
{
    return d->source;
}

QByteArray ExtraCompiler::content(const FilePath &file) const
{
    return d->contents.value(file);
}

FilePaths ExtraCompiler::targets() const
{
    return d->contents.keys();
}

void ExtraCompiler::forEachTarget(std::function<void (const FilePath &)> func) const
{
    for (auto it = d->contents.constBegin(), end = d->contents.constEnd(); it != end; ++it)
        func(it.key());
}

void ExtraCompiler::updateCompileTime()
{
    d->compileTime = QDateTime::currentDateTime();
}

QThreadPool *ExtraCompiler::extraCompilerThreadPool()
{
    return s_extraCompilerThreadPool();
}

Tasking::TaskItem ExtraCompiler::compileFileItem()
{
    return taskItemImpl(fromFileProvider());
}

void ExtraCompiler::compileFile()
{
    compileImpl(fromFileProvider());
}

void ExtraCompiler::compileContent(const QByteArray &content)
{
    compileImpl([content] { return content; });
}

void ExtraCompiler::compileImpl(const ContentProvider &provider)
{
    const auto finalize = [=] {
        d->m_taskTree.release()->deleteLater();
    };
    d->m_taskTree.reset(new TaskTree({taskItemImpl(provider)}));
    connect(d->m_taskTree.get(), &TaskTree::done, this, finalize);
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, this, finalize);
}

ExtraCompiler::ContentProvider ExtraCompiler::fromFileProvider() const
{
    const auto provider = [fileName = source()] {
        QFile file(fileName.toString());
        if (!file.open(QFile::ReadOnly | QFile::Text))
            return QByteArray();
        return file.readAll();
    };
    return provider;
}

bool ExtraCompiler::isDirty() const
{
    return d->dirty;
}

void ExtraCompiler::onTargetsBuilt(Project *project)
{
    if (project != d->project || BuildManager::isBuilding(project))
        return;

    // This is mostly a fall back for the cases when the generator couldn't be run.
    // It pays special attention to the case where a source file was newly created
    const QDateTime sourceTime = d->source.lastModified();
    if (d->compileTime.isValid() && d->compileTime >= sourceTime)
        return;

    forEachTarget([&](const FilePath &target) {
        QFileInfo fi(target.toFileInfo());
        QDateTime generateTime = fi.exists() ? fi.lastModified() : QDateTime();
        if (generateTime.isValid() && (generateTime > sourceTime)) {
            if (d->compileTime >= generateTime)
                return;

            const expected_str<QByteArray> contents = target.fileContents();
            QTC_ASSERT_EXPECTED(contents, return);

            d->compileTime = generateTime;
            setContent(target, *contents);
        }
    });
}

void ExtraCompiler::onEditorChanged(Core::IEditor *editor)
{
    // Handle old editor
    if (d->lastEditor) {
        Core::IDocument *doc = d->lastEditor->document();
        disconnect(doc, &Core::IDocument::contentsChanged,
                   this, &ExtraCompiler::setDirty);

        if (d->dirty) {
            d->dirty = false;
            compileContent(doc->contents());
        }
    }

    if (editor && editor->document()->filePath() == d->source) {
        d->lastEditor = editor;
        d->updateIssues();

        // Handle new editor
        connect(d->lastEditor->document(), &Core::IDocument::contentsChanged,
                this, &ExtraCompiler::setDirty);
    } else {
        d->lastEditor = nullptr;
    }
}

void ExtraCompiler::setDirty()
{
    d->dirty = true;
    d->timer.start(1000);
}

void ExtraCompiler::onEditorAboutToClose(Core::IEditor *editor)
{
    if (d->lastEditor != editor)
        return;

    // Oh no our editor is going to be closed
    // get the content first
    Core::IDocument *doc = d->lastEditor->document();
    disconnect(doc, &Core::IDocument::contentsChanged,
               this, &ExtraCompiler::setDirty);
    if (d->dirty) {
        d->dirty = false;
        compileContent(doc->contents());
    }
    d->lastEditor = nullptr;
}

Environment ExtraCompiler::buildEnvironment() const
{
    if (Target *target = project()->activeTarget()) {
        if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
            return bc->environment();
        } else {
            EnvironmentItems changes =
                    EnvironmentKitAspect::environmentChanges(target->kit());
            Environment env = Environment::systemEnvironment();
            env.modify(changes);
            return env;
        }
    }

    return Environment::systemEnvironment();
}

void ExtraCompiler::setCompileIssues(const Tasks &issues)
{
    d->issues = issues;
    d->updateIssues();
}

void ExtraCompilerPrivate::updateIssues()
{
    if (!lastEditor)
        return;

    auto widget = qobject_cast<TextEditor::TextEditorWidget *>(lastEditor->widget());
    if (!widget)
        return;

    QList<QTextEdit::ExtraSelection> selections;
    const QTextDocument *document = widget->document();
    for (const Task &issue : std::as_const(issues)) {
        QTextEdit::ExtraSelection selection;
        QTextCursor cursor(document->findBlockByNumber(issue.line - 1));
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        selection.cursor = cursor;

        const auto fontSettings = TextEditor::TextEditorSettings::fontSettings();
        selection.format = fontSettings.toTextCharFormat(issue.type == Task::Warning ?
                TextEditor::C_WARNING : TextEditor::C_ERROR);
        selection.format.setToolTip(issue.description());
        selections.append(selection);
    }

    widget->setExtraSelections(TextEditor::TextEditorWidget::CodeWarningsSelection, selections);
}

Utils::FutureSynchronizer *ExtraCompiler::futureSynchronizer() const
{
    return &d->m_futureSynchronizer;
}

void ExtraCompiler::setContent(const FilePath &file, const QByteArray &contents)
{
    auto it = d->contents.find(file);
    if (it != d->contents.end()) {
        if (it.value() != contents) {
            it.value() = contents;
            emit contentsChanged(file);
        }
    }
}

ExtraCompilerFactory::ExtraCompilerFactory(QObject *parent)
    : QObject(parent)
{
    factories->append(this);
}

ExtraCompilerFactory::~ExtraCompilerFactory()
{
    factories->removeAll(this);
}

QList<ExtraCompilerFactory *> ExtraCompilerFactory::extraCompilerFactories()
{
    return *factories();
}

ProcessExtraCompiler::ProcessExtraCompiler(const Project *project, const FilePath &source,
                                           const FilePaths &targets, QObject *parent) :
    ExtraCompiler(project, source, targets, parent)
{ }

Tasking::TaskItem ProcessExtraCompiler::taskItemImpl(const ContentProvider &provider)
{
    const auto setupTask = [=](AsyncTask<FileNameToContentsHash> &async) {
        async.setThreadPool(extraCompilerThreadPool());
        async.setAsyncCallData(&ProcessExtraCompiler::runInThread, this, command(),
                               workingDirectory(), arguments(), provider, buildEnvironment());
        async.setFutureSynchronizer(futureSynchronizer());
    };
    const auto taskDone = [=](const AsyncTask<FileNameToContentsHash> &async) {
        if (async.results().size() == 0)
            return;
        const FileNameToContentsHash data = async.result();
        if (data.isEmpty())
            return; // There was some kind of error...
        for (auto it = data.constBegin(), end = data.constEnd(); it != end; ++it)
            setContent(it.key(), it.value());
        updateCompileTime();
    };
    return Tasking::Async<FileNameToContentsHash>(setupTask, taskDone);
}

FilePath ProcessExtraCompiler::workingDirectory() const
{
    return FilePath();
}

QStringList ProcessExtraCompiler::arguments() const
{
    return QStringList();
}

bool ProcessExtraCompiler::prepareToRun(const QByteArray &sourceContents)
{
    Q_UNUSED(sourceContents)
    return true;
}

Tasks ProcessExtraCompiler::parseIssues(const QByteArray &stdErr)
{
    Q_UNUSED(stdErr)
    return {};
}

void ProcessExtraCompiler::runInThread(QFutureInterface<FileNameToContentsHash> &futureInterface,
                                       const FilePath &cmd, const FilePath &workDir,
                                       const QStringList &args, const ContentProvider &provider,
                                       const Environment &env)
{
    if (cmd.isEmpty() || !cmd.toFileInfo().isExecutable())
        return;

    const QByteArray sourceContents = provider();
    if (sourceContents.isNull() || !prepareToRun(sourceContents))
        return;

    QtcProcess process;

    process.setEnvironment(env);
    if (!workDir.isEmpty())
        process.setWorkingDirectory(workDir);
    process.setCommand({ cmd, args });
    process.setWriteData(sourceContents);
    process.start();
    if (!process.waitForStarted())
        return;

    while (!futureInterface.isCanceled()) {
        if (process.waitForFinished(200))
            break;
    }

    if (futureInterface.isCanceled())
        return;

    futureInterface.reportResult(handleProcessFinished(&process));
}

} // namespace ProjectExplorer
