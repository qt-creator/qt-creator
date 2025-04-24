// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "designsysteminterface.h"
#include "qmldesignerprojectmanager.h"
#include <abstractview.h>

#include <memory>

namespace QmlDesigner {

class DSStore;
class ExternalDependenciesInterface;
class DesignSystemWidget;
class QmlDesignerProjectManager;

class DesignSystemView : public AbstractView
{
    Q_OBJECT

public:
    explicit DesignSystemView(ExternalDependenciesInterface &externalDependencies);
    ~DesignSystemView() override;

    WidgetInfo widgetInfo() override;
    bool hasWidget() const override;

    void modelAttached(Model *model) override;

private:
    void loadDesignSystem();
    void resetDesignSystem();
    QWidget *createViewWidget();

private:
    QPointer<DesignSystemWidget> m_designSystemWidget;

    QPointer<QWidget> m_dsWidget;
    ExternalDependenciesInterface &m_externalDependencies;
    std::unique_ptr<DSStore> m_dsStore;
    DesignSystemInterface m_dsInterface;
};

} // namespace QmlDesigner
