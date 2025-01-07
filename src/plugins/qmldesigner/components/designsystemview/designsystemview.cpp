// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designsystemview.h"
#include "coreplugin/messagebox.h"
#include "designsystemwidget.h"
#include "dsstore.h"

#include <coreplugin/icore.h>

#include <QPushButton>
#include <QQuickWidget>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

namespace QmlDesigner {

DesignSystemView::DesignSystemView(ExternalDependenciesInterface &externalDependencies,
                                   ProjectStorageDependencies projectStorageDependencies)
    : AbstractView(externalDependencies)
    , m_externalDependencies(externalDependencies)
    , m_dsStore(std::make_unique<DSStore>(m_externalDependencies, projectStorageDependencies))
    , m_dsInterface(m_dsStore.get())
{}

DesignSystemView::~DesignSystemView() {}

WidgetInfo DesignSystemView::widgetInfo()
{
    if (!m_designSystemWidget)
        m_designSystemWidget = new DesignSystemWidget(this, &m_dsInterface);

    return createWidgetInfo(m_designSystemWidget,
                            "DesignSystemView",
                            WidgetInfo::RightPane,
                            tr("Design System"));
}

bool DesignSystemView::hasWidget() const
{
    return true;
}

void DesignSystemView::loadDesignSystem()
{
    if (auto err = m_dsStore->load())
        qDebug() << *err;
}

} // namespace QmlDesigner
