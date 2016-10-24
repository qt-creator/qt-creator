/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include <projectexplorer/kitconfigwidget.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialog;
class QLabel;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
    class Kit;
    class KitInformation;
} // namespace ProjectExplorer

namespace CMakeProjectManager {

class CMakeTool;

namespace Internal {

// --------------------------------------------------------------------
// CMakeKitConfigWidget:
// --------------------------------------------------------------------

class CMakeKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    CMakeKitConfigWidget(ProjectExplorer::Kit *kit, const ProjectExplorer::KitInformation *ki);
    ~CMakeKitConfigWidget() override;

    // KitConfigWidget interface
    QString displayName() const override;
    void makeReadOnly() override;
    void refresh() override;
    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString toolTip() const override;

private:
    int indexOf(const Core::Id &id);
    void updateComboBox();
    void cmakeToolAdded(const Core::Id &id);
    void cmakeToolUpdated(const Core::Id &id);
    void cmakeToolRemoved(const Core::Id &id);
    void currentCMakeToolChanged(int index);
    void manageCMakeTools();

    bool m_removingItem = false;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
};

// --------------------------------------------------------------------
// CMakeGeneratorKitConfigWidget:
// --------------------------------------------------------------------

class CMakeGeneratorKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    CMakeGeneratorKitConfigWidget(ProjectExplorer::Kit *kit, const ProjectExplorer::KitInformation *ki);
    ~CMakeGeneratorKitConfigWidget() override;

    // KitConfigWidget interface
    QString displayName() const override;
    void makeReadOnly() override;
    void refresh() override;
    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString toolTip() const override;

private:
    void changeGenerator();

    bool m_ignoreChange = false;
    QLabel *m_label;
    QPushButton *m_changeButton;
    CMakeTool *m_currentTool = nullptr;
};

// --------------------------------------------------------------------
// CMakeConfigurationKitConfigWidget:
// --------------------------------------------------------------------

class CMakeConfigurationKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    CMakeConfigurationKitConfigWidget(ProjectExplorer::Kit *kit, const ProjectExplorer::KitInformation *ki);

    // KitConfigWidget interface
    QString displayName() const override;
    void makeReadOnly() override;
    void refresh() override;
    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;
    QString toolTip() const override;

private:
    void editConfigurationChanges();

    void applyChanges();
    void closeChangesDialog();
    void acceptChangesDialog();

    QLabel *m_summaryLabel;
    QPushButton *m_manageButton;
    QDialog *m_dialog = nullptr;
    QPlainTextEdit *m_editor = nullptr;
};

} // namespace Internal
} // namespace CMakeProjectManager
