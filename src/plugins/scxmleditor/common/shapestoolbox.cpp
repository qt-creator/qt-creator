// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "shapestoolbox.h"
#include "baseitem.h"
#include "scxmluifactory.h"
#include "shapegroupwidget.h"
#include "shapeprovider.h"
#include "ui_shapestoolbox.h"

#include <QDebug>
#include <QResizeEvent>
#include <QShowEvent>

#include <utils/qtcassert.h>

using namespace ScxmlEditor::Common;

ShapesToolbox::ShapesToolbox(QWidget *parent)
    : QFrame(parent)
{
    m_ui.setupUi(this);
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
            m_ui.m_shapeGrouplayout->addWidget(widget);
        }
    }

    m_ui.m_shapeGrouplayout->update();
    update();
}
