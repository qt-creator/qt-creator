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

#include "itemlibraryview.h"
#include "itemlibrarywidget.h"
#include <import.h>
#include <importmanagerview.h>
#include <qmlitemnode.h>
#include <rewriterview.h>
#include <bindingproperty.h>
#include <nodelistproperty.h>
#include <utils/algorithm.h>
#include "metainfo.h"

namespace QmlDesigner {

ItemLibraryView::ItemLibraryView(QObject* parent)
    : AbstractView(parent),
      m_importManagerView(new ImportManagerView(this))

{

}

ItemLibraryView::~ItemLibraryView() = default;

bool ItemLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo ItemLibraryView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new ItemLibraryWidget;
        m_widget->setImportsWidget(m_importManagerView->widgetInfo().widget);
    }

    return createWidgetInfo(m_widget.data(),
                            new WidgetInfo::ToolBarWidgetDefaultFactory<ItemLibraryWidget>(m_widget.data()),
                            QStringLiteral("Library"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("Library"));
}

void ItemLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    m_widget->clearSearchFilter();
    m_widget->setModel(model);
    updateImports();
    model->attachView(m_importManagerView);
    m_hasErrors = !rewriterView()->errors().isEmpty();
    m_widget->setFlowMode(QmlItemNode(rootModelNode()).isFlowView());
}

void ItemLibraryView::modelAboutToBeDetached(Model *model)
{
    model->detachView(m_importManagerView);

    AbstractView::modelAboutToBeDetached(model);

    m_widget->setModel(nullptr);
}

void ItemLibraryView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    updateImports();

    // TODO: generalize the logic below to allow adding/removing any Qml component when its import is added/removed
    bool simulinkImportAdded = std::any_of(addedImports.cbegin(), addedImports.cend(), [](const Import &import) {
        return import.url() == "SimulinkConnector";
    });
    if (simulinkImportAdded) {
        // add SLConnector component when SimulinkConnector import is added
        ModelNode node = createModelNode("SLConnector", 1, 0);
        node.bindingProperty("root").setExpression(rootModelNode().validId());
        rootModelNode().defaultNodeListProperty().reparentHere(node);
    } else {
        bool simulinkImportRemoved = std::any_of(removedImports.cbegin(), removedImports.cend(), [](const Import &import) {
            return import.url() == "SimulinkConnector";
        });

        if (simulinkImportRemoved) {
            // remove SLConnector component when SimulinkConnector import is removed
            const QList<ModelNode> slConnectors = Utils::filtered(rootModelNode().directSubModelNodes(),
                                                                  [](const ModelNode &node) {
                return node.type() == "SLConnector" || node.type() == "SimulinkConnector.SLConnector";
            });

            for (ModelNode node : slConnectors)
                node.destroy();
        }
    }
}

void ItemLibraryView::setResourcePath(const QString &resourcePath)
{
    if (m_widget.isNull())
        m_widget = new ItemLibraryWidget;

    m_widget->setResourcePath(resourcePath);
}

void ItemLibraryView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    if (m_hasErrors && errors.isEmpty())
        updateImports();

     m_hasErrors = !errors.isEmpty();
}

void ItemLibraryView::updateImports()
{
    m_widget->delayedUpdateModel();
}

} //QmlDesigner
