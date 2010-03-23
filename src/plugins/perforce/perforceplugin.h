/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PERFORCEPLUGIN_H
#define PERFORCEPLUGIN_H

#include "perforcesettings.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/iversioncontrol.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QTextCodec;
class QTemporaryFile;
QT_END_NAMESPACE

namespace Utils {
    class ParameterAction;
}

namespace Locator {
    class CommandLocator;
}

namespace Perforce {
namespace Internal {

class PerforceVersionControl;

struct PerforceResponse
{
    PerforceResponse();

    bool error;
    int exitCode;
    QString stdOut;
    QString stdErr;
    QString message;
};

class PerforcePlugin : public VCSBase::VCSBasePlugin
{
    Q_OBJECT

public:
    PerforcePlugin();
    ~PerforcePlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

    bool managesDirectory(const QString &directory);
    QString findTopLevelForDirectory(const QString &directory);
    bool vcsOpen(const QString &workingDir, const QString &fileName);
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &filename);

    void p4Diff(const QString &workingDir, const QStringList &files);

    Core::IEditor *openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames);

    static PerforcePlugin *perforcePluginInstance();

    const PerforceSettings& settings() const;
    void setSettings(const Settings &s);

    // Map a perforce name "//xx" to its real name in the file system
    QString fileNameFromPerforceName(const QString& perforceName,
                                     bool quiet,
                                     QString *errorMessage) const;

public slots:
    void describe(const QString &source, const QString &n);
    void vcsAnnotate(const QString &file, const QString &revision /* = QString() */, int lineNumber);

private slots:
    void openCurrentFile();
    void addCurrentFile();
    void revertCurrentFile();
    void printOpenedFileList();
    void diffCurrentFile();
    void diffCurrentProject();
    void updateCurrentProject();
    void revertCurrentProject();
    void revertUnchangedCurrentProject();
    void updateAll();
    void diffAllOpened();
    void startSubmitProject();
    void describeChange();
    void annotateCurrentFile();
    void annotate();
    void filelogCurrentFile();
    void filelog();
    void logProject();
    void logRepository();

    void submitCurrentLog();
    void printPendingChanges();
    void slotSubmitDiff(const QStringList &files);
    void slotTopLevelFound(const QString &);
    void slotTopLevelFailed(const QString &);

protected:
    virtual void updateActions(VCSBase::VCSBasePlugin::ActionState);
    virtual bool submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor);


private:
    typedef QHash<QString, bool> ManagedDirectoryCache;

    Core::IEditor *showOutputInEditor(const QString& title, const QString output,
                                      int editorType, const QString &source,
                                      QTextCodec *codec = 0);

    // Flags for runP4Cmd.
    enum RunFlags { CommandToWindow = 0x1, StdOutToWindow = 0x2,
                    StdErrToWindow = 0x4, ErrorToWindow = 0x8,
                    OverrideDiffEnvironment = 0x10,
                    // Run completely synchronously, no signals emitted
                    RunFullySynchronous = 0x20,
                    IgnoreExitCode = 0x40,
                    ShowBusyCursor = 0x80,
                    LongTimeOut = 0x100
                   };

    // args are passed as command line arguments
    // extra args via a tempfile and the option -x "temp-filename"
    PerforceResponse runP4Cmd(const QString &workingDir,
                              const QStringList &args,
                              unsigned flags = CommandToWindow|StdErrToWindow|ErrorToWindow,
                              const QStringList &extraArgs = QStringList(),
                              const QByteArray &stdInput = QByteArray(),
                              QTextCodec *outputCodec = 0) const;

    inline PerforceResponse synchronousProcess(const QString &workingDir,
                                               const QStringList &args,
                                               unsigned flags,
                                               const QByteArray &stdInput,
                                               QTextCodec *outputCodec) const;

    inline PerforceResponse fullySynchronousProcess(const QString &workingDir,
                                                    const QStringList &args,
                                                    unsigned flags,
                                                    const QByteArray &stdInput,
                                                    QTextCodec *outputCodec) const;

    QString clientFilePath(const QString &serverFilePath);
    void annotate(const QString &workingDir, const QString &fileName,
                  const QString &changeList = QString(), int lineNumber = -1);
    void filelog(const QString &workingDir, const QStringList &fileNames = QStringList(),
                 bool enableAnnotationContextMenu = false);
    void cleanCommitMessageFile();
    bool isCommitEditorOpen() const;
    QSharedPointer<QTemporaryFile> createTemporaryArgumentFile(const QStringList &extraArgs) const;
    void getTopLevel();
    QString pendingChangesData();

    void updateCheckout(const QString &workingDir = QString(),
                        const QStringList &dirs = QStringList());
    bool revertProject(const QString &workingDir, const QStringList &args, bool unchangedOnly);

    inline PerforceVersionControl *perforceVersionControl() const;

    Locator::CommandLocator *m_commandLocator;
    Utils::ParameterAction *m_editAction;
    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    QAction *m_openedAction;
    Utils::ParameterAction *m_revertFileAction;
    Utils::ParameterAction *m_diffFileAction;
    Utils::ParameterAction *m_diffProjectAction;
    Utils::ParameterAction *m_updateProjectAction;
    Utils::ParameterAction *m_revertProjectAction;
    Utils::ParameterAction *m_revertUnchangedAction;
    QAction *m_diffAllAction;
    Utils::ParameterAction *m_submitProjectAction;
    QAction *m_pendingAction;
    QAction *m_describeAction;
    Utils::ParameterAction *m_annotateCurrentAction;
    QAction *m_annotateAction;
    Utils::ParameterAction *m_filelogCurrentAction;
    QAction *m_filelogAction;
    Utils::ParameterAction *m_logProjectAction;
    QAction *m_logRepositoryAction;
    QAction *m_submitCurrentLogAction;
    QAction *m_updateAllAction;
    bool m_submitActionTriggered;
    QAction *m_diffSelectedFiles;
    QString m_commitMessageFileName;
    QString m_commitWorkingDirectory;
    mutable QString m_tempFilePattern;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_menuAction;

    static PerforcePlugin *m_perforcePluginInstance;

    PerforceSettings m_settings;
    ManagedDirectoryCache m_managedDirectoryCache;
};

} // namespace Perforce
} // namespace Internal

#endif // PERFORCEPLUGIN_H
