/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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
template <typename T>
class QFutureInterface;
class QMutex;
using QStringPair = QPair<QString, QString>;
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

class ClearCaseResponse
{
public:
    bool error = false;
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

    FileStatus(Status _status = Unknown, QFile::Permissions perm = {})
        : status(_status), permissions(perm)
    { }
};

using StatusMap = QHash<QString, FileStatus>;

class ViewData
{
public:
    QString name;
    bool isDynamic = false;
    bool isUcm = false;
    QString root;
};

class ClearCasePluginPrivate final : public VcsBase::VcsBasePluginPrivate
{
    Q_OBJECT
    enum { SilentRun = VcsBase::VcsCommand::NoOutput | VcsBase::VcsCommand::FullySynchronously };

public:
    ClearCasePluginPrivate();
    ~ClearCasePluginPrivate() final;

    // IVersionControl
    QString displayName() const final;
    Core::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FilePath &fileName) const final;

    bool managesDirectory(const QString &directory, QString *topLevel) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;

    bool supportsOperation(Operation operation) const final;
    OpenSupportMode openSupportMode(const QString &fileName) const final;
    bool vcsOpen(const QString &fileName) final;
    SettingsFlags settingsFlags() const final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;

    bool vcsAnnotate(const QString &file, int line) final;

    QString vcsOpenText() const final;
    QString vcsMakeWritableText() const final;
    QString vcsTopic(const QString &directory) final;

    ///
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

    static ClearCasePluginPrivate *instance();

    QString ccGetCurrentActivity() const;
    QList<QStringPair> activities(int *current = nullptr) const;
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
#ifdef WITH_TESTS
    inline void setFakeCleartool(const bool b = true) { m_fakeClearTool = b; }
#endif

    void vcsAnnotateHelper(const QString &workingDir, const QString &file,
                           const QString &revision = QString(), int lineNumber = -1) const;
    bool newActivity();
    void updateStreamAndView();
    void describe(const QString &source, const QString &changeNr);

protected:
    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) override;
    bool submitEditorAboutToClose() override;
    QString ccGet(const QString &workingDir, const QString &file, const QString &prefix = QString());
    QList<QStringPair> ccGetActivities() const;

private:
    void syncSlot();
    Q_INVOKABLE void updateStatusActions();

    QString commitDisplayName() const final;
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
    void commitFromEditor() override;
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
                                   int timeOutS, unsigned flags,
                                   QTextCodec *outputCodec = nullptr) const;
    static void sync(QFutureInterface<void> &future, QStringList files);

    void history(const QString &workingDir,
                 const QStringList &file = QStringList(),
                 bool enableAnnotationContextMenu = false);
    QString ccGetFileVersion(const QString &workingDir, const QString &file) const;
    void ccUpdate(const QString &workingDir, const QStringList &relativePaths = QStringList());
    void ccDiffWithPred(const QString &workingDir, const QStringList &files);
    void startCheckIn(const QString &workingDir, const QStringList &files = QStringList());
    void cleanCheckInMessageFile();
    QString ccGetFileActivity(const QString &workingDir, const QString &file);
    QStringList ccGetActivityVersions(const QString &workingDir, const QString &activity);
    void diffGraphical(const QString &file1, const QString &file2 = QString());
    QString diffExternal(QString file1, QString file2 = QString(), bool keep = false);
    QString getFile(const QString &nativeFile, const QString &prefix);
    static void rmdir(const QString &path);
    QString runExtDiff(const QString &workingDir, const QStringList &arguments, int timeOutS,
                       QTextCodec *outputCodec = nullptr);
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

    Core::CommandLocator *m_commandLocator = nullptr;
    Utils::ParameterAction *m_checkOutAction = nullptr;
    Utils::ParameterAction *m_checkInCurrentAction = nullptr;
    Utils::ParameterAction *m_undoCheckOutAction = nullptr;
    Utils::ParameterAction *m_undoHijackAction = nullptr;
    Utils::ParameterAction *m_diffCurrentAction = nullptr;
    Utils::ParameterAction *m_historyCurrentAction = nullptr;
    Utils::ParameterAction *m_annotateCurrentAction = nullptr;
    Utils::ParameterAction *m_addFileAction = nullptr;
    QAction *m_diffActivityAction = nullptr;
    QAction *m_updateIndexAction = nullptr;
    Utils::ParameterAction *m_updateViewAction = nullptr;
    Utils::ParameterAction *m_checkInActivityAction = nullptr;
    QAction *m_checkInAllAction = nullptr;
    QAction *m_statusAction = nullptr;

    QAction *m_menuAction = nullptr;
    bool m_submitActionTriggered = false;
    QMutex *m_activityMutex;
    QList<QStringPair> m_activities;
    QSharedPointer<StatusMap> m_statusMap;

    friend class ClearCasePlugin;
#ifdef WITH_TESTS
    bool m_fakeClearTool = false;
    QString m_tempFile;
#endif
};

class ClearCasePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClearCase.json")

    ~ClearCasePlugin() final;

    bool initialize(const QStringList &arguments, QString *error_message) final;
    void extensionsInitialized() final;

#ifdef WITH_TESTS
private slots:
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
};

} // namespace Internal
} // namespace ClearCase
