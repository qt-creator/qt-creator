// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectsettingswidget.h>

#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Autotest {

class ITestFramework;
class ITestTool;

namespace Internal {

class TestProjectSettings;

class ProjectTestSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
    Q_OBJECT
public:
    explicit ProjectTestSettingsWidget(ProjectExplorer::Project *project,
                                       QWidget *parent = nullptr);

private:
    void populateFrameworks(const QHash<Autotest::ITestFramework *, bool> &frameworks,
                            const QHash<Autotest::ITestTool *, bool> &testTools);
    void onActiveFrameworkChanged(QTreeWidgetItem *item, int column);
    TestProjectSettings *m_projectSettings;
    QComboBox *m_useGlobalSettings = nullptr;
    QTreeWidget *m_activeFrameworks = nullptr;
    QComboBox *m_runAfterBuild = nullptr;
    QTimer m_syncTimer;
    int m_syncType = 0;
};

} // namespace Internal
} // namespace Autotest
