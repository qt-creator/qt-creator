/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "extracompiler.h"

#include "buildconfiguration.h"
#include "buildmanager.h"
#include "kitinformation.h"
#include "session.h"
#include "target.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/fontsettings.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QDateTime>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QTextBlock>
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

    connect(&d->timer, &QTimer::timeout, this, [this](){
        if (d->dirty && d->lastEditor) {
            d->dirty = false;
            run(d->lastEditor->document()->contents());
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
        QFileInfo targetFileInfo(target.toFileInfo());
        if (!targetFileInfo.exists()) {
            d->dirty = true;
            continue;
        }

        QDateTime lastModified = targetFileInfo.lastModified();
        if (lastModified < sourceTime)
            d->dirty = true;

        if (!d->compileTime.isValid() || d->compileTime > lastModified)
            d->compileTime = lastModified;

        QFile file(target.toString());
        if (file.open(QFile::ReadOnly | QFile::Text))
            setContent(target, file.readAll());
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

void ExtraCompiler::forEachTarget(std::function<void (const FilePath &)> func)
{
    for (auto it = d->contents.constBegin(), end = d->contents.constEnd(); it != end; ++it)
        func(it.key());
}

void ExtraCompiler::setCompileTime(const QDateTime &time)
{
    d->compileTime = time;
}

QDateTime ExtraCompiler::compileTime() const
{
    return d->compileTime;
}

QThreadPool *ExtraCompiler::extraCompilerThreadPool()
{
    return s_extraCompilerThreadPool();
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

            QFile file(target.toString());
            if (file.open(QFile::ReadOnly | QFile::Text)) {
                d->compileTime = generateTime;
                setContent(target, file.readAll());
            }
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
            run(doc->contents());
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
        run(doc->contents());
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
    for (const Task &issue : qAsConst(issues)) {
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

ProcessExtraCompiler::~ProcessExtraCompiler()
{
    if (!m_watcher)
        return;
    m_watcher->cancel();
    m_watcher->waitForFinished();
}

void ProcessExtraCompiler::run(const QByteArray &sourceContents)
{
    ContentProvider contents = [sourceContents]() { return sourceContents; };
    runImpl(contents);
}

QFuture<FileNameToContentsHash> ProcessExtraCompiler::run()
{
    const FilePath fileName = source();
    ContentProvider contents = [fileName]() {
        QFile file(fileName.toString());
        if (!file.open(QFile::ReadOnly | QFile::Text))
            return QByteArray();
        return file.readAll();
    };
    return runImpl(contents);
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

QFuture<FileNameToContentsHash> ProcessExtraCompiler::runImpl(const ContentProvider &provider)
{
    if (m_watcher)
        delete m_watcher;

    m_watcher = new QFutureWatcher<FileNameToContentsHash>();
    connect(m_watcher, &QFutureWatcher<FileNameToContentsHash>::finished,
            this, &ProcessExtraCompiler::cleanUp);

    m_watcher->setFuture(runAsync(extraCompilerThreadPool(),
                                         &ProcessExtraCompiler::runInThread, this,
                                         command(), workingDirectory(), arguments(), provider,
                                         buildEnvironment()));
    return m_watcher->future();
}

void ProcessExtraCompiler::runInThread(
        QFutureInterface<FileNameToContentsHash> &futureInterface,
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

    while (!futureInterface.isCanceled())
        if (process.waitForFinished(200))
            break;

    if (futureInterface.isCanceled())
        return;

    futureInterface.reportResult(handleProcessFinished(&process));
}

void ProcessExtraCompiler::cleanUp()
{
    QTC_ASSERT(m_watcher, return);
    auto future = m_watcher->future();
    delete m_watcher;
    m_watcher = nullptr;
    if (!future.resultCount())
        return;
    const FileNameToContentsHash data = future.result();

    if (data.isEmpty())
        return; // There was some kind of error...

    for (auto it = data.constBegin(), end = data.constEnd(); it != end; ++it)
        setContent(it.key(), it.value());

    setCompileTime(QDateTime::currentDateTime());
}

} // namespace ProjectExplorer
