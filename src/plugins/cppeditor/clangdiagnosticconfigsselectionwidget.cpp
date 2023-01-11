// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfigsselectionwidget.h"

#include "clangdiagnosticconfigswidget.h"
#include "cppcodemodelsettings.h"
#include "cppeditortr.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace CppEditor {

ClangDiagnosticConfigsSelectionWidget::ClangDiagnosticConfigsSelectionWidget(QWidget *parent)
    : QWidget(parent)
{
    setUpUi(true);
}

ClangDiagnosticConfigsSelectionWidget::ClangDiagnosticConfigsSelectionWidget(
        QFormLayout *parentLayout)
{
    setUpUi(false);
    parentLayout->addRow(label(), this);
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

QString ClangDiagnosticConfigsSelectionWidget::label() const
{
    return Tr::tr("Diagnostic configuration:");
}

void ClangDiagnosticConfigsSelectionWidget::setUpUi(bool withLabel)
{
    m_button = new QPushButton;
    const auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    if (withLabel)
        layout->addWidget(new QLabel(label()));
    layout->addWidget(m_button);
    layout->addStretch();

    connect(m_button, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsSelectionWidget::onButtonClicked);
}

void ClangDiagnosticConfigsSelectionWidget::onButtonClicked()
{
    ClangDiagnosticConfigsWidget *widget = m_createEditWidget(m_diagnosticConfigsModel.allConfigs(),
                                                              m_currentConfigId);
    widget->sync();
    widget->layout()->setContentsMargins(0, 0, 0, 0);

    QDialog dialog;
    dialog.setWindowTitle(Tr::tr("Diagnostic Configurations"));
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

} // CppEditor namespace
