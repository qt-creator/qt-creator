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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

class BuildConfiguration;
class BuildInfo;
class NamedWidget;
class Target;

namespace Internal {

class BuildSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    BuildSettingsWidget(Target *target);
    ~BuildSettingsWidget() override;

    void clearWidgets();
    void addSubWidget(NamedWidget *widget);
    QList<NamedWidget *> subWidgets() const;

private:
    void updateBuildSettings();
    void currentIndexChanged(int index);

    void renameConfiguration();
    void updateAddButtonMenu();

    void updateActiveConfiguration();

    void createConfiguration(BuildInfo *info);
    void cloneConfiguration(BuildConfiguration *toClone);
    void deleteConfiguration(BuildConfiguration *toDelete);
    QString uniqueName(const QString &name);

    Target *m_target = nullptr;
    BuildConfiguration *m_buildConfiguration = nullptr;

    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_renameButton = nullptr;
    QPushButton *m_makeActiveButton = nullptr;
    QComboBox *m_buildConfigurationComboBox = nullptr;
    QMenu *m_addButtonMenu = nullptr;

    QList<NamedWidget *> m_subWidgets;
    QList<QLabel *> m_labels;
    QList<BuildInfo *> m_buildInfoList;
};

} // namespace Internal
} // namespace ProjectExplorer
