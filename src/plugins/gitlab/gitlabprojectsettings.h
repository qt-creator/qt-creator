/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <projectexplorer/projectsettingswidget.h>
#include <utils/id.h>

#include <QObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

#include <utility>

namespace ProjectExplorer { class Project; }

namespace Utils { class InfoLabel; }

namespace GitLab {

class Project;

class GitLabProjectSettings : public QObject
{
    Q_OBJECT
public:
    explicit GitLabProjectSettings(ProjectExplorer::Project *project);
    Utils::Id currentServer() const { return m_id; }
    void setCurrentServer(const Utils::Id &id) { m_id = id; }
    QString currentServerHost() const { return m_host; }
    void setCurrentServerHost(const QString &server) { m_host = server; }
    void setCurrentProject(const QString &projectName) { m_currentProject = projectName; }
    QString currentProject() const { return m_currentProject; }
    bool isLinked() const { return m_linked; }
    void setLinked(bool linked);
    ProjectExplorer::Project *project() const { return m_project; }

    static std::tuple<QString, QString, int> remotePartsFromRemote(const QString &remote);
private:
    void load();
    void save();

    ProjectExplorer::Project *m_project = nullptr;
    QString m_host;
    Utils::Id m_id;
    QString m_currentProject;
    bool m_linked = false;
};

class GitLabProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
    Q_OBJECT
public:
    explicit GitLabProjectSettingsWidget(ProjectExplorer::Project *project,
                                         QWidget *parent = nullptr);

private:
    enum CheckMode { Connection, Link };

    void unlink();
    void checkConnection(CheckMode mode);
    void onConnectionChecked(const Project &project, const Utils::Id &serverId,
                             const QString &remote, const QString &projName);
    void updateUi();
    void updateEnabledStates();

    GitLabProjectSettings *m_projectSettings = nullptr;
    QComboBox *m_linkedGitLabServer = nullptr;
    QComboBox *m_hostCB = nullptr;
    QPushButton *m_linkWithGitLab = nullptr;
    QPushButton *m_unlink = nullptr;
    QPushButton *m_checkConnection = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;
    CheckMode m_checkMode = Connection;
};

} // namespace GitLab

