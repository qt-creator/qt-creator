/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CMAKEPROJECTMANAGER_H
#define CMAKEPROJECTMANAGER_H

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/iprojectmanager.h>
#include <utils/pathchooser.h>
#include <QtCore/QFuture>
#include <QtCore/QStringList>
#include <QtCore/QDir>

namespace CMakeProjectManager {
namespace Internal {

class CMakeSettingsPage;
class CMakeRunner;

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

    QString createXmlFile(const QStringList &arguments, const QString &sourceDirectory, const QDir &buildDirectory);
private:
    int m_projectContext;
    int m_projectLanguage;
    CMakeSettingsPage *m_settingsPage;
};

class CMakeRunner
{
public:
    CMakeRunner();
    void setExecutable(const QString &executable);
    QString executable() const;
    QString version() const;
    bool supportsQtCreator() const;

private:
    void run(QFutureInterface<void> &fi);
    void waitForUpToDate() const;
    QString m_executable;
    QString m_version;
    bool m_supportsQtCreator;
    bool m_cacheUpToDate;
    mutable QFuture<void> m_future;
    mutable QMutex m_mutex;
};

class CMakeSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    CMakeSettingsPage();
    virtual ~CMakeSettingsPage();
    virtual QString name() const;
    virtual QString category() const;
    virtual QString trCategory() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();

    QString cmakeExecutable() const;
    void askUserForCMakeExecutable();
private:
    void updateCachedInformation() const;
    void saveSettings() const;
    QString findCmakeExecutable() const;

    mutable CMakeRunner m_cmakeRunner;
    Core::Utils::PathChooser *m_pathchooser;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECTMANAGER_H
