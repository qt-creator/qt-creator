// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shapestoolbox.h"
#include "baseitem.h"
#include "scxmluifactory.h"
#include "shapegroupwidget.h"
#include "shapeprovider.h"

#include <QScrollArea>
#include <QVBoxLayout>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

using namespace ScxmlEditor::Common;

ShapesToolbox::ShapesToolbox(QWidget *parent)
    : QFrame(parent)
{
    auto scrollArea = new QScrollArea;
    scrollArea->setFrameShape(NoFrame);
    scrollArea->setWidgetResizable(true);

    auto shapeGroupsContainer = new QWidget;
    scrollArea->setWidget(shapeGroupsContainer);

    m_shapeGroupsLayout = new QVBoxLayout(shapeGroupsContainer);
    m_shapeGroupsLayout->setContentsMargins(0, 0, 0, 0);
    m_shapeGroupsLayout->setSpacing(0);

    using namespace Layouting;
    Column {
        spacing(0),
        scrollArea,
        noMargin,
    }.attachTo(this);
}

void ShapesToolbox::setUIFactory(ScxmlEditor::PluginInterface::ScxmlUiFactory *factory)
{
    QTC_ASSERT(factory, return);

    m_shapeProvider = static_cast<PluginInterface::ShapeProvider*>(factory->object("shapeProvider"));
    connect(m_shapeProvider.data(), &PluginInterface::ShapeProvider::changed, this, &ShapesToolbox::initView);
    initView();
}

void ShapesToolbox::initView()
{
    // Delete old widgets
    while (!m_widgets.isEmpty())
        delete m_widgets.takeLast();

    // Create new widgets
    if (m_shapeProvider) {
        for (int i = 0; i < m_shapeProvider->groupCount(); ++i) {
            auto widget = new ShapeGroupWidget(m_shapeProvider, i);
            m_widgets << widget;
            m_shapeGroupsLayout->addWidget(widget);
        }
    }

    m_shapeGroupsLayout->addStretch(1);
    m_shapeGroupsLayout->update();
    update();
}
