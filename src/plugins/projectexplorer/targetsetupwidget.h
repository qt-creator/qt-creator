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

#pragma once

#include "projectexplorer_export.h"

#include "buildinfo.h"
#include "task.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpacerItem;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
} // namespace Utils

namespace ProjectExplorer {
class BuildInfo;
class Kit;

namespace Internal {

class TargetSetupWidget : public QWidget
{
    Q_OBJECT

public:
    TargetSetupWidget(Kit *k,
                      const QString &projectPath,
                      const QList<BuildInfo *> &infoList);
    ~TargetSetupWidget() override;

    Kit *kit();
    void clearKit();

    bool isKitSelected() const;
    void setKitSelected(bool b);

    void addBuildInfo(BuildInfo *info, bool isImport);

    QList<const BuildInfo *> selectedBuildInfoList() const;
    void setProjectPath(const QString &projectPath);
    void expandWidget();

signals:
    void selectedToggled() const;

private:
    void handleKitUpdate(ProjectExplorer::Kit *k);

    void checkBoxToggled(bool b);
    void pathChanged();
    void targetCheckBoxToggled(bool b);
    void manageKit();

    void reportIssues(int index);
    QPair<Task::TaskType, QString> findIssues(const BuildInfo *info);
    void clear();

    Kit *m_kit;
    QString m_projectPath;
    bool m_haveImported = false;
    Utils::DetailsWidget *m_detailsWidget;
    QPushButton *m_manageButton;
    QGridLayout *m_newBuildsLayout;
    QList<QCheckBox *> m_checkboxes;
    QList<Utils::PathChooser *> m_pathChoosers;
    QList<BuildInfo *> m_infoList;
    QList<bool> m_enabled;
    QList<QLabel *> m_reportIssuesLabels;
    QList<bool> m_issues;
    bool m_ignoreChange = false;
    int m_selected = 0; // Number of selected buildconfiguartions
};

} // namespace Internal
} // namespace ProjectExplorer
