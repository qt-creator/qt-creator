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

#include "cvssettings.h"
#include "cvsutils.h"

#include <vcsbase/vcsbaseplugin.h>

QT_BEGIN_NAMESPACE
class QDir;
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Core {
class CommandLocator;
class IVersionControl;
} // namespace Core

namespace Utils { class ParameterAction; }
namespace VcsBase { class VcsBaseSubmitEditor; }

namespace Cvs {
namespace Internal {

class CvsSubmitEditor;
class CvsControl;
class CvsClient;

class CvsResponse
{
public:
    enum Result { Ok, NonNullExitCode, OtherError };

    Result result = Ok;
    QString stdOut;
    QString stdErr;
    QString message;
};

class CvsPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CVS.json")

public:
    ~CvsPlugin() override;

    CvsClient *client() const;

    bool initialize(const QStringList &arguments, QString *errorMessage) override;

    CvsSubmitEditor *openCVSSubmitEditor(const QString &fileName);

    // IVersionControl
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName);
    bool managesDirectory(const QString &directory, QString *topLevel = 0) const;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const;
    // cvs 'edit' is used to implement 'open' (cvsnt).
    bool edit(const QString &topLevel, const QStringList &files);

    static CvsPlugin *instance();

    void vcsAnnotate(const QString &workingDirectory, const QString &file,
                     const QString &revision, int lineNumber);

#ifdef WITH_TESTS
private slots:
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState) override;
    bool submitEditorAboutToClose() override;

private:
    void addCurrentFile();
    void revertCurrentFile();
    void diffProject();
    void diffCurrentFile();
    void revertAll();
    void startCommitAll();
    void startCommitDirectory();
    void startCommitCurrentFile();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void projectStatus();
    void updateDirectory();
    void updateProject();
    void submitCurrentLog();
    void diffCommitFiles(const QStringList &);
    void logProject();
    void logRepository();
    void commitProject();
    void diffRepository();
    void statusRepository();
    void updateRepository();
    void editCurrentFile();
    void uneditCurrentFile();
    void uneditCurrentRepository();

    bool isCommitEditorOpen() const;
    Core::IEditor *showOutputInEditor(const QString& title, const QString &output,
                                      int editorType, const QString &source,
                                      QTextCodec *codec);

    CvsResponse runCvs(const QString &workingDirectory,
                       const QStringList &arguments,
                       int timeOutS,
                       unsigned flags,
                       QTextCodec *outputCodec = 0) const;

    void annotate(const QString &workingDir, const QString &file,
                  const QString &revision = QString(), int lineNumber= -1);
    bool describe(const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const QString &toplevel, const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const QString &repository, QList<CvsLogEntry> entries, QString *errorMessage);
    void filelog(const QString &workingDir,
                 const QString &file = QString(),
                 bool enableAnnotationContextMenu = false);
    bool unedit(const QString &topLevel, const QStringList &files);
    bool status(const QString &topLevel, const QString &file, const QString &title);
    bool update(const QString &topLevel, const QString &file);
    bool checkCVSDirectory(const QDir &directory) const;
    // Quick check if files are modified
    bool diffCheckModified(const QString &topLevel, const QStringList &files, bool *modified);
    QString findTopLevelForDirectoryI(const QString &directory) const;
    void startCommit(const QString &workingDir, const QString &file = QString());
    bool commit(const QString &messageFile, const QStringList &subVersionFileList);
    void cleanCommitMessageFile();
    inline CvsControl *cvsVersionControl() const;

    CvsSettings m_settings;
    CvsClient *m_client = nullptr;

    QString m_commitMessageFileName;
    QString m_commitRepository;

    Core::CommandLocator *m_commandLocator = nullptr;
    Utils::ParameterAction *m_addAction = nullptr;
    Utils::ParameterAction *m_deleteAction = nullptr;
    Utils::ParameterAction *m_revertAction = nullptr;
    Utils::ParameterAction *m_editCurrentAction = nullptr;
    Utils::ParameterAction *m_uneditCurrentAction = nullptr;
    QAction *m_uneditRepositoryAction = nullptr;
    Utils::ParameterAction *m_diffProjectAction = nullptr;
    Utils::ParameterAction *m_diffCurrentAction = nullptr;
    Utils::ParameterAction *m_logProjectAction = nullptr;
    QAction *m_logRepositoryAction = nullptr;
    QAction *m_commitAllAction = nullptr;
    QAction *m_revertRepositoryAction = nullptr;
    Utils::ParameterAction *m_commitCurrentAction = nullptr;
    Utils::ParameterAction *m_filelogCurrentAction = nullptr;
    Utils::ParameterAction *m_annotateCurrentAction = nullptr;
    Utils::ParameterAction *m_statusProjectAction = nullptr;
    Utils::ParameterAction *m_updateProjectAction = nullptr;
    Utils::ParameterAction *m_commitProjectAction = nullptr;
    Utils::ParameterAction *m_updateDirectoryAction = nullptr;
    Utils::ParameterAction *m_commitDirectoryAction = nullptr;
    QAction *m_diffRepositoryAction = nullptr;
    QAction *m_updateRepositoryAction = nullptr;
    QAction *m_statusRepositoryAction = nullptr;

    QAction *m_submitCurrentLogAction = nullptr;
    QAction *m_submitDiffAction = nullptr;
    QAction *m_submitUndoAction = nullptr;
    QAction *m_submitRedoAction = nullptr;
    QAction *m_menuAction = nullptr;
    bool m_submitActionTriggered = false;

    static CvsPlugin *m_cvsPluginInstance;
};

} // namespace Cvs
} // namespace Internal
