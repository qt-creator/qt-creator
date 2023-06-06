// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectnodes.h"
#include "project.h"

#include <coreplugin/editormanager/ieditor.h>

#include <utils/environment.h>
#include <utils/filepath.h>

#include <QByteArray>
#include <QHash>
#include <QList>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
template <typename T>
class QPromise;
class QThreadPool;
QT_END_NAMESPACE

namespace Tasking { class GroupItem; }
namespace Utils { class Process; }

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
    void forEachTarget(std::function<void(const Utils::FilePath &)> func) const;

    Tasking::GroupItem compileFileItem();
    void compileFile();
    bool isDirty() const;
    void block();
    void unblock();

signals:
    void contentsChanged(const Utils::FilePath &file);

protected:
    static QThreadPool *extraCompilerThreadPool();

    void setContent(const Utils::FilePath &file, const QByteArray &content);
    void updateCompileTime();
    Utils::Environment buildEnvironment() const;
    using ContentProvider = std::function<QByteArray()>;

private:
    void onTargetsBuilt(Project *project);
    void onEditorChanged(Core::IEditor *editor);
    void onEditorAboutToClose(Core::IEditor *editor);
    void setDirty();
    ContentProvider fromFileProvider() const;
    void compileContent(const QByteArray &content);
    void compileImpl(const ContentProvider &provider);
    void compileIfDirty();
    virtual Tasking::GroupItem taskItemImpl(const ContentProvider &provider) = 0;

    const std::unique_ptr<ExtraCompilerPrivate> d;
};

class PROJECTEXPLORER_EXPORT ProcessExtraCompiler : public ExtraCompiler
{
    Q_OBJECT
public:

    ProcessExtraCompiler(const Project *project, const Utils::FilePath &source,
                         const Utils::FilePaths &targets, QObject *parent = nullptr);

protected:
    // Information about the process to run:
    virtual Utils::FilePath workingDirectory() const;
    virtual Utils::FilePath command() const = 0;
    virtual QStringList arguments() const;

    virtual bool prepareToRun(const QByteArray &sourceContents);

    virtual FileNameToContentsHash handleProcessFinished(Utils::Process *process) = 0;

    virtual Tasks parseIssues(const QByteArray &stdErr);

private:
    Tasking::GroupItem taskItemImpl(const ContentProvider &provider) final;
    void runInThread(QPromise<FileNameToContentsHash> &promise,
                     const Utils::FilePath &cmd, const Utils::FilePath &workDir,
                     const QStringList &args, const ContentProvider &provider,
                     const Utils::Environment &env);
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
                                  const Utils::FilePaths &targets) = 0;

    static QList<ExtraCompilerFactory *> extraCompilerFactories();
};

} // namespace ProjectExplorer
