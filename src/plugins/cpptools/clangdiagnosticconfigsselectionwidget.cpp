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

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace CppTools {

ClangDiagnosticConfigsSelectionWidget::ClangDiagnosticConfigsSelectionWidget(QWidget *parent)
    : QWidget(parent)
    , m_label(new QLabel(tr("Diagnostic Configuration:")))
    , m_button(new QPushButton)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    layout->addWidget(m_label);
    layout->addWidget(m_button, 1);
    layout->addStretch();

    connect(m_button,
            &QPushButton::clicked,
            this,
            &ClangDiagnosticConfigsSelectionWidget::onButtonClicked);
}

void ClangDiagnosticConfigsSelectionWidget::refresh(const ClangDiagnosticConfigsModel &model,
                                                    const Utils::Id &configToSelect,
                                                    const CreateEditWidget &createEditWidget)
{
    m_diagnosticConfigsModel = model;
    m_currentConfigId = configToSelect;
    m_createEditWidget = createEditWidget;

    const ClangDiagnosticConfig config = m_diagnosticConfigsModel.configWithId(configToSelect);
    m_button->setText(config.displayName());
}

Utils::Id ClangDiagnosticConfigsSelectionWidget::currentConfigId() const
{
    return m_currentConfigId;
}

ClangDiagnosticConfigs ClangDiagnosticConfigsSelectionWidget::customConfigs() const
{
    return m_diagnosticConfigsModel.customConfigs();
}

void ClangDiagnosticConfigsSelectionWidget::onButtonClicked()
{
    ClangDiagnosticConfigsWidget *widget = m_createEditWidget(m_diagnosticConfigsModel.allConfigs(),
                                                              m_currentConfigId);
    widget->sync();
    widget->layout()->setContentsMargins(0, 0, 0, 0);

    QDialog dialog;
    dialog.setWindowTitle(ClangDiagnosticConfigsWidget::tr("Diagnostic Configurations"));
    dialog.setLayout(new QVBoxLayout);
    dialog.layout()->addWidget(widget);
    auto *buttonsBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialog.layout()->addWidget(buttonsBox);

    connect(buttonsBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonsBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    const bool previousEnableLowerClazyLevels = codeModelSettings()->enableLowerClazyLevels();
    if (dialog.exec() == QDialog::Accepted) {
        if (previousEnableLowerClazyLevels != codeModelSettings()->enableLowerClazyLevels())
            codeModelSettings()->toSettings(Core::ICore::settings());

        m_diagnosticConfigsModel = ClangDiagnosticConfigsModel(widget->configs());
        m_currentConfigId = widget->currentConfig().id();
        m_button->setText(widget->currentConfig().displayName());

        emit changed();
    }
}

} // CppTools namespace
