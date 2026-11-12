// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfigidaspect.h"

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

#ifdef WITH_TESTS
#include <QTest>
#endif

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

    ClangDiagnosticConfigsModel model() const
    {
        return m_diagnosticConfigsModel;
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

// ClangDiagnosticConfigIdAspect

ClangDiagnosticConfigIdAspect::ClangDiagnosticConfigIdAspect(AspectContainer *container)
    : TypedAspect(container)
{}

void ClangDiagnosticConfigIdAspect::setModelFactory(ModelFactory factory)
{
    m_modelFactory = std::move(factory);
}

void ClangDiagnosticConfigIdAspect::setEditWidgetFactory(EditWidgetFactory factory)
{
    m_editFactory = std::move(factory);
}

void ClangDiagnosticConfigIdAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto widget = createSubWidget<ClangDiagnosticConfigsSelectionWidget>();
    ++m_widgetCount;
    connect(widget, &QObject::destroyed, this, [this] { --m_widgetCount; });

    connect(this, &ClangDiagnosticConfigIdAspect::configsChanged, widget, [this, widget] {
        const Id id = m_model.hasConfigWithId(m_guiId) ? m_guiId : defaultValue();
        widget->refresh(m_model, id, m_editFactory);
    });
    connect(widget, &ClangDiagnosticConfigsSelectionWidget::changed, this, [this, widget] {
        m_model = widget->model();
        m_guiId = widget->currentConfigId();
        m_guiCustomConfigs = widget->customConfigs();
        emit configsChanged(QPrivateSignal());
        handleGuiChanged();
    });

    if (m_modelFactory && m_editFactory) {
        m_model = m_modelFactory();
        m_committedCustomConfigs = m_customConfigs = m_model.customConfigs();
        m_guiId = m_volatileValue;
        m_guiCustomConfigs = m_customConfigs;
        emit configsChanged(QPrivateSignal());
    }
    parent.addItem(widget);
}

bool ClangDiagnosticConfigIdAspect::guiToVolatileValue()
{
    if (m_widgetCount == 0)
        return false;
    const bool changed = (m_guiId != m_volatileValue) || (m_guiCustomConfigs != m_customConfigs);
    m_volatileValue = m_guiId;
    m_customConfigs = m_guiCustomConfigs;
    return changed;
}

bool ClangDiagnosticConfigIdAspect::isDirty() const
{
    return TypedAspect<Id>::isDirty() || m_customConfigs != m_committedCustomConfigs;
}

void ClangDiagnosticConfigIdAspect::apply()
{
    TypedAspect<Id>::apply();
    m_committedCustomConfigs = m_customConfigs;
}

void ClangDiagnosticConfigIdAspect::refresh()
{
    volatileValueToGui();
}

bool ClangDiagnosticConfigIdAspect::hasWidget() const
{
    return m_widgetCount > 0;
}

void ClangDiagnosticConfigIdAspect::volatileValueToGui()
{
    if (m_widgetCount == 0 || !m_modelFactory || !m_editFactory)
        return;
    m_model = m_modelFactory();
    m_customConfigs = m_model.customConfigs();
    m_guiId = m_volatileValue;
    m_guiCustomConfigs = m_customConfigs;
    emit configsChanged(QPrivateSignal());
}

void ClangDiagnosticConfigIdAspect::fromMap(const Store &map)
{
    if (!settingsKey().isEmpty()) {
        const auto it = map.find(settingsKey());
        if (it != map.end())
            setValue(Id::fromSetting(it.value()), BeQuiet);
    }
}

void ClangDiagnosticConfigIdAspect::toMap(Store &map) const
{
    if (!settingsKey().isEmpty())
        map.insert(settingsKey(), value().toSetting());
}

void ClangDiagnosticConfigIdAspect::readSettings()
{
    TypedAspect<Id>::readSettings();
    if (m_persistCustomConfigs)
        m_customConfigs = diagnosticConfigsFromSettings(&Utils::userSettings());
}

void ClangDiagnosticConfigIdAspect::writeSettings() const
{
    TypedAspect<Id>::writeSettings();
    if (m_persistCustomConfigs)
        diagnosticConfigsToSettings(&Utils::userSettings(), m_customConfigs);
}

#ifdef WITH_TESTS
namespace Internal {

class ClangDiagnosticConfigIdAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    void testUpdatesEveryWidget()
    {
        const auto model = [] {
            ClangDiagnosticConfigsModel model;
            model.addBuiltinConfigs();
            return model;
        };
        QVERIFY(model().size() >= 2);
        const ClangDiagnosticConfig firstConfig = model().at(0);
        const ClangDiagnosticConfig secondConfig = model().at(1);

        ClangDiagnosticConfigIdAspect aspect;
        aspect.setDefaultValue(firstConfig.id());
        aspect.setModelFactory(model);
        aspect.setEditWidgetFactory([](const ClangDiagnosticConfigs &configs, const Id &id) {
            return new ClangDiagnosticConfigsWidget(configs, id);
        });
        QVERIFY(!aspect.hasWidget());

        auto first = std::make_unique<QWidget>();
        auto second = std::make_unique<QWidget>();
        Layouting::Column{aspect}.attachTo(first.get());
        Layouting::Column{aspect}.attachTo(second.get());
        QVERIFY(aspect.hasWidget());
        QPushButton *firstButton = first->findChild<QPushButton *>();
        QPushButton *secondButton = second->findChild<QPushButton *>();
        QVERIFY(firstButton);
        QVERIFY(secondButton);

        aspect.setValue(secondConfig.id());
        QCOMPARE(firstButton->text(), secondConfig.displayName());
        QCOMPARE(secondButton->text(), secondConfig.displayName());

        first.reset();
        QVERIFY(aspect.hasWidget());
        second.reset();
        QVERIFY(!aspect.hasWidget());
    }
};

QObject *createClangDiagnosticConfigIdAspectTest()
{
    return new ClangDiagnosticConfigIdAspectTest;
}

} // namespace Internal
#endif // WITH_TESTS

} // namespace CppEditor

#include "clangdiagnosticconfigidaspect.moc"
