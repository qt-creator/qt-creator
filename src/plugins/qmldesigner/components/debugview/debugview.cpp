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

#include "debugview.h"
#include "debugviewwidget.h"

#include <qmldesignerplugin.h>

#include <bindingproperty.h>
#include <signalhandlerproperty.h>
#include <nodeabstractproperty.h>
#include <variantproperty.h>

#include <qmlitemnode.h>

namespace   {
const QString lineBreak = QStringLiteral("<br>");

bool isDebugViewEnabled()
{
    return (QmlDesigner::QmlDesignerPlugin::instance()->settings().enableDebugView);
}

bool isDebugViewShown()
{
    return (QmlDesigner::QmlDesignerPlugin::instance()->settings().showDebugView);
}

}

namespace QmlDesigner {

namespace  Internal {

DebugView::DebugView(QObject *parent) : AbstractView(parent),
    m_debugViewWidget(new DebugViewWidget)
{
}

DebugView::~DebugView()
{
}

void DebugView::modelAttached(Model *model)
{
    log(tr("Model attached"), tr("FileName %1").arg(model->fileUrl().toLocalFile()));
    m_debugViewWidget->setDebugViewEnabled(isDebugViewEnabled());
    if (isDebugViewEnabled())
        qDebug() << tr("DebugView is enabled");
    AbstractView::modelAttached(model);
}

void DebugView::modelAboutToBeDetached(Model *model)
{
    log(tr("Model detached"), tr("FileName %1").arg(model->fileUrl().toLocalFile()));
    AbstractView::modelAboutToBeDetached(model);
}

void DebugView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    if (isDebugViewEnabled()) {
        QString message;
        message += tr("Added imports:") += lineBreak;
        foreach (const Import &import, addedImports) {
            message += import.toImportString() += lineBreak;
        }

        message += tr("Removed imports:") += lineBreak;
        foreach (const Import &import, removedImports) {
            message += import.toImportString() += lineBreak;
        }

        log(tr("Imports changed:"), message);
    }
}

void DebugView::nodeCreated(const ModelNode &createdNode)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << createdNode;
        log(tr("Node created:"), message.readAll());
    }
}

void DebugView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << removedNode << lineBreak;
        foreach (const ModelNode &modelNode, removedNode.allSubModelNodes()) {
            message << tr("ChildNode:") << modelNode << lineBreak;
        }

        log(tr("Node about to be removed:"), message.readAll());
    }
}

void DebugView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                               const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << node;
        message << tr("New parent property:");
        message << lineBreak;
        message << newPropertyParent;
        message << tr("Old parent property:");
        message << lineBreak;
        message << oldPropertyParent;
        message << tr("PropertyChangeFlag");
        message << lineBreak;
        message << propertyChange;
        log(tr("Node reparanted:"), message.readAll());
    }
}

void DebugView::nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << node;
        message << tr("New Id:") << ' ' << newId << lineBreak;
        message << tr("Old Id:") << ' ' << oldId << lineBreak;
        log(tr("Node id changed:"), string);
    }
}

void DebugView::propertiesAboutToBeRemoved(const QList<AbstractProperty> & /*propertyList*/)
{
}

void DebugView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const VariantProperty &property, propertyList) {
            message << property;
        }
        log(tr("VariantProperties changed:"), string);
    }
}

void DebugView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const BindingProperty &property, propertyList) {
            message << property;
        }
        log(tr("BindingProperties changed:"), string);
    }
}

void DebugView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const SignalHandlerProperty &property, propertyList) {
            message << property;
        }
        log(tr("SignalHandlerProperties changed:"), string);
    }
}

void DebugView::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    if (isDebugViewEnabled()) {
        QString message;
        message += type;
        message += QStringLiteral(" ");
        message += QString::number(majorVersion);
        message += QStringLiteral(" ");
        message += QString::number(minorVersion);
        log(tr("Node id changed:"), message);
    }
}

void DebugView::selectedNodesChanged(const QList<ModelNode> &selectedNodes /*selectedNodeList*/,
                                     const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    foreach (const ModelNode &selectedNode, selectedNodes) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << selectedNode;
        foreach (const VariantProperty &property, selectedNode.variantProperties()) {
            message << property;
        }
        log(tr("Node selected:"), string);
    }
}

void DebugView::scriptFunctionsChanged(const ModelNode & /*node*/, const QStringList & /*scriptFunctionList*/)
{
}

void DebugView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const AbstractProperty &property, propertyList) {
            message << property;
        }
        log(tr("Properties removed:"), string);
    }
}

void DebugView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << node;
        message << name;
        message << data.toString();

        log(tr("Auxiliary Data Changed:"), string);
    }
}

void DebugView::rewriterBeginTransaction()
{
    if (isDebugViewEnabled())
        log(tr("Begin rewriter transaction"), QString(), true);
}

void DebugView::rewriterEndTransaction()
{
    if (isDebugViewEnabled())
        log(tr("End rewriter transaction"), QString(), true);
}

WidgetInfo DebugView::widgetInfo()
{
    return createWidgetInfo(m_debugViewWidget.data(), 0, QStringLiteral("DebugView"), WidgetInfo::LeftPane, 0, tr("Debug View"));
}

bool DebugView::hasWidget() const
{
    if (isDebugViewShown())
        return true;

    return false;
}

void DebugView::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        typedef QPair<ModelNode, PropertyName> Pair;

        foreach (const Pair &pair, propertyList) {
            message << pair.first;
            message << lineBreak;
            message << pair.second;
        }

        logInstance(tr("Instance property change"), string);
    }
}

void DebugView::instanceErrorChange(const QVector<ModelNode> &/*errorNodeList*/)
{
}

void DebugView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        foreach (const ModelNode &modelNode, completedNodeList) {
            message << modelNode << lineBreak;
            if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                message << tr("parent: ") << QmlItemNode(modelNode).instanceParent() << lineBreak;
            }
        }

        logInstance(tr("Instance Completed"), string);
    }
}

void DebugView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        foreach (const ModelNode &modelNode, informationChangeHash.keys()) {
            message << modelNode;
            message << informationChangeHash.value(modelNode);
        }

        logInstance(tr("Instance Completed"), string);
    }

}

void DebugView::instancesRenderImageChanged(const QVector<ModelNode> & /*nodeList*/)
{
}

void DebugView::instancesPreviewImageChanged(const QVector<ModelNode> & /*nodeList*/)
{
}

void DebugView::instancesChildrenChanged(const QVector<ModelNode> & nodeList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        foreach (const ModelNode &modelNode, nodeList) {
            message << modelNode << lineBreak;
            if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                message << tr("parent: ") << QmlItemNode(modelNode).instanceParent() << lineBreak;
            }
        }

        logInstance(tr("Instances children changed:"), string);
    }
}

void DebugView::customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << view;
        message << identifier;
        foreach (const ModelNode &node, nodeList) {
            message << node;
        }

        foreach (const QVariant &variant, data) {
            message << variant.toString();
        }

        log(tr("Custom Notification:"), string);
    }
}

void DebugView::nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << modelNode;
        message << newNodeSource;

        log(tr("Node Source Changed:"), string);
    }
}

void DebugView::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &/*parentProperty*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << removedNode;

        log(tr("Node Removed:"), string);
    }
}

void DebugView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{

}

void DebugView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{

}

void DebugView::currentStateChanged(const ModelNode &/*node*/)
{

}

void DebugView::nodeOrderChanged(const NodeListProperty &/*listProperty*/, const ModelNode &/*movedNode*/, int /*oldIndex*/)
{

}

void DebugView::log(const QString &title, const QString &message, bool highlight)
{
    m_debugViewWidget->addLogMessage(title, message, highlight);
}

void DebugView::logInstance(const QString &title, const QString &message, bool highlight)
{
    m_debugViewWidget->addLogInstanceMessage(title, message, highlight);
}

} // namesapce Internal

} // namespace QmlDesigner
