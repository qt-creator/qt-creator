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

#ifndef CVSPLUGIN_H
#define CVSPLUGIN_H

#include "cvssettings.h"
#include "cvsutils.h"

#include <vcsbase/vcsbaseplugin.h>

QT_BEGIN_NAMESPACE
class QDir;
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Core {
class IEditorFactory;
class IVersionControl;
}

namespace Utils { class ParameterAction; }
namespace VcsBase { class VcsBaseSubmitEditor; }
namespace Locator { class CommandLocator; }

namespace Cvs {
namespace Internal {

struct CvsDiffParameters;
class CvsSubmitEditor;
class CvsControl;

struct CvsResponse
{
    enum Result { Ok, NonNullExitCode, OtherError };
    CvsResponse() : result(Ok) {}

    Result result;
    QString stdOut;
    QString stdErr;
    QString message;
};

class CvsPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CVS.json")

public:
    CvsPlugin();
    ~CvsPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    void cvsDiff(const QString &workingDir, const QStringList &files);

    CvsSubmitEditor *openCVSSubmitEditor(const QString &fileName);

    CvsSettings settings() const;
    void setSettings(const CvsSettings &s);

    // IVersionControl
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName);
    bool managesDirectory(const QString &directory, QString *topLevel = 0) const;
    // cvs 'edit' is used to implement 'open' (cvsnt).
    bool edit(const QString &topLevel, const QStringList &files);

    static CvsPlugin *instance();

public slots:
    void vcsAnnotate(const QString &file, const QString &revision /* = QString() */, int lineNumber);

private slots:
    void addCurrentFile();
    void revertCurrentFile();
    void diffProject();
    void diffCurrentFile();
    void revertAll();
    void startCommitAll();
    void startCommitCurrentFile();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void projectStatus();
    void slotDescribe(const QString &source, const QString &changeNr);
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
    void cvsDiff(const Cvs::Internal::CvsDiffParameters &p);
#ifdef WITH_TESTS
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
#endif

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor);

private:
    bool isCommitEditorOpen() const;
    Core::IEditor *showOutputInEditor(const QString& title, const QString &output,
                                      int editorType, const QString &source,
                                      QTextCodec *codec);

    CvsResponse runCvs(const QString &workingDirectory,
                       const QStringList &arguments,
                       int timeOut,
                       unsigned flags, QTextCodec *outputCodec = 0);

    void annotate(const QString &workingDir, const QString &file,
                  const QString &revision = QString(), int lineNumber= -1);
    bool describe(const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const QString &toplevel, const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const QString &repository, QList<CvsLogEntry> entries, QString *errorMessage);
    void filelog(const QString &workingDir,
                 const QStringList &files = QStringList(),
                 bool enableAnnotationContextMenu = false);
    bool unedit(const QString &topLevel, const QStringList &files);
    bool status(const QString &topLevel, const QStringList &files, const QString &title);
    bool update(const QString &topLevel, const QStringList &files);
    bool checkCVSDirectory(const QDir &directory) const;
    // Quick check if files are modified
    bool diffCheckModified(const QString &topLevel, const QStringList &files, bool *modified);
    QString findTopLevelForDirectoryI(const QString &directory) const;
    void startCommit(const QString &workingDir, const QStringList &files = QStringList());
    bool commit(const QString &messageFile, const QStringList &subVersionFileList);
    void cleanCommitMessageFile();
    inline CvsControl *cvsVersionControl() const;

    CvsSettings m_settings;
    QString m_commitMessageFileName;
    QString m_commitRepository;

    Locator::CommandLocator *m_commandLocator;
    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    Utils::ParameterAction *m_revertAction;
    Utils::ParameterAction *m_editCurrentAction;
    Utils::ParameterAction *m_uneditCurrentAction;
    QAction *m_uneditRepositoryAction;
    Utils::ParameterAction *m_diffProjectAction;
    Utils::ParameterAction *m_diffCurrentAction;
    Utils::ParameterAction *m_logProjectAction;
    QAction *m_logRepositoryAction;
    QAction *m_commitAllAction;
    QAction *m_revertRepositoryAction;
    Utils::ParameterAction *m_commitCurrentAction;
    Utils::ParameterAction *m_filelogCurrentAction;
    Utils::ParameterAction *m_annotateCurrentAction;
    Utils::ParameterAction *m_statusProjectAction;
    Utils::ParameterAction *m_updateProjectAction;
    Utils::ParameterAction *m_commitProjectAction;
    QAction *m_diffRepositoryAction;
    QAction *m_updateRepositoryAction;
    QAction *m_statusRepositoryAction;

    QAction *m_submitCurrentLogAction;
    QAction *m_submitDiffAction;
    QAction *m_submitUndoAction;
    QAction *m_submitRedoAction;
    QAction *m_menuAction;
    bool    m_submitActionTriggered;

    static CvsPlugin *m_cvsPluginInstance;
};

} // namespace Cvs
} // namespace Internal

#endif // CVSPLUGIN_H
