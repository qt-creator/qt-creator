/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    while (m_widgets.count() > 0)
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
