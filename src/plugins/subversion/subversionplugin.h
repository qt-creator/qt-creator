/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef SUBVERSIONPLUGIN_H
#define SUBVERSIONPLUGIN_H

#include "subversionsettings.h"

#include <vcsbase/vcsbaseplugin.h>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QDir;
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Core {
    class CommandLocator;
    class IEditor;
}
namespace Utils { class ParameterAction; }

namespace Subversion {
namespace Internal {

class SubversionSubmitEditor;
class SubversionControl;
class SubversionClient;

struct SubversionResponse
{
    SubversionResponse() : error(false) {}
    bool error;
    QString stdOut;
    QString stdErr;
    QString message;
};

class SubversionPlugin : public VcsBase::VcsBasePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Subversion.json")

public:
    SubversionPlugin();
    ~SubversionPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    SubversionSubmitEditor *openSubversionSubmitEditor(const QString &fileName);

    SubversionSettings settings() const;
    void setSettings(const SubversionSettings &s);

    // IVersionControl
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName);
    bool vcsMove(const QString &workingDir, const QString &from, const QString &to);
    bool managesDirectory(const QString &directory, QString *topLevel = 0) const;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const;
    bool vcsCheckout(const QString &directory, const QByteArray &url);

    static SubversionPlugin *instance();

    QString monitorFile(const QString &repository) const;
    QString synchronousTopic(const QString &repository) const;
    SubversionResponse runSvn(const QString &workingDir,
                              const QStringList &arguments, int timeOut,
                              unsigned flags, QTextCodec *outputCodec = 0) const;

public slots:
    void vcsAnnotate(const QString &workingDir, const QString &file,
                     const QString &revision = QString(), int lineNumber = -1);

private slots:
    void addCurrentFile();
    void revertCurrentFile();
    void diffProject();
    void diffCurrentFile();
    void cleanCommitMessageFile();
    void startCommitAll();
    void startCommitProject();
    void startCommitCurrentFile();
    void revertAll();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void annotateVersion(const QString &workingDirectory, const QString &file,
                         const QString &revision, int lineNumber);
    void projectStatus();
    void describe(const QString &source, const QString &changeNr);
    void slotDescribe();
    void updateProject();
    void submitCurrentLog();
    void diffCommitFiles(const QStringList &);
    void logProject();
    void logRepository();
    void diffRepository();
    void statusRepository();
    void updateRepository();
#ifdef WITH_TESTS
    void testLogResolving();
#endif

protected:
    void updateActions(VcsBase::VcsBasePlugin::ActionState);
    bool submitEditorAboutToClose();

private:
    inline bool isCommitEditorOpen() const;
    Core::IEditor *showOutputInEditor(const QString &title, const QString &output,
                                      int editorType, const QString &source,
                                      QTextCodec *codec);

    void filelog(const QString &workingDir,
                 const QString &file = QString(),
                 bool enableAnnotationContextMenu = false);
    void svnStatus(const QString &workingDir, const QString &relativePath = QString());
    void svnUpdate(const QString &workingDir, const QString &relativePath = QString());
    bool checkSVNSubDir(const QDir &directory) const;
    void startCommit(const QString &workingDir, const QStringList &files = QStringList());
    inline SubversionControl *subVersionControl() const;

    const QStringList m_svnDirectories;

    SubversionSettings m_settings;
    SubversionClient *m_client;
    QString m_commitMessageFileName;
    QString m_commitRepository;

    Core::CommandLocator *m_commandLocator;
    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    Utils::ParameterAction *m_revertAction;
    Utils::ParameterAction *m_diffProjectAction;
    Utils::ParameterAction *m_diffCurrentAction;
    Utils::ParameterAction *m_logProjectAction;
    QAction *m_logRepositoryAction;
    QAction *m_commitAllAction;
    QAction *m_revertRepositoryAction;
    QAction *m_diffRepositoryAction;
    QAction *m_statusRepositoryAction;
    QAction *m_updateRepositoryAction;
    Utils::ParameterAction *m_commitCurrentAction;
    Utils::ParameterAction *m_filelogCurrentAction;
    Utils::ParameterAction *m_annotateCurrentAction;
    Utils::ParameterAction *m_statusProjectAction;
    Utils::ParameterAction *m_updateProjectAction;
    Utils::ParameterAction *m_commitProjectAction;
    QAction *m_describeAction;

    QAction *m_submitCurrentLogAction;
    QAction *m_submitDiffAction;
    QAction *m_submitUndoAction;
    QAction *m_submitRedoAction;
    QAction *m_menuAction;
    bool m_submitActionTriggered;

    static SubversionPlugin *m_subversionPluginInstance;
};

} // namespace Subversion
} // namespace Internal

#endif // SUBVERSIONPLUGIN_H
