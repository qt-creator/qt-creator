// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvtargetdrivermodel.h"
#include "uvtargetdriverviewer.h"

#include <baremetal/baremetaltr.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace BareMetal::Internal::Uv {

// DriverSelectorToolPanel

DriverSelectorToolPanel::DriverSelectorToolPanel(QWidget *parent)
    : FadingPanel(parent)
{
    const auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    const auto button = new QPushButton(Tr::tr("Manage..."));
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
    m_dllEdit->setToolTip(Tr::tr("Debugger driver library."));
    layout->addRow(Tr::tr("Driver library:"), m_dllEdit);
    m_cpuDllView = new DriverSelectionCpuDllView(m_selection);
    layout->addRow(Tr::tr("CPU library:"), m_cpuDllView);
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
            ? Tr::tr("Target driver not selected.") : m_selection.name;
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
    setWindowTitle(Tr::tr("Available Target Drivers"));

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

} // BareMetal::Internal::Uv
