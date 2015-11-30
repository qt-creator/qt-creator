/**************************************************************************
**
** Copyright (C) 2015 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLEARCASEPLUGIN_H
#define CLEARCASEPLUGIN_H

#include "clearcasesettings.h"

#include <coreplugin/id.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>

#include <QFile>
#include <QPair>
#include <QStringList>
#include <QMetaType>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QDir;
template <typename T>
class QFutureInterface;
class QMutex;
typedef QPair<QString, QString> QStringPair;
class QTextCodec;
QT_END_NAMESPACE

namespace Core {
class CommandLocator;
class IVersionControl;
class IEditor;
} // namespace Core

namespace Utils { class ParameterAction; }
namespace VcsBase { class VcsBaseSubmitEditor; }
namespace ProjectExplorer { class Project; }

namespace ClearCase {
namespace Internal {

class ClearCaseSubmitEditor;
class ClearCaseControl;

class ClearCaseResponse
{
public:
    ClearCaseResponse() : error(false) {}
    bool error;
    QString stdOut;
    QString stdErr;
    QString message;
};

class FileStatus
{
public:
    enum Status
    {
        Unknown    = 0x0f,
        CheckedIn  = 0x01,
        CheckedOut = 0x02,
        Hijacked   = 0x04,
        NotManaged = 0x08,
        Missing    = 0x10,
        Derived    = 0x20
    } status;

    QFile::Permissions permissions;

    FileStatus(Status _status = Unknown, QFile::Permissions perm = 0)
        : status(_status), permissions(perm)
    {}
};

typedef QHash<QString, FileStatus> StatusMap;

class ViewData
{
public:
    ViewData();

    QString name;
    bool isDynamic;
    bool isUcm;
    QString root;
};

class ClearCasePlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClearCase.json")

    enum { SilentRun = VcsBase::VcsCommand::NoOutput | VcsBase::VcsCommand::FullySynchronously };

public:
    ClearCasePlugin();
    ~ClearCasePlugin();

    bool initialize(const QStringList &arguments, QString *error_message);

    ClearCaseSubmitEditor *openClearCaseSubmitEditor(const QString &fileName, bool isUcm);

    const ClearCaseSettings &settings() const;
    void setSettings(const ClearCaseSettings &s);

    // IVersionControl
    bool vcsOpen(const QString &workingDir, const QString &fileName);
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName);
    bool vcsCheckIn(const QString &workingDir, const QStringList &files, const QString &activity,
                    bool isIdentical, bool isPreserve, bool replaceActivity);
    bool vcsUndoCheckOut(const QString &workingDir, const QString &fileName, bool keep);
    bool vcsUndoHijack(const QString &workingDir, const QString &fileName, bool keep);
    bool vcsMove(const QString &workingDir, const QString &from, const QString &to);
    bool vcsSetActivity(const QString &workingDir, const QString &title, const QString &activity);
    bool managesDirectory(const QString &directory, QString *topLevel = 0) const;
    bool vcsCheckout(const QString &directory, const QByteArray &url);

    static ClearCasePlugin *instance();

    QString ccGetCurrentActivity() const;
    QList<QStringPair> activities(int *current = 0) const;
    QString ccGetPredecessor(const QString &version) const;
    QStringList ccGetActiveVobs() const;
    ViewData ccGetView(const QString &workingDir) const;
    QString ccGetComment(const QString &workingDir, const QString &fileName) const;
    bool ccFileOp(const QString &workingDir, const QString &title, const QStringList &args,
                  const QString &fileName, const QString &file2 = QString());
    FileStatus vcsStatus(const QString &file) const;
    void checkAndReIndexUnknownFile(const QString &file);
    QString currentView() const { return m_viewData.name; }
    QString viewRoot() const { return m_viewData.root; }
    void refreshActivities();
    inline bool isUcm() const { return m_viewData.isUcm; }
    inline bool isDynamic() const { return m_viewData.isDynamic; }
    void setStatus(const QString &file, FileStatus::Status status, bool update = true);

    bool ccCheckUcm(const QString &viewname, const QString &workingDir) const;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const;
#ifdef WITH_TESTS
    inline void setFakeCleartool(const bool b = true) { m_fakeClearTool = b; }
    inline bool isFakeCleartool() const { return m_fakeClearTool; }
#endif

public slots:
    void vcsAnnotate(const QString &workingDir, const QString &file,
                     const QString &revision = QString(), int lineNumber = -1) const;
    bool newActivity();
    void updateStreamAndView();

private slots:
    void annotateVersion(const QString &workingDirectory, const QString &file, const QString &revision, int lineNumber);
    void describe(const QString &source, const QString &changeNr);
    void syncSlot();
    void updateStatusActions();

#ifdef WITH_TESTS
    void initTestCase();
    void cleanupTestCase();
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
    void testFileStatusParsing_data();
    void testFileStatusParsing();
    void testFileNotManaged();
    void testFileCheckedOutDynamicView();
    void testFileCheckedInDynamicView();
    void testFileNotManagedDynamicView();
    void testStatusActions_data();
    void testStatusActions();
    void testVcsStatusDynamicReadonlyNotManaged();
    void testVcsStatusDynamicNotManaged();
#endif

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose();
    QString ccGet(const QString &workingDir, const QString &file, const QString &prefix = QString());
    QList<QStringPair> ccGetActivities() const;

private:
    void checkOutCurrentFile();
    void addCurrentFile();
    void undoCheckOutCurrent();
    void undoHijackCurrent();
    void diffActivity();
    void diffCurrentFile();
    void startCheckInAll();
    void startCheckInActivity();
    void startCheckInCurrentFile();
    void historyCurrentFile();
    void annotateCurrentFile();
    void viewStatus();
    void checkInSelected();
    void diffCheckInFiles(const QStringList &);
    void updateIndex();
    void updateView();
    void projectChanged(ProjectExplorer::Project *project);
    void tasksFinished(Core::Id type);
    void closing();

    inline bool isCheckInEditorOpen() const;
    QStringList getVobList() const;
    QString ccManagesDirectory(const QString &directory) const;
    QString ccViewRoot(const QString &directory) const;
    QString findTopLevel(const QString &directory) const;
    Core::IEditor *showOutputInEditor(const QString& title, const QString &output,
                                      int editorType, const QString &source,
                                      QTextCodec *codec) const;
    QString runCleartoolSync(const QString &workingDir, const QStringList &arguments) const;
    ClearCaseResponse runCleartool(const QString &workingDir, const QStringList &arguments,
                                   int timeOutS, unsigned flags, QTextCodec *outputCodec = 0) const;
    static void sync(QFutureInterface<void> &future, QStringList files);

    void history(const QString &workingDir,
                 const QStringList &file = QStringList(),
                 bool enableAnnotationContextMenu = false);
    QString ccGetFileVersion(const QString &workingDir, const QString &file) const;
    void ccUpdate(const QString &workingDir, const QStringList &relativePaths = QStringList());
    void ccDiffWithPred(const QString &workingDir, const QStringList &files);
    void startCheckIn(const QString &workingDir, const QStringList &files = QStringList());
    void cleanCheckInMessageFile();
    inline ClearCaseControl *clearCaseControl() const;
    QString ccGetFileActivity(const QString &workingDir, const QString &file);
    QStringList ccGetActivityVersions(const QString &workingDir, const QString &activity);
    void diffGraphical(const QString &file1, const QString &file2 = QString());
    QString diffExternal(QString file1, QString file2 = QString(), bool keep = false);
    QString getFile(const QString &nativeFile, const QString &prefix);
    static void rmdir(const QString &path);
    QString runExtDiff(const QString &workingDir, const QStringList &arguments, int timeOutS,
                       QTextCodec *outputCodec = 0);
    static QString getDriveLetterOfPath(const QString &directory);

    FileStatus::Status getFileStatus(const QString &fileName) const;
    void updateStatusForFile(const QString &absFile);
    void updateEditDerivedObjectWarning(const QString &fileName, const FileStatus::Status status);

    ClearCaseSettings m_settings;

    QString m_checkInMessageFileName;
    QString m_checkInView;
    QString m_topLevel;
    QString m_stream;
    ViewData m_viewData;
    QString m_intStream;
    QString m_activity;
    QString m_diffPrefix;

    Core::CommandLocator *m_commandLocator;
    Utils::ParameterAction *m_checkOutAction;
    Utils::ParameterAction *m_checkInCurrentAction;
    Utils::ParameterAction *m_undoCheckOutAction;
    Utils::ParameterAction *m_undoHijackAction;
    Utils::ParameterAction *m_diffCurrentAction;
    Utils::ParameterAction *m_historyCurrentAction;
    Utils::ParameterAction *m_annotateCurrentAction;
    Utils::ParameterAction *m_addFileAction;
    QAction *m_diffActivityAction;
    QAction *m_updateIndexAction;
    Utils::ParameterAction *m_updateViewAction;
    Utils::ParameterAction *m_checkInActivityAction;
    QAction *m_checkInAllAction;
    QAction *m_statusAction;

    QAction *m_checkInSelectedAction;
    QAction *m_checkInDiffAction;
    QAction *m_submitUndoAction;
    QAction *m_submitRedoAction;
    QAction *m_menuAction;
    bool m_submitActionTriggered;
    QMutex *m_activityMutex;
    QList<QStringPair> m_activities;
    QSharedPointer<StatusMap> m_statusMap;

    static ClearCasePlugin *m_clearcasePluginInstance;
#ifdef WITH_TESTS
    bool m_fakeClearTool;
    QString m_tempFile;
#endif
};

} // namespace Internal
} // namespace ClearCase

#endif // CLEARCASEPLUGIN_H
