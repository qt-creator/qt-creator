// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvtargetdeviceviewer.h"

#include "uvproject.h" // for buildPackageId()
#include "uvtargetdevicemodel.h"

#include <baremetal/baremetaltr.h>

#include <utils/pathchooser.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace BareMetal::Internal::Uv {

// DeviceSelectorToolPanel

DeviceSelectorToolPanel::DeviceSelectorToolPanel(QWidget *parent)
    : FadingPanel(parent)
{
    const auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    const auto button = new QPushButton(Tr::tr("Manage..."));
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
    layout->addRow(Tr::tr("Vendor:"), m_vendorEdit);
    m_packageEdit = new QLineEdit;
    m_packageEdit->setReadOnly(true);
    layout->addRow(Tr::tr("Package:"), m_packageEdit);
    m_descEdit = new QPlainTextEdit;
    m_descEdit->setReadOnly(true);
    layout->addRow(Tr::tr("Description:"), m_descEdit);
    m_memoryView = new DeviceSelectionMemoryView(m_selection);
    layout->addRow(Tr::tr("Memory:"), m_memoryView);
    m_algorithmView = new DeviceSelectionAlgorithmView(m_selection);
    layout->addRow(Tr::tr("Flash algorithm:"), m_algorithmView);
    m_peripheralDescriptionFileChooser = new Utils::PathChooser(this);
    m_peripheralDescriptionFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_peripheralDescriptionFileChooser->setPromptDialogFilter(
                Tr::tr("Peripheral description files (*.svd)"));
    m_peripheralDescriptionFileChooser->setPromptDialogTitle(
                Tr::tr("Select Peripheral Description File"));
    layout->addRow(Tr::tr("Peripheral description file:"),
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
    connect(m_peripheralDescriptionFileChooser, &Utils::PathChooser::textChanged,
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
    m_peripheralDescriptionFileChooser->setFilePath(Utils::FilePath::fromString(m_selection.svd));
}

// DeviceSelector

DeviceSelector::DeviceSelector(QWidget *parent)
    : DetailsWidget(parent)
{
    const auto toolPanel = new DeviceSelectorToolPanel;
    setToolWidget(toolPanel);
    const auto detailsPanel = new DeviceSelectorDetailsPanel(m_selection);
    setWidget(detailsPanel);

    connect(toolPanel, &DeviceSelectorToolPanel::clicked, this, [this] {
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
    const QString summary = m_selection.name.isEmpty()
            ? Tr::tr("Target device not selected.") : m_selection.name;
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
    setWindowTitle(Tr::tr("Available Target Devices"));

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

} // BareMetal::Internal::Uv
