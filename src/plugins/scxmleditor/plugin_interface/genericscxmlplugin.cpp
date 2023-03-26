// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
