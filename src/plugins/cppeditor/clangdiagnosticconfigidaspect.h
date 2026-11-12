// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "clangdiagnosticconfigsmodel.h"

#include <utils/aspects.h>
#include <utils/id.h>

namespace CppEditor {

namespace Internal { QObject *createClangDiagnosticConfigIdAspectTest(); }

class ClangDiagnosticConfigsWidget;

// Aspect holding a ClangDiagnosticConfig identifier.

class CPPEDITOR_EXPORT ClangDiagnosticConfigIdAspect final
    : public Utils::TypedAspect<Utils::Id>
{
    Q_OBJECT

public:
    explicit ClangDiagnosticConfigIdAspect(Utils::AspectContainer *container = nullptr);

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
    bool hasWidget() const;

signals:
    void configsChanged(QPrivateSignal);

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

    // What the widgets show. They edit it through their dialog.
    ClangDiagnosticConfigsModel m_model;
    Utils::Id m_guiId;
    ClangDiagnosticConfigs m_guiCustomConfigs;
    int m_widgetCount = 0;
};

} // namespace CppEditor
