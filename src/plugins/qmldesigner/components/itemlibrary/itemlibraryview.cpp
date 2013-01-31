/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "itemlibraryview.h"
#include "itemlibrarywidget.h"
#include <import.h>

namespace QmlDesigner {

ItemLibraryView::ItemLibraryView(QObject* parent) : AbstractView(parent)
{

}

ItemLibraryView::~ItemLibraryView()
{

}

QWidget *ItemLibraryView::widget()
{
    if (m_widget.isNull())
        m_widget = new ItemLibraryWidget;

    return m_widget.data();
}

void ItemLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    m_widget->setModel(model);
    updateImports();
}

void ItemLibraryView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    m_widget->setModel(0);
}

void ItemLibraryView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    updateImports();
}

void ItemLibraryView::nodeCreated(const ModelNode &)
{

}

void ItemLibraryView::nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags)
{

}

void ItemLibraryView::propertiesRemoved(const QList<AbstractProperty> &)
{

}

void ItemLibraryView::variantPropertiesChanged(const QList<VariantProperty> &, PropertyChangeFlags)
{

}

void ItemLibraryView::bindingPropertiesChanged(const QList<BindingProperty> &, PropertyChangeFlags)
{

}

void ItemLibraryView::nodeAboutToBeRemoved(const ModelNode &)
{

}

void ItemLibraryView::nodeOrderChanged(const NodeListProperty &, const ModelNode &, int)
{

}

void ItemLibraryView::nodeAboutToBeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{

}

void ItemLibraryView::nodeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{

}

void ItemLibraryView::rootNodeTypeChanged(const QString &, int , int )
{

}

void ItemLibraryView::nodeIdChanged(const ModelNode &, const QString &, const QString& )
{

}

void ItemLibraryView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& )
{

}

void ItemLibraryView::selectedNodesChanged(const QList<ModelNode> &,
                                  const QList<ModelNode> &)
{

}

void ItemLibraryView::auxiliaryDataChanged(const ModelNode &, const QString &, const QVariant &)
{

}

void ItemLibraryView::scriptFunctionsChanged(const ModelNode &, const QStringList &)
{

}

void ItemLibraryView::instancePropertyChange(const QList<QPair<ModelNode, QString> > &)
{

}

void ItemLibraryView::instancesCompleted(const QVector<ModelNode> &)
{

}

void ItemLibraryView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{
}

void ItemLibraryView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void ItemLibraryView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void ItemLibraryView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void ItemLibraryView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{

}

void ItemLibraryView::nodeSourceChanged(const ModelNode &, const QString & /*newNodeSource*/)
{

}

void ItemLibraryView::rewriterBeginTransaction()
{
}

void ItemLibraryView::rewriterEndTransaction()
{
}

void ItemLibraryView::actualStateChanged(const ModelNode &/*node*/)
{
}

void ItemLibraryView::setResourcePath(const QString &resourcePath)
{
    if (m_widget.isNull())
        m_widget = new ItemLibraryWidget;

    m_widget->setResourcePath(resourcePath);
}

void ItemLibraryView::updateImports()
{
    m_widget->updateModel();
}

} //QmlDesigner
