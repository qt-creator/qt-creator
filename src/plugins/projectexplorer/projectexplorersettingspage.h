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

#ifndef PROJECTEXPLORERSETTINGSPAGE_H
#define PROJECTEXPLORERSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ui_projectexplorersettingspage.h"

#include <QPointer>

namespace ProjectExplorer {
namespace Internal {

struct ProjectExplorerSettings;

// Documentation inside.
class ProjectExplorerSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProjectExplorerSettingsWidget(QWidget *parent = 0);

    ProjectExplorerSettings settings() const;
    void setSettings(const ProjectExplorerSettings  &s) const;

    QString projectsDirectory() const;
    void setProjectsDirectory(const QString &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

    QString buildDirectory() const;
    void setBuildDirectory(const QString &bd);

    QString searchKeywords() const;

private slots:
    void slotDirectoryButtonGroupChanged();
    void resetDefaultBuildDirectory();
    void updateResetButton();

private:
    void setJomVisible(bool);

    Ui::ProjectExplorerSettingsPageUi m_ui;
    mutable QString m_searchKeywords;
};

class ProjectExplorerSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    ProjectExplorerSettingsPage();
    ~ProjectExplorerSettingsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &s) const;

private:
    QString m_searchKeywords;
    QPointer<ProjectExplorerSettingsWidget> m_widget;
};

} // Internal
} // ProjectExplorer

#endif // PROJECTEXPLORERSETTINGSPAGE_H
