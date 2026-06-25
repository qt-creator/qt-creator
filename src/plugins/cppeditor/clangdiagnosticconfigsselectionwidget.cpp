// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfigsselectionwidget.h"

#include "clangdiagnosticconfigswidget.h"
#include "cppeditortr.h"

#include <coreplugin/icore.h>

#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>
#include <utils/store.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

using namespace Utils;

namespace CppEditor {

class ClangDiagnosticConfigsSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    ClangDiagnosticConfigsSelectionWidget()
    {
        using namespace Layouting;
        Row {
            Tr::tr("Diagnostic configuration:"),
            &m_button,
            st,
            noMargin
        }.attachTo(this);

        connect(&m_button, &QPushButton::clicked,
                this, &ClangDiagnosticConfigsSelectionWidget::onButtonClicked);
    }

    using CreateEditWidget
        = std::function<ClangDiagnosticConfigsWidget *(const ClangDiagnosticConfigs &configs,
                                                       const Id &configToSelect)>;

    void refresh(const ClangDiagnosticConfigsModel &model,
                 const Id &configToSelect,
                 const CreateEditWidget &createEditWidget)
    {
        m_diagnosticConfigsModel = model;
        m_currentConfigId = configToSelect;
        m_createEditWidget = createEditWidget;

        const ClangDiagnosticConfig config = m_diagnosticConfigsModel.configWithId(configToSelect);
        m_button.setText(config.displayName());
    }

    Id currentConfigId() const
    {
        return m_currentConfigId;
    }

    ClangDiagnosticConfigs customConfigs() const
    {
        return m_diagnosticConfigsModel.customConfigs();
    }

    void onButtonClicked()
    {
        const ClangDiagnosticConfigs oldConfigs = m_diagnosticConfigsModel.allConfigs();
        ClangDiagnosticConfigsWidget *widget = m_createEditWidget(oldConfigs, m_currentConfigId);
        widget->sync();
        widget->layout()->setContentsMargins(0, 0, 0, 0);

        QDialog dialog;
        dialog.setWindowTitle(Tr::tr("Diagnostic Configurations"));
        dialog.setLayout(new QVBoxLayout);
        dialog.layout()->addWidget(widget);
        auto buttonsBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        dialog.layout()->addWidget(buttonsBox);

        connect(buttonsBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonsBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            const Id origId = m_currentConfigId;
            m_diagnosticConfigsModel = ClangDiagnosticConfigsModel(widget->configs());
            m_currentConfigId = widget->currentConfig().id();
            const QString origDisplayName = m_button.text();
            m_button.setText(widget->currentConfig().displayName());

            emit changed();
            if (origId != m_currentConfigId || origDisplayName != m_button.text()
                || oldConfigs != widget->configs()) {
                Utils::checkSettingsDirty();
            }
        }
    }

signals:
    void changed();

private:
    ClangDiagnosticConfigsModel m_diagnosticConfigsModel;
    Utils::Id m_currentConfigId;

    QPushButton m_button;
    CreateEditWidget m_createEditWidget;
};

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
        m_committedCustomConfigs = m_customConfigs = m_widget->customConfigs();
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

bool DiagnosticConfigIdAspect::isDirty() const
{
    return TypedAspect<Id>::isDirty() || m_customConfigs != m_committedCustomConfigs;
}

void DiagnosticConfigIdAspect::apply()
{
    TypedAspect<Id>::apply();
    m_committedCustomConfigs = m_customConfigs;
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

void DiagnosticConfigIdAspect::fromMap(const Store &map)
{
    if (!settingsKey().isEmpty()) {
        const auto it = map.find(settingsKey());
        if (it != map.end())
            setValue(Id::fromSetting(it.value()), BeQuiet);
    }
}

void DiagnosticConfigIdAspect::toMap(Store &map) const
{
    if (!settingsKey().isEmpty())
        map.insert(settingsKey(), value().toSetting());
}

void DiagnosticConfigIdAspect::readSettings()
{
    TypedAspect<Id>::readSettings();
    if (m_persistCustomConfigs)
        m_customConfigs = diagnosticConfigsFromSettings(&Utils::userSettings());
}

void DiagnosticConfigIdAspect::writeSettings() const
{
    TypedAspect<Id>::writeSettings();
    if (m_persistCustomConfigs)
        diagnosticConfigsToSettings(&Utils::userSettings(), m_customConfigs);
}

} // namespace CppEditor

#include "clangdiagnosticconfigsselectionwidget.moc"
