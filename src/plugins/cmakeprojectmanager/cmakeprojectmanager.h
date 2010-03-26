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

#ifndef CMAKEPROJECTMANAGER_H
#define CMAKEPROJECTMANAGER_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/environment.h>
#include <utils/pathchooser.h>
#include <QtCore/QFuture>
#include <QtCore/QStringList>
#include <QtCore/QDir>

QT_FORWARD_DECLARE_CLASS(QProcess)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace CMakeProjectManager {
namespace Internal {

class CMakeSettingsPage;

class CMakeManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT
public:
    CMakeManager(CMakeSettingsPage *cmakeSettingsPage);

    virtual int projectContext() const;
    virtual int projectLanguage() const;

    virtual ProjectExplorer::Project *openProject(const QString &fileName);
    virtual QString mimeType() const;

    QString cmakeExecutable() const;
    bool isCMakeExecutableValid() const;

    void setCMakeExecutable(const QString &executable);

    void createXmlFile(QProcess *process,
                       const QStringList &arguments,
                       const QString &sourceDirectory,
                       const QDir &buildDirectory,
                       const ProjectExplorer::Environment &env,
                       const QString &generator);
    bool hasCodeBlocksMsvcGenerator() const;
    static QString findCbpFile(const QDir &);

    static QString findDumperLibrary(const ProjectExplorer::Environment &env);
private:
    static QString qtVersionForQMake(const QString &qmakePath);
    static QPair<QString, QString> findQtDir(const ProjectExplorer::Environment &env);
    int m_projectContext;
    int m_projectLanguage;
    CMakeSettingsPage *m_settingsPage;
};

class CMakeSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    CMakeSettingsPage();
    virtual ~CMakeSettingsPage();
    virtual QString id() const;
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();

    QString cmakeExecutable() const;
    void setCMakeExecutable(const QString &executable);
    bool isCMakeExecutableValid();
    bool hasCodeBlocksMsvcGenerator() const;
private slots:
    void cmakeFinished();
private:
    void saveSettings() const;
    void startProcess();
    QString findCmakeExecutable() const;
    void updateInfo();

    Utils::PathChooser *m_pathchooser;
    QString m_cmakeExecutable;
    enum STATE { VALID, INVALID, RUNNING } m_state;
    QProcess *m_process;
    QString m_version;
    bool m_hasCodeBlocksMsvcGenerator;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECTMANAGER_H
