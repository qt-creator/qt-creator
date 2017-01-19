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
    return QmlDesigner::DesignerSettings::getValue(
        QmlDesigner::DesignerSettingsKey::ENABLE_DEBUGVIEW).toBool();
}

bool isDebugViewShown()
{
    return QmlDesigner::DesignerSettings::getValue(
        QmlDesigner::DesignerSettingsKey::SHOW_DEBUGVIEW).toBool();
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
    delete m_debugViewWidget;
}

void DebugView::modelAttached(Model *model)
{
    log("::modelAttached:", QString("filename %1").arg(model->fileUrl().toLocalFile()));
    m_debugViewWidget->setDebugViewEnabled(isDebugViewEnabled());
    if (isDebugViewEnabled())
        qDebug() << tr("Debug view is enabled");
    AbstractView::modelAttached(model);
}

void DebugView::modelAboutToBeDetached(Model *model)
{
    log("::modelAboutToBeDetached:", QString("filename %1").arg(model->fileUrl().toLocalFile()));
    AbstractView::modelAboutToBeDetached(model);
}

void DebugView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    if (isDebugViewEnabled()) {
        QString message;
        message += QString("added imports:") += lineBreak;
        foreach (const Import &import, addedImports) {
            message += import.toImportString() += lineBreak;
        }

        message += QString("removed imports:") += lineBreak;
        foreach (const Import &import, removedImports) {
            message += import.toImportString() += lineBreak;
        }

        log("::importsChanged:", message);
    }
}

void DebugView::nodeCreated(const ModelNode &createdNode)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << createdNode;
        log("::nodeCreated:", message.readAll());
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
            message << "child node:" << modelNode << lineBreak;
        }

        log("::nodeAboutToBeRemoved:", message.readAll());
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
        message << "new parent property:";
        message << lineBreak;
        message << newPropertyParent;
        message << "old parent property:";
        message << lineBreak;
        message << oldPropertyParent;
        message << "property change flag";
        message << lineBreak;
        message << propertyChange;
        log(tr("::nodeReparented:"), message.readAll());
    }
}

void DebugView::nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << node;
        message << QString("new id:") << ' ' << newId << lineBreak;
        message << QString("old id:") << ' ' << oldId << lineBreak;
        log(tr("::nodeIdChanged:"), string);
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
        log("::variantPropertiesChanged:", string);
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
        log("::Binding properties changed:", string);
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
        log("::signalHandlerPropertiesChanged:", string);
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
        log("::rootNodeTypeChanged:", message);
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
        log("::selectedNodesChanged:", string);
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
        log("::propertiesRemoved:", string);
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

        log("::auxiliaryDataChanged:", string);
    }
}

void DebugView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings)
{
     if (isDebugViewEnabled()) {
         QTextStream message;
         QString string;
         message.setString(&string);

         foreach (const DocumentMessage &error, errors) {
             message << error.toString();
         }

         foreach (const DocumentMessage &warning, warnings) {
             message << warning.toString();
         }

         log("::documentMessageChanged:", string);
     }
}

void DebugView::rewriterBeginTransaction()
{
    if (isDebugViewEnabled())
        log("::rewriterBeginTransaction:", QString(), true);
}

void DebugView::rewriterEndTransaction()
{
    if (isDebugViewEnabled())
        log("::rewriterEndTransaction:", QString(), true);
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

void DebugView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
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

        logInstance(":instancePropertyChanged::", string);
    }
}

void DebugView::instanceErrorChanged(const QVector<ModelNode> &/*errorNodeList*/)
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
                message << "parent: " << QmlItemNode(modelNode).instanceParent() << lineBreak;
            }
        }

        logInstance(":instancesCompleted::", string);
    }
}

void DebugView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        foreach (const ModelNode &modelNode, informationChangedHash.keys()) {
            message << modelNode;
            message << informationChangedHash.value(modelNode);
        }

        logInstance("::instanceInformationsChanged:", string);
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
                message << "parent: " << QmlItemNode(modelNode).instanceParent() << lineBreak;
            }
        }

        logInstance("::instancesChildrenChanged:", string);
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

        log("::customNotification:", string);
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

        log("::nodeSourceChanged:", string);
    }
}

void DebugView::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &/*parentProperty*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << removedNode;

        log("::nodeRemoved:", string);
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
