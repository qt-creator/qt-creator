// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectsettingswidget.h>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Todo {
namespace Internal {

class TodoProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
    Q_OBJECT

public:
    explicit TodoProjectSettingsWidget(ProjectExplorer::Project *project);
    ~TodoProjectSettingsWidget() override;

signals:
    void projectSettingsChanged();

private:
    void addExcludedPatternButtonClicked();
    void removeExcludedPatternButtonClicked();
    void setExcludedPatternsButtonsEnabled();
    void excludedPatternChanged(QListWidgetItem *item);
    QListWidgetItem *addToExcludedPatternsList(const QString &pattern);
    void loadSettings();
    void saveSettings();
    void prepareItem(QListWidgetItem *item) const;

    ProjectExplorer::Project *m_project;
    QListWidget *m_excludedPatternsList;
    QPushButton *m_removeExcludedPatternButton;
};

} // namespace Internal
} // namespace Todo
