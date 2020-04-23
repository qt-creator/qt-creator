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

#include "uvproject.h" // for buildPackageId()
#include "uvtargetdevicemodel.h"
#include "uvtargetdeviceviewer.h"

#include <utils/pathchooser.h>

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

// DeviceSelectorToolPanel

DeviceSelectorToolPanel::DeviceSelectorToolPanel(QWidget *parent)
    : FadingPanel(parent)
{
    const auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    const auto button = new QPushButton(tr("Manage..."));
    layout->addWidget(button);
    setLayout(layout);
    connect(button, &QPushButton::clicked, this, &DeviceSelectorToolPanel::clicked);
}

void DeviceSelectorToolPanel::fadeTo(qreal value)
{
    Q_UNUSED(value)
}

void DeviceSelectorToolPanel::setOpacity(qreal value)
{
    Q_UNUSED(value)
}

// DeviceSelectorDetailsPanel

DeviceSelectorDetailsPanel::DeviceSelectorDetailsPanel(DeviceSelection &selection,
                                                       QWidget *parent)
    : QWidget(parent), m_selection(selection)
{
    const auto layout = new QFormLayout;
    m_vendorEdit = new QLineEdit;
    m_vendorEdit->setReadOnly(true);
    layout->addRow(tr("Vendor:"), m_vendorEdit);
    m_packageEdit = new QLineEdit;
    m_packageEdit->setReadOnly(true);
    layout->addRow(tr("Package:"), m_packageEdit);
    m_descEdit = new QPlainTextEdit;
    m_descEdit->setReadOnly(true);
    layout->addRow(tr("Description:"), m_descEdit);
    m_memoryView = new DeviceSelectionMemoryView(m_selection);
    layout->addRow(tr("Memory:"), m_memoryView);
    m_algorithmView = new DeviceSelectionAlgorithmView(m_selection);
    layout->addRow(tr("Flash algorithm:"), m_algorithmView);
    m_peripheralDescriptionFileChooser = new Utils::PathChooser(this);
    m_peripheralDescriptionFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_peripheralDescriptionFileChooser->setPromptDialogFilter(
                tr("Peripheral description files (*.svd)"));
    m_peripheralDescriptionFileChooser->setPromptDialogTitle(
                tr("Select Peripheral Description File"));
    layout->addRow(tr("Peripheral description file:"),
                   m_peripheralDescriptionFileChooser);
    setLayout(layout);

    refresh();

    connect(m_memoryView, &DeviceSelectionMemoryView::memoryChanged,
            this, &DeviceSelectorDetailsPanel::selectionChanged);
    connect(m_algorithmView, &DeviceSelectionAlgorithmView::algorithmChanged,
            this, [this](int index) {
        if (index >= 0)
            m_selection.algorithmIndex = index;
        emit selectionChanged();
    });
    connect(m_peripheralDescriptionFileChooser, &Utils::PathChooser::pathChanged,
            this, &DeviceSelectorDetailsPanel::selectionChanged);
}

static QString trimVendor(const QString &vendor)
{
    const int colonIndex = vendor.lastIndexOf(':');
    return vendor.mid(0, colonIndex);
}

void DeviceSelectorDetailsPanel::refresh()
{
    m_vendorEdit->setText(trimVendor(m_selection.vendorName));
    m_packageEdit->setText(buildPackageId(m_selection));
    m_descEdit->setPlainText(m_selection.desc);
    m_memoryView->refresh();
    m_algorithmView->refresh();
    m_algorithmView->setAlgorithm(m_selection.algorithmIndex);
    m_peripheralDescriptionFileChooser->setPath(m_selection.svd);
}

// DeviceSelector

DeviceSelector::DeviceSelector(QWidget *parent)
    : DetailsWidget(parent)
{
    const auto toolPanel = new DeviceSelectorToolPanel;
    setToolWidget(toolPanel);
    const auto detailsPanel = new DeviceSelectorDetailsPanel(m_selection);
    setWidget(detailsPanel);

    connect(toolPanel, &DeviceSelectorToolPanel::clicked, this, [this]() {
        DeviceSelectionDialog dialog(m_toolsIniFile, this);
        const int result = dialog.exec();
        if (result != QDialog::Accepted)
            return;
        DeviceSelection selection;
        selection = dialog.selection();
        setSelection(selection);
    });

    connect(detailsPanel, &DeviceSelectorDetailsPanel::selectionChanged,
            this, &DeviceSelector::selectionChanged);
}

void DeviceSelector::setToolsIniFile(const Utils::FilePath &toolsIniFile)
{
    m_toolsIniFile = toolsIniFile;
    setEnabled(m_toolsIniFile.exists());
}

Utils::FilePath DeviceSelector::toolsIniFile() const
{
    return m_toolsIniFile;
}

void DeviceSelector::setSelection(const DeviceSelection &selection)
{
    m_selection = selection;
    const auto summary = m_selection.name.isEmpty()
            ? tr("Target device not selected.") : m_selection.name;
    setSummaryText(summary);
    setExpandable(!m_selection.name.isEmpty());

    if (const auto detailsPanel = qobject_cast<DeviceSelectorDetailsPanel *>(widget()))
        detailsPanel->refresh();

    emit selectionChanged();
}

DeviceSelection DeviceSelector::selection() const
{
    return m_selection;
}

// DeviceSelectionDialog

DeviceSelectionDialog::DeviceSelectionDialog(const Utils::FilePath &toolsIniFile, QWidget *parent)
    : QDialog(parent), m_model(new DeviceSelectionModel(this)), m_view(new DeviceSelectionView(this))
{
    setWindowTitle(tr("Available Target Devices"));

    const auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &DeviceSelectionDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DeviceSelectionDialog::reject);

    connect(m_view, &DeviceSelectionView::deviceSelected, this,
            [this](const DeviceSelection &selection) {
        m_selection = selection;
    });

    m_model->fillAllPacks(toolsIniFile);
    m_view->setModel(m_model);
}

void DeviceSelectionDialog::setSelection(const DeviceSelection &selection)
{
    m_selection = selection;
}

DeviceSelection DeviceSelectionDialog::selection() const
{
    return m_selection;
}

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
