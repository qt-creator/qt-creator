// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectsettingswidget.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace ClangTools {
namespace Internal {

class ClangToolsProjectSettings;
class RunSettingsWidget;

class ClangToolsProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
    Q_OBJECT

public:
    explicit ClangToolsProjectSettingsWidget(ProjectExplorer::Project *project, QWidget *parent = nullptr);

private:
    void onGlobalCustomChanged(bool useGlobal);

    void updateButtonStates();
    void updateButtonStateRemoveSelected();
    void updateButtonStateRemoveAll();
    void removeSelected();

    QComboBox *m_globalCustomComboBox;
    QPushButton *m_restoreGlobal;
    RunSettingsWidget *m_runSettingsWidget;
    QTreeView *m_diagnosticsView;
    QPushButton *m_removeSelectedButton;
    QPushButton *m_removeAllButton;

    QSharedPointer<ClangToolsProjectSettings> const m_projectSettings;
};

} // namespace Internal
} // namespace ClangTools
