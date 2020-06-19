/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "workspacedialog.h"

#include "dockmanager.h"

#include <utils/algorithm.h>

#include <QInputDialog>
#include <QRegularExpression>
#include <QValidator>

namespace ADS {

class WorkspaceValidator : public QValidator
{
public:
    WorkspaceValidator(QObject *parent, const QStringList &workspaces);
    void fixup(QString &input) const override;
    QValidator::State validate(QString &input, int &pos) const override;

private:
    QStringList m_workspaces;
};

WorkspaceValidator::WorkspaceValidator(QObject *parent, const QStringList &workspaces)
    : QValidator(parent)
    , m_workspaces(workspaces)
{}

QValidator::State WorkspaceValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)

    static const QRegularExpression rx("^[a-zA-Z0-9 ()\\-]*$");

    if (!rx.match(input).hasMatch())
        return QValidator::Invalid;

    if (m_workspaces.contains(input))
        return QValidator::Intermediate;
    else
        return QValidator::Acceptable;
}

void WorkspaceValidator::fixup(QString &input) const
{
    int i = 2;
    QString copy;
    do {
        copy = input + QLatin1String(" (") + QString::number(i) + QLatin1Char(')');
        ++i;
    } while (m_workspaces.contains(copy));
    input = copy;
}

WorkspaceNameInputDialog::WorkspaceNameInputDialog(DockManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    auto hlayout = new QVBoxLayout(this);
    auto label = new QLabel(tr("Enter the name of the workspace:"), this);
    hlayout->addWidget(label);
    m_newWorkspaceLineEdit = new QLineEdit(this);
    m_newWorkspaceLineEdit->setValidator(new WorkspaceValidator(this, m_manager->workspaces()));
    hlayout->addWidget(m_newWorkspaceLineEdit);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                        Qt::Horizontal,
                                        this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_switchToButton = new QPushButton;
    buttons->addButton(m_switchToButton, QDialogButtonBox::AcceptRole);
    connect(m_switchToButton, &QPushButton::clicked, [this]() { m_usedSwitchTo = true; });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    hlayout->addWidget(buttons);
    setLayout(hlayout);
}

void WorkspaceNameInputDialog::setActionText(const QString &actionText,
                                             const QString &openActionText)
{
    m_okButton->setText(actionText);
    m_switchToButton->setText(openActionText);
}

void WorkspaceNameInputDialog::setValue(const QString &value)
{
    m_newWorkspaceLineEdit->setText(value);
}

QString WorkspaceNameInputDialog::value() const
{
    return m_newWorkspaceLineEdit->text();
}

bool WorkspaceNameInputDialog::isSwitchToRequested() const
{
    return m_usedSwitchTo;
}

WorkspaceDialog::WorkspaceDialog(DockManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    m_ui.setupUi(this);
    m_ui.workspaceView->setActivationMode(Utils::DoubleClickActivation);

    connect(m_ui.btCreateNew,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::createNewWorkspace);
    connect(m_ui.btClone,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::cloneCurrentWorkspace);
    connect(m_ui.btDelete,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::deleteSelectedWorkspaces);
    connect(m_ui.btSwitch,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::switchToCurrentWorkspace);
    connect(m_ui.btRename,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::renameCurrentWorkspace);
    connect(m_ui.btReset,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::resetCurrentWorkspace);
    connect(m_ui.workspaceView,
            &WorkspaceView::activated,
            m_ui.workspaceView,
            &WorkspaceView::switchToCurrentWorkspace);
    connect(m_ui.workspaceView,
            &WorkspaceView::selected,
            this,
            &WorkspaceDialog::updateActions);
    connect(m_ui.btImport,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::importWorkspace);
    connect(m_ui.btExport,
            &QAbstractButton::clicked,
            m_ui.workspaceView,
            &WorkspaceView::exportCurrentWorkspace);

    m_ui.whatsAWorkspaceLabel->setOpenExternalLinks(true);

   updateActions(m_ui.workspaceView->selectedWorkspaces());
}

void WorkspaceDialog::setAutoLoadWorkspace(bool check)
{
    m_ui.autoLoadCheckBox->setChecked(check);
}

bool WorkspaceDialog::autoLoadWorkspace() const
{
    return m_ui.autoLoadCheckBox->checkState() == Qt::Checked;
}

DockManager *WorkspaceDialog::dockManager() const
{
    return m_manager;
}

void WorkspaceDialog::updateActions(const QStringList &workspaces)
{
    if (workspaces.isEmpty()) {
        m_ui.btDelete->setEnabled(false);
        m_ui.btRename->setEnabled(false);
        m_ui.btClone->setEnabled(false);
        m_ui.btReset->setEnabled(false);
        m_ui.btSwitch->setEnabled(false);
        m_ui.btExport->setEnabled(false);
        return;
    }
    const bool presetIsSelected = Utils::anyOf(workspaces, [this](const QString &workspace) {
        return m_manager->isWorkspacePreset(workspace);
    });
    const bool activeIsSelected = Utils::anyOf(workspaces, [this](const QString &workspace) {
        return workspace == m_manager->activeWorkspace();
    });
    m_ui.btDelete->setEnabled(!activeIsSelected && !presetIsSelected);
    m_ui.btRename->setEnabled(workspaces.size() == 1 && !presetIsSelected);
    m_ui.btClone->setEnabled(workspaces.size() == 1);
    m_ui.btReset->setEnabled(presetIsSelected);
    m_ui.btSwitch->setEnabled(workspaces.size() == 1);
    m_ui.btExport->setEnabled(workspaces.size() == 1);
}

} // namespace ADS
