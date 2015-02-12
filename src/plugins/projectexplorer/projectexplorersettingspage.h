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
