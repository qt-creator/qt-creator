/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion and Hugues Delorme
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

#include "vcsbase_global.h"

#include "vcsbaseclientsettings.h"

#include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/processenums.h>

#include <QObject>
#include <QStringList>
#include <QVariant>

#include <functional>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QTextCodec;
class QToolBar;
QT_END_NAMESPACE

namespace Utils { class QtcProcess; }

namespace VcsBase {

class VcsCommand;
class VcsBaseEditorWidget;
class VcsJob;
class VcsBaseClientImplPrivate;
class VcsBaseEditorConfig;

class VCSBASE_EXPORT VcsBaseClientImpl : public QObject
{
    Q_OBJECT

public:
    explicit VcsBaseClientImpl(VcsBaseSettings *baseSettings);
    ~VcsBaseClientImpl() override = default;

    VcsBaseSettings &settings() const;

    virtual Utils::FilePath vcsBinary() const;
    int vcsTimeoutS() const;

    enum JobOutputBindMode {
        NoOutputBind,
        VcsWindowOutputBind
    };

    VcsBaseEditorWidget *createVcsEditor(Utils::Id kind, QString title,
                                         const QString &source, QTextCodec *codec,
                                         const char *registerDynamicProperty,
                                         const QString &dynamicPropertyValue) const;

    VcsCommand *createCommand(const Utils::FilePath &workingDirectory,
                              VcsBaseEditorWidget *editor = nullptr,
                              JobOutputBindMode mode = NoOutputBind) const;

    void enqueueJob(VcsCommand *cmd, const QStringList &args,
                    const Utils::ExitCodeInterpreter &interpreter = {}) const;

    virtual Utils::Environment processEnvironment() const;

    // VCS functionality:
    virtual VcsBaseEditorWidget *annotate(const Utils::FilePath &workingDir,
                                          const QString &file,
                                          const QString &revision = {},
                                          int lineNumber = -1,
                                          const QStringList &extraOptions = {}) = 0;

    static QStringList splitLines(const QString &s);

    static QString stripLastNewline(const QString &in);

    // Fully synchronous VCS execution (QProcess-based)
    void vcsFullySynchronousExec(Utils::QtcProcess &process,
                                 const Utils::FilePath &workingDir, const QStringList &args,
                                 unsigned flags = 0, int timeoutS = -1, QTextCodec *codec = nullptr) const;
    void vcsFullySynchronousExec(Utils::QtcProcess &process,
                                 const Utils::FilePath &workingDir, const Utils::CommandLine &cmdLine,
                                 unsigned flags = 0, int timeoutS = -1, QTextCodec *codec = nullptr) const;

    // Simple helper to execute a single command using createCommand and enqueueJob.
    VcsCommand *vcsExec(const Utils::FilePath &workingDirectory, const QStringList &arguments,
                        VcsBaseEditorWidget *editor = nullptr, bool useOutputToWindow = false,
                        unsigned additionalFlags = 0, const QVariant &cookie = {}) const;

protected:
    void resetCachedVcsInfo(const Utils::FilePath &workingDir);
    virtual void annotateRevisionRequested(const Utils::FilePath &workingDirectory, const QString &file,
                                           const QString &change, int line);

    // Synchronous VCS execution using Utils::SynchronousProcess, with
    // log windows updating (using VcsBasePlugin::runVcs with flags)
    void vcsSynchronousExec(Utils::QtcProcess &proc,
                            const Utils::FilePath &workingDir,
                            const QStringList &args,
                            unsigned flags = 0,
                            QTextCodec *outputCodec = nullptr) const;

private:
    void saveSettings();

    VcsBaseSettings *m_baseSettings = nullptr; // Aspect based.
};

class VCSBASE_EXPORT VcsBaseClient : public VcsBaseClientImpl
{
    Q_OBJECT

public:
    class VCSBASE_EXPORT StatusItem
    {
    public:
        StatusItem() = default;
        QString flags;
        QString file;
    };

    explicit VcsBaseClient(VcsBaseSettings *baseSettings);

    virtual bool synchronousCreateRepository(const Utils::FilePath &workingDir,
                                             const QStringList &extraOptions = {});
    virtual bool synchronousClone(const Utils::FilePath &workingDir,
                                  const QString &srcLocation,
                                  const QString &dstLocation,
                                  const QStringList &extraOptions = {});
    virtual bool synchronousAdd(const Utils::FilePath &workingDir,
                                const QString &relFileName,
                                const QStringList &extraOptions = {});
    virtual bool synchronousRemove(const Utils::FilePath &workingDir,
                                   const QString &fileName,
                                   const QStringList &extraOptions = {});
    virtual bool synchronousMove(const Utils::FilePath &workingDir,
                                 const QString &from, const QString &to,
                                 const QStringList &extraOptions = {});
    virtual bool synchronousPull(const Utils::FilePath &workingDir,
                                 const QString &srcLocation,
                                 const QStringList &extraOptions = {});
    virtual bool synchronousPush(const Utils::FilePath &workingDir,
                                 const QString &dstLocation,
                                 const QStringList &extraOptions = {});
    VcsBaseEditorWidget *annotate(const Utils::FilePath &workingDir,
                                  const QString &file,
                                  const QString &revision = {},
                                  int lineNumber = -1,
                                  const QStringList &extraOptions = {}) override;
    virtual void diff(const Utils::FilePath &workingDir,
                      const QStringList &files = {},
                      const QStringList &extraOptions = {});
    virtual void log(const Utils::FilePath &workingDir,
                     const QStringList &files = {},
                     const QStringList &extraOptions = {},
                     bool enableAnnotationContextMenu = false);
    virtual void status(const Utils::FilePath &workingDir,
                        const QString &file = {},
                        const QStringList &extraOptions = {});
    virtual void emitParsedStatus(const Utils::FilePath &repository,
                                  const QStringList &extraOptions = {});
    virtual void revertFile(const Utils::FilePath &workingDir,
                            const QString &file,
                            const QString &revision = {},
                            const QStringList &extraOptions = {});
    virtual void revertAll(const Utils::FilePath &workingDir,
                           const QString &revision = {},
                           const QStringList &extraOptions = {});
    virtual void import(const Utils::FilePath &repositoryRoot,
                        const QStringList &files,
                        const QStringList &extraOptions = {});
    virtual void update(const Utils::FilePath &repositoryRoot,
                        const QString &revision = {},
                        const QStringList &extraOptions = {});
    virtual void commit(const Utils::FilePath &repositoryRoot,
                        const QStringList &files,
                        const QString &commitMessageFile,
                        const QStringList &extraOptions = {});

    virtual Utils::FilePath findTopLevelForFile(const Utils::FilePath &/*file*/) const { return {}; }

    virtual void view(const QString &source, const QString &id,
                      const QStringList &extraOptions = QStringList());

signals:
    void parsedStatus(const QList<VcsBase::VcsBaseClient::StatusItem> &statusList);
    // Passes on changed signals from VcsJob to Control
    void changed(const QVariant &v);

public:
    enum VcsCommandTag
    {
        CreateRepositoryCommand,
        CloneCommand,
        AddCommand,
        RemoveCommand,
        MoveCommand,
        PullCommand,
        PushCommand,
        CommitCommand,
        ImportCommand,
        UpdateCommand,
        RevertCommand,
        AnnotateCommand,
        DiffCommand,
        LogCommand,
        StatusCommand
    };
protected:
    virtual QString vcsCommandString(VcsCommandTag cmd) const;
    virtual Utils::Id vcsEditorKind(VcsCommandTag cmd) const = 0;
    virtual Utils::ExitCodeInterpreter exitCodeInterpreter(VcsCommandTag cmd) const;

    virtual QStringList revisionSpec(const QString &/*revision*/) const { return {}; }

    typedef std::function<VcsBaseEditorConfig *(QToolBar *)> ConfigCreator;
    void setDiffConfigCreator(ConfigCreator creator);
    void setLogConfigCreator(ConfigCreator creator);

    virtual StatusItem parseStatusLine(const QString &/*line*/) const { return {}; }

    QString vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const;

private:
    void statusParser(const QString&);

    VcsBaseClient::ConfigCreator m_diffConfigCreator;
    VcsBaseClient::ConfigCreator m_logConfigCreator;
};

} //namespace VcsBase
