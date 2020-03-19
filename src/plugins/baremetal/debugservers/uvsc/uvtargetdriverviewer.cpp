/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "uvproject.h" // for targetUVisionPath()
#include "uvtargetdrivermodel.h"
#include "uvtargetdriverviewer.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace BareMetal {
namespace Internal {
namespace Uv {

// DriverSelectorToolPanel

DriverSelectorToolPanel::DriverSelectorToolPanel(QWidget *parent)
    : FadingPanel(parent)
{
    const auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    const auto button = new QPushButton(tr("Manage..."));
    layout->addWidget(button);
    setLayout(layout);
    connect(button, &QPushButton::clicked, this, &DriverSelectorToolPanel::clicked);
}

void DriverSelectorToolPanel::fadeTo(qreal value)
{
    Q_UNUSED(value)
}

void DriverSelectorToolPanel::setOpacity(qreal value)
{
    Q_UNUSED(value)
}

// DriverSelectorDetailsPanel

DriverSelectorDetailsPanel::DriverSelectorDetailsPanel(DriverSelection &selection, QWidget *parent)
    : QWidget(parent), m_selection(selection)
{
    const auto layout = new QFormLayout;
    m_dllEdit = new QLineEdit;;
    m_dllEdit->setReadOnly(true);
    m_dllEdit->setToolTip(tr("Debugger driver library."));
    layout->addRow(tr("Driver library:"), m_dllEdit);
    m_cpuDllView = new DriverSelectionCpuDllView(m_selection);
    layout->addRow(tr("CPU library:"), m_cpuDllView);
    setLayout(layout);

    refresh();

    connect(m_cpuDllView, &DriverSelectionCpuDllView::dllChanged, this, [this](int index) {
        if (index >= 0)
            m_selection.cpuDllIndex = index;
        emit selectionChanged();
    });
}

void DriverSelectorDetailsPanel::refresh()
{
    m_dllEdit->setText((m_selection.dll));
    m_cpuDllView->refresh();
    m_cpuDllView->setCpuDll(m_selection.cpuDllIndex);
}

// DriverSelector

DriverSelector::DriverSelector(const QStringList &supportedDrivers, QWidget *parent)
    : DetailsWidget(parent)
{
    const auto toolPanel = new DriverSelectorToolPanel;
    toolPanel->setEnabled(!supportedDrivers.isEmpty());
    setToolWidget(toolPanel);
    const auto detailsPanel = new DriverSelectorDetailsPanel(m_selection);
    setWidget(detailsPanel);

    connect(toolPanel, &DriverSelectorToolPanel::clicked, this, [=]() {
        DriverSelectionDialog dialog(m_toolsIniFile, supportedDrivers, this);
        const int result = dialog.exec();
        if (result != QDialog::Accepted)
            return;
        DriverSelection selection;
        selection = dialog.selection();
        setSelection(selection);
    });

    connect(detailsPanel, &DriverSelectorDetailsPanel::selectionChanged,
            this, &DriverSelector::selectionChanged);
}

void DriverSelector::setToolsIniFile(const Utils::FilePath &toolsIniFile)
{
    m_toolsIniFile = toolsIniFile;
    setEnabled(m_toolsIniFile.exists());
}

Utils::FilePath DriverSelector::toolsIniFile() const
{
    return m_toolsIniFile;
}

void DriverSelector::setSelection(const DriverSelection &selection)
{
    m_selection = selection;
    const auto summary = m_selection.name.isEmpty()
            ? tr("Target driver not selected.") : m_selection.name;
    setSummaryText(summary);
    setExpandable(!m_selection.name.isEmpty());

    if (const auto detailsPanel = qobject_cast<DriverSelectorDetailsPanel *>(widget()))
        detailsPanel->refresh();

    emit selectionChanged();
}

DriverSelection DriverSelector::selection() const
{
    return m_selection;
}

// DriverSelectionDialog

DriverSelectionDialog::DriverSelectionDialog(const Utils::FilePath &toolsIniFile,
                                             const QStringList &supportedDrivers,
                                             QWidget *parent)
    : QDialog(parent), m_model(new DriverSelectionModel(this)),
      m_view(new DriverSelectionView(this))
{
    setWindowTitle(tr("Available Target Drivers"));

    const auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &DriverSelectionDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DriverSelectionDialog::reject);

    connect(m_view, &DriverSelectionView::driverSelected, this,
            [this](const DriverSelection &selection) {
        m_selection = selection;
    });

    m_model->fillDrivers(toolsIniFile, supportedDrivers);
    m_view->setModel(m_model);
}

void DriverSelectionDialog::setSelection(const DriverSelection &selection)
{
    m_selection = selection;
}

DriverSelection DriverSelectionDialog::selection() const
{
    return m_selection;
}

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
