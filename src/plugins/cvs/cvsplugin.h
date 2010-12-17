/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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

namespace Utils {
    class ParameterAction;
}

namespace VCSBase {
    class VCSBaseSubmitEditor;
}

namespace Locator {
    class CommandLocator;
}

namespace CVS {
namespace Internal {

class CVSSubmitEditor;
class CVSControl;

struct CVSResponse
{
    enum Result { Ok, NonNullExitCode, OtherError };
    CVSResponse() : result(Ok) {}

    Result result;
    QString stdOut;
    QString stdErr;
    QString message;
};

class CVSPlugin : public VCSBase::VCSBasePlugin
{
    Q_OBJECT

public:
    CVSPlugin();
    ~CVSPlugin();

    virtual bool initialize(const QStringList &arguments, QString *error_message);

    void cvsDiff(const QString &workingDir, const QStringList &files);

    CVSSubmitEditor *openCVSSubmitEditor(const QString &fileName);

    CVSSettings settings() const;
    void setSettings(const CVSSettings &s);

    // IVersionControl
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName);
    bool managesDirectory(const QString &directory, QString *topLevel = 0) const;
    // cvs 'edit' is used to implement 'open' (cvsnt).
    bool edit(const QString &topLevel, const QStringList &files);

    static CVSPlugin *cvsPluginInstance();

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

protected:
    virtual void updateActions(VCSBase::VCSBasePlugin::ActionState);
    virtual bool submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor);

private:
    bool isCommitEditorOpen() const;
    Core::IEditor * showOutputInEditor(const QString& title, const QString &output,
                                       int editorType, const QString &source,
                                       QTextCodec *codec);

    CVSResponse runCVS(const QString &workingDirectory,
                       const QStringList &arguments,
                       int timeOut,
                       unsigned flags, QTextCodec *outputCodec = 0);

    void annotate(const QString &workingDir, const QString &file,
                  const QString &revision = QString(), int lineNumber= -1);
    bool describe(const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const QString &toplevel, const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const QString &repository, QList<CVS_LogEntry> entries, QString *errorMessage);
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
    inline CVSControl *cvsVersionControl() const;

    CVSSettings m_settings;
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

    static CVSPlugin *m_cvsPluginInstance;
};

} // namespace CVS
} // namespace Internal

#endif // CVSPLUGIN_H
