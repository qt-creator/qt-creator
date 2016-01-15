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

#ifndef PROJECTEXPLORERSETTINGSPAGE_H
#define PROJECTEXPLORERSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_projectexplorersettingspage.h"

#include <QPointer>
#include <QUuid>

namespace ProjectExplorer {
namespace Internal {

class ProjectExplorerSettings;

// Documentation inside.
class ProjectExplorerSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProjectExplorerSettingsWidget(QWidget *parent = 0);

    ProjectExplorerSettings settings() const;
    void setSettings(const ProjectExplorerSettings  &s);

    QString projectsDirectory() const;
    void setProjectsDirectory(const QString &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

    QString buildDirectory() const;
    void setBuildDirectory(const QString &bd);

private slots:
    void slotDirectoryButtonGroupChanged();
    void resetDefaultBuildDirectory();
    void updateResetButton();

private:
    void setJomVisible(bool);

    Ui::ProjectExplorerSettingsPageUi m_ui;
    QUuid m_environmentId;
};

class ProjectExplorerSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    ProjectExplorerSettingsPage();
    ~ProjectExplorerSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<ProjectExplorerSettingsWidget> m_widget;
};

} // Internal
} // ProjectExplorer

#endif // PROJECTEXPLORERSETTINGSPAGE_H
