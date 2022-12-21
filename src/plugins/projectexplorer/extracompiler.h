// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectnodes.h"
#include "project.h"
#include "task.h"

#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>
#include <utils/environment.h>

#include <QByteArray>
#include <QHash>
#include <QList>

#include <functional>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QThreadPool);
QT_BEGIN_NAMESPACE
template <typename T>
class QFutureInterface;
template <typename T>
class QFutureWatcher;
QT_END_NAMESPACE

namespace Utils { class QtcProcess; }

namespace ProjectExplorer {

class ExtraCompilerPrivate;
using FileNameToContentsHash = QHash<Utils::FilePath, QByteArray>;

class PROJECTEXPLORER_EXPORT ExtraCompiler : public QObject
{
    Q_OBJECT
public:

    ExtraCompiler(const Project *project, const Utils::FilePath &source,
                  const Utils::FilePaths &targets, QObject *parent = nullptr);
    ~ExtraCompiler() override;

    const Project *project() const;
    Utils::FilePath source() const;

    QByteArray content(const Utils::FilePath &file) const;

    Utils::FilePaths targets() const;
    void forEachTarget(std::function<void(const Utils::FilePath &)> func);

    virtual QFuture<FileNameToContentsHash> run() = 0;
    bool isDirty() const;

signals:
    void contentsChanged(const Utils::FilePath &file);

protected:
    static QThreadPool *extraCompilerThreadPool();
    void setContent(const Utils::FilePath &file, const QByteArray &content);
    void updateCompileTime();
    Utils::Environment buildEnvironment() const;
    void setCompileIssues(const Tasks &issues);

private:
    void onTargetsBuilt(Project *project);
    void onEditorChanged(Core::IEditor *editor);
    void onEditorAboutToClose(Core::IEditor *editor);
    void setDirty();
    // This method may not block!
    virtual void run(const QByteArray &sourceContent) = 0;

    const std::unique_ptr<ExtraCompilerPrivate> d;
};

class PROJECTEXPLORER_EXPORT ProcessExtraCompiler : public ExtraCompiler
{
    Q_OBJECT
public:

    ProcessExtraCompiler(const Project *project, const Utils::FilePath &source,
                         const Utils::FilePaths &targets, QObject *parent = nullptr);
    ~ProcessExtraCompiler() override;

protected:
    // This will run a process in a thread, if
    //  * command() does not return an empty file name
    //  * command() is exectuable
    //  * prepareToRun returns true
    //  * The process is not yet running
    void run(const QByteArray &sourceContents) override;
    QFuture<FileNameToContentsHash> run() override;

    // Information about the process to run:
    virtual Utils::FilePath workingDirectory() const;
    virtual Utils::FilePath command() const = 0;
    virtual QStringList arguments() const;

    virtual bool prepareToRun(const QByteArray &sourceContents);

    virtual FileNameToContentsHash handleProcessFinished(Utils::QtcProcess *process) = 0;

    virtual Tasks parseIssues(const QByteArray &stdErr);

private:
    using ContentProvider = std::function<QByteArray()>;
    QFuture<FileNameToContentsHash> runImpl(const ContentProvider &sourceContents);
    void runInThread(QFutureInterface<FileNameToContentsHash> &futureInterface,
                     const Utils::FilePath &cmd, const Utils::FilePath &workDir,
                     const QStringList &args, const ContentProvider &provider,
                     const Utils::Environment &env);
    void cleanUp();

    QFutureWatcher<FileNameToContentsHash> *m_watcher = nullptr;
};

class PROJECTEXPLORER_EXPORT ExtraCompilerFactory : public QObject
{
    Q_OBJECT
public:
    explicit ExtraCompilerFactory(QObject *parent = nullptr);
    ~ExtraCompilerFactory() override;

    virtual FileType sourceType() const = 0;
    virtual QString sourceTag() const = 0;

    virtual ExtraCompiler *create(const Project *project,
                                  const Utils::FilePath &source,
                                  const Utils::FilePaths &targets)
        = 0;

    static QList<ExtraCompilerFactory *> extraCompilerFactories();
};

} // namespace ProjectExplorer
