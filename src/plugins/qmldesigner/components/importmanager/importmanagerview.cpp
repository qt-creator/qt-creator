/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "importmanagerview.h"
#include "importswidget.h"

#include <rewritingexception.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

ImportManagerView::ImportManagerView(QObject *parent) :
    AbstractView(parent),
    m_importsWidget(0)

{
}

ImportManagerView::~ImportManagerView()
{

}

bool ImportManagerView::hasWidget() const
{
    return true;
}

WidgetInfo ImportManagerView::widgetInfo()
{
    if (m_importsWidget == 0) {
        m_importsWidget = new ImportsWidget;
        connect(m_importsWidget, SIGNAL(removeImport(Import)), this, SLOT(removeImport(Import)));
        connect(m_importsWidget, SIGNAL(addImport(Import)), this, SLOT(addImport(Import)));

        if (model())
            m_importsWidget->setImports(model()->imports());
    }

    return createWidgetInfo(m_importsWidget, 0, "ImportManager", WidgetInfo::LeftPane, 1);
}

void ImportManagerView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if (m_importsWidget) {
        m_importsWidget->setImports(model->imports());
        m_importsWidget->setPossibleImports(model->possibleImports());
        m_importsWidget->setUsedImports(model->usedImports());
    }
}

void ImportManagerView::modelAboutToBeDetached(Model *model)
{
    if (m_importsWidget) {
        m_importsWidget->removeImports();
        m_importsWidget->removePossibleImports();
        m_importsWidget->removeUsedImports();
    }

    AbstractView::modelAboutToBeDetached(model);
}

void ImportManagerView::nodeCreated(const ModelNode &/*createdNode*/)
{
    if (m_importsWidget)
        m_importsWidget->setUsedImports(model()->usedImports());
}

void ImportManagerView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
    if (m_importsWidget)
        m_importsWidget->setUsedImports(model()->usedImports());
}

void ImportManagerView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::nodeIdChanged(const ModelNode &/*node*/, const QString &/*newId*/, const QString &/*oldId*/)
{

}

void ImportManagerView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &/*propertyList*/)
{

}

void ImportManagerView::propertiesRemoved(const QList<AbstractProperty> &/*propertyList*/)
{

}

void ImportManagerView::variantPropertiesChanged(const QList<VariantProperty> &/*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::bindingPropertiesChanged(const QList<BindingProperty> &/*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &/*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void ImportManagerView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{

}

void ImportManagerView::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &/*propertyList*/)
{

}

void ImportManagerView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{

}

void ImportManagerView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{

}

void ImportManagerView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void ImportManagerView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void ImportManagerView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void ImportManagerView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{

}

void ImportManagerView::nodeSourceChanged(const ModelNode &/*modelNode*/, const QString &/*newNodeSource*/)
{

}

void ImportManagerView::rewriterBeginTransaction()
{

}

void ImportManagerView::rewriterEndTransaction()
{

}

void ImportManagerView::currentStateChanged(const ModelNode &/*node*/)
{

}

void ImportManagerView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{

}

void ImportManagerView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/)
{

}

void ImportManagerView::nodeOrderChanged(const NodeListProperty &/*listProperty*/, const ModelNode &/*movedNode*/, int /*oldIndex*/)
{

}

void ImportManagerView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    if (m_importsWidget) {
        m_importsWidget->setImports(model()->imports());
        m_importsWidget->setPossibleImports(model()->possibleImports());
        m_importsWidget->setUsedImports(model()->usedImports());
    }
}

void ImportManagerView::auxiliaryDataChanged(const ModelNode &/*node*/, const PropertyName &/*name*/, const QVariant &/*data*/)
{

}

void ImportManagerView::customNotification(const AbstractView * /*view*/, const QString &/*identifier*/, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{

}

void ImportManagerView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{

}

void ImportManagerView::removeImport(const Import &import)
{
    try {
        if (model())
            model()->changeImports(QList<Import>(), QList<Import>() << import);
    }
    catch (const RewritingException &e) {
        e.showException();
    }
}

void ImportManagerView::addImport(const Import &import)
{
    try {
        if (model())
            model()->changeImports(QList<Import>() << import, QList<Import>());
    }
    catch (const RewritingException &e) {
        e.showException();
    }

    QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
}

} // namespace QmlDesigner
