// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfigsselectionwidget.h"

#include "clangdiagnosticconfigswidget.h"
#include "cppeditortr.h"

#include <coreplugin/icore.h>

#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

using namespace Utils;

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
                                                    const Id &configToSelect,
                                                    const CreateEditWidget &createEditWidget)
{
    m_diagnosticConfigsModel = model;
    m_currentConfigId = configToSelect;
    m_createEditWidget = createEditWidget;

    const ClangDiagnosticConfig config = m_diagnosticConfigsModel.configWithId(configToSelect);
    m_button->setText(config.displayName());
}

Id ClangDiagnosticConfigsSelectionWidget::currentConfigId() const
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
    const ClangDiagnosticConfigs oldConfigs = m_diagnosticConfigsModel.allConfigs();
    ClangDiagnosticConfigsWidget *widget = m_createEditWidget(oldConfigs, m_currentConfigId);
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

    if (dialog.exec() == QDialog::Accepted) {
        const Id origId = m_currentConfigId;
        m_diagnosticConfigsModel = ClangDiagnosticConfigsModel(widget->configs());
        m_currentConfigId = widget->currentConfig().id();
        const QString origDisplayName = m_button->text();
        m_button->setText(widget->currentConfig().displayName());

        emit changed();
        if (origId != m_currentConfigId || origDisplayName != m_button->text()
            || oldConfigs != widget->configs()) {
            Utils::markSettingsDirty();
        }
    }
}

// DiagnosticConfigIdAspect

DiagnosticConfigIdAspect::DiagnosticConfigIdAspect(AspectContainer *container)
    : TypedAspect(container)
{}

void DiagnosticConfigIdAspect::setModelFactory(ModelFactory factory)
{
    m_modelFactory = std::move(factory);
}

void DiagnosticConfigIdAspect::setEditWidgetFactory(EditWidgetFactory factory)
{
    m_editFactory = std::move(factory);
}

void DiagnosticConfigIdAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    m_widget = createSubWidget<ClangDiagnosticConfigsSelectionWidget>();
    if (m_modelFactory && m_editFactory) {
        const ClangDiagnosticConfigsModel model = m_modelFactory();
        const Id id = model.hasConfigWithId(volatileValue()) ? volatileValue() : defaultValue();
        m_widget->refresh(model, id, m_editFactory);
        m_customConfigs = m_widget->customConfigs();
    }
    connect(m_widget, &ClangDiagnosticConfigsSelectionWidget::changed, this, [this] {
        if (m_widget)
            handleGuiChanged();
    });
    parent.addItem(m_widget.data());
}

bool DiagnosticConfigIdAspect::guiToVolatileValue()
{
    if (!m_widget)
        return false;
    const Id newId = m_widget->currentConfigId();
    const ClangDiagnosticConfigs newConfigs = m_widget->customConfigs();
    const bool changed = (newId != m_volatileValue) || (newConfigs != m_customConfigs);
    m_volatileValue = newId;
    m_customConfigs = newConfigs;
    return changed;
}

void DiagnosticConfigIdAspect::refresh()
{
    volatileValueToGui();
}

void DiagnosticConfigIdAspect::volatileValueToGui()
{
    if (!m_widget || !m_modelFactory || !m_editFactory)
        return;
    const ClangDiagnosticConfigsModel model = m_modelFactory();
    const Id id = model.hasConfigWithId(m_volatileValue) ? m_volatileValue : defaultValue();
    m_widget->refresh(model, id, m_editFactory);
    m_customConfigs = m_widget->customConfigs();
}

} // namespace CppEditor
