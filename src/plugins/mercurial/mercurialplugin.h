/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIALPLUGIN_H
#define MERCURIALPLUGIN_H

#include "mercurialsettings.h"

#include <vcsbase/vcsbaseplugin.h>

#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE
class QAction;
class QTemporaryFile;
QT_END_NAMESPACE

namespace Core {
class ActionManager;
class ActionContainer;
class ICore;
class IVersionControl;
class IEditorFactory;
class IEditor;
} // namespace Core

namespace Utils {
class ParameterAction;
} //namespace Utils

namespace VCSBase {
class VCSBaseSubmitEditor;
}

namespace Locator {
    class CommandLocator;
}

namespace Mercurial {
namespace Internal {

class OptionsPage;
class MercurialClient;
class MercurialControl;
class MercurialEditor;
class MercurialSettings;

class MercurialPlugin : public VCSBase::VCSBasePlugin
{
    Q_OBJECT

public:
    MercurialPlugin();
    virtual ~MercurialPlugin();
    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    static MercurialPlugin *instance() { return m_instance; }
    MercurialClient *client() const { return m_client; }

    QStringList standardArguments() const;

    const MercurialSettings &settings() const;
    void setSettings(const MercurialSettings &settings);

private slots:
    // File menu action Slots
    void addCurrentFile();
    void annotateCurrentFile();
    void diffCurrentFile();
    void logCurrentFile();
    void revertCurrentFile();
    void statusCurrentFile();

    //Directory menu Action slots
    void diffRepository();
    void logRepository();
    void revertMulti();
    void statusMulti();

    //repository menu action slots
    void pull();
    void push();
    void update();
    void import();
    void incoming();
    void outgoing();
    void commit();
    void showCommitWidget(const QList<QPair<QString, QString> > &status);
    void commitFromEditor();
    void diffFromEditorSelected(const QStringList &files);

    //TODO implement
   /* //repository management action slots
    void merge();
    void branch();
    void heads();
    void parents();
    void tags();
    void tip();
    void paths();

    //less used repository action
    void init();
    void serve();*/

protected:
    virtual void updateActions(VCSBase::VCSBasePlugin::ActionState);
    virtual bool submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor);

private:
    //methods
    void createMenu();
    void createSubmitEditorActions();
    void createSeparator(const QList<int> &context, const QString &id);
    void createFileActions(const QList<int> &context);
    void createDirectoryActions(const QList<int> &context);
    void createRepositoryActions(const QList<int> &context);
    void createRepositoryManagementActions(const QList<int> &context);
    void createLessUsedActions(const QList<int> &context);
    void deleteCommitLog();

    //Variables
    static MercurialPlugin *m_instance;
    MercurialSettings mercurialSettings;
    OptionsPage *optionsPage;
    MercurialClient *m_client;

    Core::ICore *core;
    Locator::CommandLocator *m_commandLocator;
    Core::ActionManager *actionManager;
    Core::ActionContainer *mercurialContainer;

    QList<QAction *> m_repositoryActionList;
    QTemporaryFile *changeLog;

    //Menu Items (file actions)
    Utils::ParameterAction *m_addAction;
    Utils::ParameterAction *m_deleteAction;
    Utils::ParameterAction *annotateFile;
    Utils::ParameterAction *diffFile;
    Utils::ParameterAction *logFile;
    Utils::ParameterAction *renameFile;
    Utils::ParameterAction *revertFile;
    Utils::ParameterAction *statusFile;

    QAction *m_createRepositoryAction;
    //submit editor actions
    QAction *editorCommit;
    QAction *editorDiff;
    QAction *editorUndo;
    QAction *editorRedo;
    QAction *m_menuAction;

    QString m_submitRepository;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALPLUGIN_H
