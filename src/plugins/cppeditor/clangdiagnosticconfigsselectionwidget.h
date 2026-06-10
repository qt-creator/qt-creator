// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "clangdiagnosticconfigsmodel.h"

#include <utils/aspects.h>
#include <utils/id.h>

#include <QPointer>
#include <QWidget>

#include <functional>

namespace Layouting { class Layout; }

QT_BEGIN_NAMESPACE
class QFormLayout;
class QPushButton;
QT_END_NAMESPACE

namespace CppEditor {

class ClangDiagnosticConfigsWidget;

class CPPEDITOR_EXPORT ClangDiagnosticConfigsSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClangDiagnosticConfigsSelectionWidget(QWidget *parent = nullptr);
    explicit ClangDiagnosticConfigsSelectionWidget(QFormLayout *parentLayout);

    using CreateEditWidget
        = std::function<ClangDiagnosticConfigsWidget *(const ClangDiagnosticConfigs &configs,
                                                       const Utils::Id &configToSelect)>;

    void refresh(const ClangDiagnosticConfigsModel &model,
                 const Utils::Id &configToSelect,
                 const CreateEditWidget &createEditWidget);

    Utils::Id currentConfigId() const;
    ClangDiagnosticConfigs customConfigs() const;

signals:
    void changed();

private:
    QString label() const;
    void setUpUi(bool withLabel);
    void onButtonClicked();

    ClangDiagnosticConfigsModel m_diagnosticConfigsModel;
    Utils::Id m_currentConfigId;
    bool m_showTidyClazyUi = true;

    QPushButton *m_button = nullptr;

    CreateEditWidget m_createEditWidget;
};

// Aspect holding a ClangDiagnosticConfig identifier. addToLayoutImpl() creates a
// ClangDiagnosticConfigsSelectionWidget driven by caller-supplied factories, so
// it participates fully in AspectContainer setEnabled() and setLayouter() machinery.
class CPPEDITOR_EXPORT DiagnosticConfigIdAspect final
    : public Utils::TypedAspect<Utils::Id>
{
public:
    explicit DiagnosticConfigIdAspect(Utils::AspectContainer *container = nullptr);

    using ModelFactory      = std::function<ClangDiagnosticConfigsModel()>;
    using EditWidgetFactory = std::function<ClangDiagnosticConfigsWidget *(
        const ClangDiagnosticConfigs &, const Utils::Id &)>;

    void setModelFactory(ModelFactory factory);
    void setEditWidgetFactory(EditWidgetFactory factory);

    ClangDiagnosticConfigs customConfigs() const { return m_customConfigs; }
    void setCustomConfigs(const ClangDiagnosticConfigs &configs) { m_customConfigs = configs; }

    void setPersistCustomConfigs(bool persist) { m_persistCustomConfigs = persist; }

    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;
    void readSettings() final;
    void writeSettings() const final;

    void addToLayoutImpl(Layouting::Layout &parent) final;

    void refresh();
    bool hasWidget() const { return m_widget != nullptr; }

private:
    bool guiToVolatileValue() final;
    void volatileValueToGui() final;
    bool isDirty() const final;
    void apply() final;

    ModelFactory      m_modelFactory;
    EditWidgetFactory m_editFactory;
    ClangDiagnosticConfigs m_customConfigs;
    ClangDiagnosticConfigs m_committedCustomConfigs;
    bool m_persistCustomConfigs = false;
    QPointer<ClangDiagnosticConfigsSelectionWidget> m_widget;
};

} // namespace CppEditor
