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

#pragma once

#include "projectnodes.h"
#include "project.h"
#include "task.h"

#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>
#include <utils/environment.h>

#include <QByteArray>
#include <QFuture>
#include <QHash>
#include <QList>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QProcess);
QT_FORWARD_DECLARE_CLASS(QThreadPool);

namespace ProjectExplorer {

class ExtraCompilerPrivate;
using FileNameToContentsHash = QHash<Utils::FileName, QByteArray>;

class PROJECTEXPLORER_EXPORT ExtraCompiler : public QObject
{
    Q_OBJECT
public:

    ExtraCompiler(const Project *project, const Utils::FileName &source,
                  const Utils::FileNameList &targets, QObject *parent = nullptr);
    ~ExtraCompiler() override;

    const Project *project() const;
    Utils::FileName source() const;

    // You can set the contents from the outside. This is done if the file has been (re)created by
    // the regular build process.
    void setContent(const Utils::FileName &file, const QByteArray &content);
    QByteArray content(const Utils::FileName &file) const;

    Utils::FileNameList targets() const;
    void forEachTarget(std::function<void(const Utils::FileName &)> func);

    void setCompileTime(const QDateTime &time);
    QDateTime compileTime() const;

    static QThreadPool *extraCompilerThreadPool();

signals:
    void contentsChanged(const Utils::FileName &file);

protected:
    Utils::Environment buildEnvironment() const;
    void setCompileIssues(const QList<Task> &issues);

private:
    void onTargetsBuilt(Project *project);
    void onEditorChanged(Core::IEditor *editor);
    void onEditorAboutToClose(Core::IEditor *editor);
    void onActiveTargetChanged();
    void onActiveBuildConfigurationChanged();
    void setDirty();
    // This method may not block!
    virtual void run(const QByteArray &sourceContent) = 0;
    virtual void run(const Utils::FileName &file) = 0;

    ExtraCompilerPrivate *const d;
};

class PROJECTEXPLORER_EXPORT ProcessExtraCompiler : public ExtraCompiler
{
    Q_OBJECT
public:

    ProcessExtraCompiler(const Project *project, const Utils::FileName &source,
                         const Utils::FileNameList &targets, QObject *parent = nullptr);
    ~ProcessExtraCompiler();

protected:
    // This will run a process in a thread, if
    //  * command() does not return an empty file name
    //  * command() is exectuable
    //  * prepareToRun returns true
    //  * The process is not yet running
    void run(const QByteArray &sourceContents) override;
    void run(const Utils::FileName &fileName) override;

    // Information about the process to run:
    virtual Utils::FileName workingDirectory() const;
    virtual Utils::FileName command() const = 0;
    virtual QStringList arguments() const;

    virtual bool prepareToRun(const QByteArray &sourceContents);

    virtual void handleProcessError(QProcess *process) { Q_UNUSED(process); }
    virtual void handleProcessStarted(QProcess *process, const QByteArray &sourceContents)
    { Q_UNUSED(process); Q_UNUSED(sourceContents); }
    virtual FileNameToContentsHash handleProcessFinished(QProcess *process) = 0;

    virtual QList<Task> parseIssues(const QByteArray &stdErr);

private:
    using ContentProvider = std::function<QByteArray()>;
    void runImpl(const ContentProvider &sourceContents);
    void runInThread(QFutureInterface<FileNameToContentsHash> &futureInterface,
                     const Utils::FileName &cmd, const Utils::FileName &workDir,
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
    ~ExtraCompilerFactory();

    virtual FileType sourceType() const = 0;
    virtual QString sourceTag() const = 0;

    virtual ExtraCompiler *create(const Project *project, const Utils::FileName &source,
                                  const Utils::FileNameList &targets) = 0;

    static QList<ExtraCompilerFactory *> extraCompilerFactories();
};

} // namespace ProjectExplorer
