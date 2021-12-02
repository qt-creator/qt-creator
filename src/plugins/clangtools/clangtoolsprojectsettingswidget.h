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
class QPushButton;
class QTreeView;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace ClangTools {
namespace Internal {

class ClangToolsProjectSettings;
class RunSettingsWidget;

class ProjectSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectSettingsWidget(ProjectExplorer::Project *project, QWidget *parent = nullptr);

private:
    void onGlobalCustomChanged();
    void onGlobalCustomChanged(int index);

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
