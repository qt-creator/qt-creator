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

#include "clangdiagnosticconfigsselectionwidget.h"

#include "clangdiagnosticconfigswidget.h"
#include "cppcodemodelsettings.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>

namespace CppTools {

ClangDiagnosticConfigsSelectionWidget::ClangDiagnosticConfigsSelectionWidget(QWidget *parent)
    : QWidget(parent)
    , m_label(new QLabel(tr("Diagnostic Configuration:"), this))
    , m_selectionComboBox(new QComboBox(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    setLayout(layout);
    layout->addWidget(m_label);
    layout->addWidget(m_selectionComboBox, 1);
    auto *manageButton = new QPushButton(tr("Manage..."), this);
    layout->addWidget(manageButton);
    layout->addStretch();

    connectToClangDiagnosticConfigsDialog(manageButton);

    refresh(codeModelSettings()->clangDiagnosticConfigId());

    connectToCurrentIndexChanged();
}

Core::Id ClangDiagnosticConfigsSelectionWidget::currentConfigId() const
{
    return Core::Id::fromSetting(m_selectionComboBox->currentData());
}

void ClangDiagnosticConfigsSelectionWidget::connectToCurrentIndexChanged()
{
    m_currentIndexChangedConnection
            = connect(m_selectionComboBox,
                      static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                      this,
                      [this]() { emit currentConfigChanged(currentConfigId()); });
}

void ClangDiagnosticConfigsSelectionWidget::disconnectFromCurrentIndexChanged()
{
    disconnect(m_currentIndexChangedConnection);
}

void ClangDiagnosticConfigsSelectionWidget::refresh(Core::Id id)
{
    disconnectFromCurrentIndexChanged();

    int configToSelectIndex = -1;
    m_selectionComboBox->clear();
    m_diagnosticConfigsModel = ClangDiagnosticConfigsModel(
                codeModelSettings()->clangCustomDiagnosticConfigs());
    const int size = m_diagnosticConfigsModel.size();
    for (int i = 0; i < size; ++i) {
        const ClangDiagnosticConfig &config = m_diagnosticConfigsModel.at(i);
        const QString displayName
                = ClangDiagnosticConfigsModel::displayNameWithBuiltinIndication(config);
        m_selectionComboBox->addItem(displayName, config.id().toSetting());

        if (id == config.id())
            configToSelectIndex = i;
    }

    if (configToSelectIndex != -1)
        m_selectionComboBox->setCurrentIndex(configToSelectIndex);
    else
        emit currentConfigChanged(currentConfigId());

    connectToCurrentIndexChanged();
}

void ClangDiagnosticConfigsSelectionWidget::connectToClangDiagnosticConfigsDialog(QPushButton *button)
{
    connect(button, &QPushButton::clicked, [this]() {
        ClangDiagnosticConfigsWidget *widget = new ClangDiagnosticConfigsWidget(currentConfigId());
        widget->layout()->setMargin(0);
        QDialog dialog;
        dialog.setWindowTitle(ClangDiagnosticConfigsWidget::tr("Diagnostic Configurations"));
        dialog.setLayout(new QVBoxLayout);
        dialog.layout()->addWidget(widget);
        auto *buttonsBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        dialog.layout()->addWidget(buttonsBox);
        connect(buttonsBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonsBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        const bool previousEnableLowerClazyLevels = codeModelSettings()->enableLowerClazyLevels();
        connect(&dialog, &QDialog::accepted, [widget, previousEnableLowerClazyLevels]() {
            QSharedPointer<CppCodeModelSettings> settings = codeModelSettings();
            const ClangDiagnosticConfigs oldDiagnosticConfigs
                    = settings->clangCustomDiagnosticConfigs();
            const ClangDiagnosticConfigs currentDiagnosticConfigs = widget->customConfigs();
            if (oldDiagnosticConfigs != currentDiagnosticConfigs
                 || previousEnableLowerClazyLevels != codeModelSettings()->enableLowerClazyLevels()) {
                const ClangDiagnosticConfigsModel configsModel(currentDiagnosticConfigs);
                if (!configsModel.hasConfigWithId(settings->clangDiagnosticConfigId()))
                    settings->resetClangDiagnosticConfigId();
                settings->setClangCustomDiagnosticConfigs(currentDiagnosticConfigs);
                settings->toSettings(Core::ICore::settings());
            }
        });
        dialog.exec();
    });
}

} // CppTools namespace
