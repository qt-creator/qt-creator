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

#ifndef SUBVERSIONPLUGIN_H
#define SUBVERSIONPLUGIN_H

#include "subversionsettings.h"

#include <vcsbase/vcsbaseplugin.h>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QDir;
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Core {
    class IVersionControl;
    class IEditor;
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

namespace Subversion {
namespace Internal {

class SubversionSubmitEditor;
class SubversionControl;

struct SubversionResponse
{
    SubversionResponse() : error(false) {}
    bool error;
    QString stdOut;
    QString stdErr;
    QString message;
};

class SubversionPlugin : public VCSBase::VCSBasePlugin
{
    Q_OBJECT

public:
    SubversionPlugin();
    ~SubversionPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();

    void svnDiff(const QString  &workingDir, const QStringList &files, QString diffname = QString());

    SubversionSubmitEditor *openSubversionSubmitEditor(const QString &fileName);

    SubversionSettings settings() const;
    void setSettings(const SubversionSettings &s);

    // IVersionControl
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsAdd14(const QString &workingDir, const QString &fileName);
    bool vcsAdd15(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName);
    bool managesDirectory(const QString &directory) const;
    QString findTopLevelForDirectory(const QString &directory) const;

    static SubversionPlugin *subversionPluginInstance();

private slots:
    void addCurrentFile();
    void revertCurrentFile();
    void diffProject();
    void diffCurrentFile();
    void startCommitAll();
    void startCommitCurrentFile();
    void revertAll();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void annotateVersion(const QString &file, const QString &revision, int lineNumber);
    void projectStatus();
    void describe(const QString &source, const QString &changeNr);
    void slotDescribe();
    void updateProject();
    void submitCurrentLog();
    void diffCommitFiles(const QStringList &);
    void logProject();
    void logRepository();

protected:
    virtual void updateActions(VCSBase::VCSBasePlugin::ActionState);
    virtual bool submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor);

private:
    inline bool isCommitEditorOpen() const;
    Core::IEditor * showOutputInEditor(const QString& title, const QString &output,
                                       int editorType, const QString &source,
                                       QTextCodec *codec);
    SubversionResponse runSvn(const QString &workingDir,
                              const QStringList &arguments, int timeOut,
                              bool showStdOutInOutputWindow, QTextCodec *outputCodec = 0);
    void annotate(const QString &workingDir, const QString &file,
                  const QString &revision = QString(), int lineNumber = -1);
    void filelog(const QString &workingDir,
                 const QStringList &file = QStringList(),
                 bool enableAnnotationContextMenu = false);
    bool managesDirectory(const QDir &directory) const;
    QString findTopLevelForDirectoryI(const QString &directory) const;
    void startCommit(const QString &workingDir, const QStringList &files = QStringList());
    bool commit(const QString &messageFile, const QStringList &subVersionFileList);
    void cleanCommitMessageFile();
    inline SubversionControl *subVersionControl() const;

    const QStringList m_svnDirectories;

    SubversionSettings m_settings;
    QString m_commitMessageFileName;
    QString m_commitRepository;

    Locator::CommandLocator *m_commandLocator;
    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    Utils::ParameterAction *m_revertAction;
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
    QAction *m_describeAction;

    QAction *m_submitCurrentLogAction;
    QAction *m_submitDiffAction;
    QAction *m_submitUndoAction;
    QAction *m_submitRedoAction;
    QAction *m_menuAction;
    bool    m_submitActionTriggered;

    static SubversionPlugin *m_subversionPluginInstance;
};

} // namespace Subversion
} // namespace Internal

#endif // SUBVERSIONPLUGIN_H
