// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

class BuildConfiguration;
class BuildInfo;
class Target;

namespace Internal {

class BuildSettingsWidget final : public QWidget
{
public:
    BuildSettingsWidget(Target *target);
    ~BuildSettingsWidget() final;

private:
    void clearWidgets();
    void addSubWidget(QWidget *widget, const QString &displayName);

    void updateBuildSettings();
    void currentIndexChanged(int index);

    void renameConfiguration();
    void updateAddButtonMenu();

    void updateActiveConfiguration();

    void createConfiguration(const BuildInfo &info);
    void cloneConfiguration();
    void deleteConfiguration(BuildConfiguration *toDelete);
    QString uniqueName(const QString &name, bool allowCurrentName);

    Target *m_target = nullptr;
    BuildConfiguration *m_buildConfiguration = nullptr;

    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_renameButton = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_makeActiveButton = nullptr;
    QComboBox *m_buildConfigurationComboBox = nullptr;
    QMenu *m_addButtonMenu = nullptr;

    QList<QWidget *> m_subWidgets;
};

} // namespace Internal
} // namespace ProjectExplorer
