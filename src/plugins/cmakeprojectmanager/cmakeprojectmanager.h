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

#ifndef CMAKEPROJECTMANAGER_H
#define CMAKEPROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <coreplugin/icontext.h>
#include <texteditor/codeassist/keywordscompletionassist.h>

#include <utils/environment.h>
#include <utils/pathchooser.h>

#include <QFuture>
#include <QStringList>
#include <QDir>
#include <QVector>
#include <QAction>

#include "cmakevalidator.h"

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace Utils {
class QtcProcess;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeSettingsPage;

class CMakeManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT
public:
    CMakeManager(CMakeSettingsPage *cmakeSettingsPage);

    virtual ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);
    virtual QString mimeType() const;

    QString cmakeExecutable() const;
    bool isCMakeExecutableValid() const;

    void setCMakeExecutable(const QString &executable);

    void createXmlFile(Utils::QtcProcess *process,
                       const QString &arguments,
                       const QString &sourceDirectory,
                       const QDir &buildDirectory,
                       const Utils::Environment &env,
                       const QString &generator);
    bool hasCodeBlocksMsvcGenerator() const;
    bool hasCodeBlocksNinjaGenerator() const;
    static QString findCbpFile(const QDir &);

    static QString findDumperLibrary(const Utils::Environment &env);
private slots:
    void updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node);
    void runCMake();
    void runCMakeContextMenu();
private:
    void runCMake(ProjectExplorer::Project *project);
    static QString qtVersionForQMake(const QString &qmakePath);
    static QPair<QString, QString> findQtDir(const Utils::Environment &env);
    CMakeSettingsPage *m_settingsPage;
    QAction *m_runCMakeAction;
    QAction *m_runCMakeActionContextMenu;
    ProjectExplorer::Project *m_contextProject;
};

class CMakeSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    CMakeSettingsPage();
    ~CMakeSettingsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

    QString cmakeExecutable() const;
    void setCMakeExecutable(const QString &executable);
    bool isCMakeExecutableValid() const;
    bool hasCodeBlocksMsvcGenerator() const;
    bool hasCodeBlocksNinjaGenerator() const;

    TextEditor::Keywords keywords();

private:
    void saveSettings() const;
    QString findCmakeExecutable() const;

    Utils::PathChooser *m_pathchooser;
    CMakeValidator m_cmakeValidatorForUser;
    CMakeValidator m_cmakeValidatorForSystem;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECTMANAGER_H
