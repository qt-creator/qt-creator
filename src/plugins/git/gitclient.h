/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GITCLIENT_H
#define GITCLIENT_H

#include "gitsettings.h"
#include "gitcommand.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QString>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QErrorMessage;
class QSignalMapper;
class QDebug;
QT_END_NAMESPACE

namespace Core {
    class ICore;
}

namespace VCSBase {
    class VCSBaseEditor;
}

namespace Git {
namespace Internal {

class GitPlugin;
class GitOutputWindow;
class GitCommand;
struct CommitData;
struct GitSubmitEditorPanelData;
struct Stash;

class GitClient : public QObject
{
    Q_OBJECT

public:
    static const char *stashNamePrefix;

    explicit GitClient(GitPlugin *plugin);
    ~GitClient();

    bool managesDirectory(const QString &) const { return false; }
    QString findTopLevelForDirectory(const QString &) const { return QString(); }

    static QString findRepositoryForFile(const QString &fileName);
    static QString findRepositoryForDirectory(const QString &dir);

    void diff(const QString &workingDirectory, const QStringList &diffArgs, const QString &fileName);
    void diff(const QString &workingDirectory, const QStringList &diffArgs,
              const QStringList &unstagedFileNames, const QStringList &stagedFileNames= QStringList());

    void status(const QString &workingDirectory);
    void graphLog(const QString &workingDirectory);
    void log(const QString &workingDirectory, const QStringList &fileNames,
             bool enableAnnotationContextMenu = false);
    void blame(const QString &workingDirectory, const QString &fileName,
               const QString &revision = QString(), int lineNumber = -1);
    void checkout(const QString &workingDirectory, const QString &file);
    void checkoutBranch(const QString &workingDirectory, const QString &branch);
    void hardReset(const QString &workingDirectory, const QString &commit = QString());
    void addFile(const QString &workingDirectory, const QString &fileName);
    bool synchronousAdd(const QString &workingDirectory, const QStringList &files);
    bool synchronousReset(const QString &workingDirectory,
                          const QStringList &files = QStringList(),
                          QString *errorMessage = 0);
    bool synchronousInit(const QString &workingDirectory);
    bool synchronousCheckoutFiles(const QString &workingDirectory,
                                  QStringList files = QStringList(),
                                  QString revision = QString(), QString *errorMessage = 0);
    // Checkout branch
    bool synchronousCheckoutBranch(const QString &workingDirectory, const QString &branch, QString *errorMessage = 0);

    // Do a stash and return identier.
    enum { StashPromptDescription = 0x1, StashImmediateRestore = 0x2, StashIgnoreUnchanged = 0x4 };
    QString synchronousStash(const QString &workingDirectory,
                             const QString &messageKeyword = QString(),
                             unsigned flags = 0, bool *unchanged = 0);

    bool executeSynchronousStash(const QString &workingDirectory,
                                 const QString &message = QString(),
                                 QString *errorMessage = 0);
    bool synchronousStashRestore(const QString &workingDirectory,
                                 const QString &stash,
                                 const QString &branch = QString(),
                                 QString *errorMessage = 0);
    bool synchronousStashRemove(const QString &workingDirectory,
                                const QString &stash = QString(),
                                QString *errorMessage = 0);
    bool synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                              QString *output, QString *errorMessage);
    bool synchronousShow(const QString &workingDirectory, const QString &id,
                              QString *output, QString *errorMessage);
    bool synchronousParentRevisions(const QString &workingDirectory,
                                    const QStringList &files /* = QStringList() */,
                                    const QString &revision,
                                    QStringList *parents,
                                    QString *errorMessage);
    bool synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                     QString *description, QString *errorMessage);
    bool synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                     const QString &format, QString *description, QString *errorMessage);
    bool synchronousShortDescriptions(const QString &workingDirectory, const QStringList &revisions,
                                      QStringList *descriptions, QString *errorMessage);
    bool synchronousTopRevision(const QString &workingDirectory, QString *revision = 0,
                                QString *branch = 0, QString *errorMessage = 0);

    void pull(const QString &workingDirectory);
    void push(const QString &workingDirectory);

    void stashPop(const QString &workingDirectory);
    void revert(const QStringList &files);
    void branchList(const QString &workingDirectory);
    void stashList(const QString &workingDirectory);
    bool synchronousStashList(const QString &workingDirectory,
                              QList<Stash> *stashes,
                              QString *errorMessage = 0);
    // Resolve a stash name from message (for IVersionControl's names).
    bool stashNameFromMessage(const QString &workingDirectory,
                              const QString &messge, QString *name,
                              QString *errorMessage = 0);

    QString readConfig(const QString &workingDirectory, const QStringList &configVar);

    QString readConfigValue(const QString &workingDirectory, const QString &configVar);

    enum StashResult { StashUnchanged, StashCanceled, StashFailed,
                       Stashed, NotStashed /* User did not want it */ };
    StashResult ensureStash(const QString &workingDirectory, QString *errorMessage);
    StashResult ensureStash(const QString &workingDirectory);

    bool getCommitData(const QString &workingDirectory,
                       QString *commitTemplate,
                       CommitData *d,
                       QString *errorMessage);

    bool addAndCommit(const QString &workingDirectory,
                      const GitSubmitEditorPanelData &data,
                      const QString &messageFile,
                      const QStringList &checkedFiles,
                      const QStringList &origCommitFiles,
                      const QStringList &origDeletedFiles);

    enum StatusResult { StatusChanged, StatusUnchanged, StatusFailed };
    StatusResult gitStatus(const QString &workingDirectory,
                           bool untracked = false,
                           QString *output = 0,
                           QString *errorMessage = 0);

    GitSettings  settings() const;
    void setSettings(const GitSettings &s);

    QStringList binary() const; // Executable + basic arguments
    QStringList processEnvironment() const;

    static QString msgNoChangedFiles();

    static const char *noColorOption;

public slots:
    void show(const QString &source, const QString &id);

private slots:
    void slotBlameRevisionRequested(const QString &source, QString change, int lineNumber);

private:
    VCSBase::VCSBaseEditor *createVCSEditor(const QString &kind,
                                                 QString title,
                                                 const QString &source,
                                                 bool setSourceCodec,
                                                 const char *registerDynamicProperty,
                                                 const QString &dynamicPropertyValue) const;

    GitCommand *createCommand(const QString &workingDirectory,
                             VCSBase::VCSBaseEditor* editor = 0,
                             bool outputToWindow = false,
                             int editorLineNumber = -1);

    GitCommand *executeGit(const QString &workingDirectory,
                           const QStringList &arguments,
                           VCSBase::VCSBaseEditor* editor = 0,
                           bool outputToWindow = false,
                           GitCommand::TerminationReportMode tm = GitCommand::NoReport,
                           int editorLineNumber = -1);

    bool synchronousGit(const QString &workingDirectory,
                        const QStringList &arguments,
                        QByteArray* outputText = 0,
                        QByteArray* errorText = 0,
                        bool logCommandToWindow = true);

    enum RevertResult { RevertOk, RevertUnchanged, RevertCanceled, RevertFailed };
    RevertResult revertI(QStringList files, bool *isDirectory, QString *errorMessage);
    void connectRepositoryChanged(const QString & repository, GitCommand *cmd);

    const QString m_msgWait;
    GitPlugin     *m_plugin;
    Core::ICore   *m_core;
    GitSettings   m_settings;
    QString m_binaryPath;
    QSignalMapper *m_repositoryChangedSignalMapper;
};


} // namespace Internal
} // namespace Git

#endif // GITCLIENT_H
