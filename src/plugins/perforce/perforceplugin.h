/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PERFORCEPLUGIN_H
#define PERFORCEPLUGIN_H

#include "perforcesettings.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/iversioncontrol.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QSharedPointer>
#include <QHash>

QT_BEGIN_NAMESPACE
class QFile;
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {
    class ParameterAction;
    class TempFileSaver;
}

namespace Locator {
    class CommandLocator;
}

namespace Perforce {
namespace Internal {
struct PerforceDiffParameters;
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

class PerforcePlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Perforce.json")

public:
    PerforcePlugin();
    ~PerforcePlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

    bool managesDirectory(const QString &directory, QString *topLevel = 0);
    bool vcsOpen(const QString &workingDir, const QString &fileName);
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &filename);
    bool vcsMove(const QString &workingDir, const QString &from, const QString &to);

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
    void p4Diff(const Perforce::Internal::PerforceDiffParameters &p);

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

#ifdef WITH_TESTS
    void testLogResolving();
#endif
protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor);


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
    QSharedPointer<Utils::TempFileSaver> createTemporaryArgumentFile(const QStringList &extraArgs,
                                                                     QString *errorString) const;
    void getTopLevel();
    QString pendingChangesData();

    void updateCheckout(const QString &workingDir = QString(),
                        const QStringList &dirs = QStringList());
    bool revertProject(const QString &workingDir, const QStringList &args, bool unchangedOnly);
    bool managesDirectoryFstat(const QString &directory);

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
