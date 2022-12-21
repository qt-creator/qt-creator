// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

private:
    void updateBuildSettings();
    void currentIndexChanged(int index);

    void renameConfiguration();
    void updateAddButtonMenu();

    void updateActiveConfiguration();

    void createConfiguration(const BuildInfo &info);
    void cloneConfiguration();
    void deleteConfiguration(BuildConfiguration *toDelete);
    QString uniqueName(const QString &name);

    Target *m_target = nullptr;
    BuildConfiguration *m_buildConfiguration = nullptr;

    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_renameButton = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_makeActiveButton = nullptr;
    QComboBox *m_buildConfigurationComboBox = nullptr;
    QMenu *m_addButtonMenu = nullptr;

    QList<NamedWidget *> m_subWidgets;
    QList<QLabel *> m_labels;
};

} // namespace Internal
} // namespace ProjectExplorer
