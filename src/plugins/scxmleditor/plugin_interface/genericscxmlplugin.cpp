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

#include "genericscxmlplugin.h"
#include "mytypes.h"
#include "scattributeitemdelegate.h"
#include "scattributeitemmodel.h"
#include "scgraphicsitemprovider.h"
#include "scshapeprovider.h"
#include "scutilsprovider.h"
#include "scxmluifactory.h"

using namespace ScxmlEditor::PluginInterface;

GenericScxmlPlugin::GenericScxmlPlugin(QObject *parent)
    : QObject(parent)
{
}

GenericScxmlPlugin::~GenericScxmlPlugin()
{
    delete m_attributeItemDelegate;
    delete m_attributeItemModel;
    delete m_graphicsItemProvider;
    delete m_shapeProvider;
    delete m_utilsProvider;
}

void GenericScxmlPlugin::documentChanged(DocumentChangeType type, ScxmlDocument *doc)
{
    Q_UNUSED(type)
    Q_UNUSED(doc)
}

void GenericScxmlPlugin::detach()
{
    m_factory->unregisterObject("attributeItemDelegate", m_attributeItemDelegate);
    m_factory->unregisterObject("attributeItemModel", m_attributeItemModel);
    m_factory->unregisterObject("graphicsItemProvider", m_graphicsItemProvider);
    m_factory->unregisterObject("shapeProvider", m_shapeProvider);
    m_factory->unregisterObject("utilsProvider", m_utilsProvider);
}

void GenericScxmlPlugin::init(ScxmlUiFactory *factory)
{
    m_factory = factory;

    m_attributeItemDelegate = new SCAttributeItemDelegate;
    m_attributeItemModel = new SCAttributeItemModel;
    m_graphicsItemProvider = new SCGraphicsItemProvider;
    m_shapeProvider = new SCShapeProvider;
    m_utilsProvider = new SCUtilsProvider;

    m_factory->registerObject("attributeItemDelegate", m_attributeItemDelegate);
    m_factory->registerObject("attributeItemModel", m_attributeItemModel);
    m_factory->registerObject("graphicsItemProvider", m_graphicsItemProvider);
    m_factory->registerObject("shapeProvider", m_shapeProvider);
    m_factory->registerObject("utilsProvider", m_utilsProvider);
}

void GenericScxmlPlugin::refresh()
{
}
