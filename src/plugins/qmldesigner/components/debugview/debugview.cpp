// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugview.h"
#include "debugviewwidget.h"

#include <qmldesignerplugin.h>

#include <bindingproperty.h>
#include <model/modelutils.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>

#include <qmlitemnode.h>

#include <utils/algorithm.h>
#include <utils/smallstringio.h>

namespace   {
const QString lineBreak = QStringLiteral("<br>");

bool isDebugViewEnabled()
{
    return QmlDesigner::QmlDesignerPlugin::settings().value(
        QmlDesigner::DesignerSettingsKey::ENABLE_DEBUGVIEW).toBool();
}

bool isDebugViewShown()
{
    return QmlDesigner::QmlDesignerPlugin::settings().value(
        QmlDesigner::DesignerSettingsKey::SHOW_DEBUGVIEW).toBool();
}

}

namespace QmlDesigner {

namespace  Internal {

DebugView::DebugView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_debugViewWidget(new DebugViewWidget)
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

void DebugView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
    if (isDebugViewEnabled()) {
        QString message;
        message += QString("added imports:") += lineBreak;
        for (const Import &import : addedImports) {
            message += import.toImportString() += lineBreak;
        }

        message += QString("removed imports:") += lineBreak;
        for (const Import &import : removedImports) {
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
        message << createdNode.majorVersion() << "." << createdNode.minorVersion();
        message << createdNode.nodeSource();
        message << "MetaInfo " << createdNode.metaInfo().isValid() << " ";
        if (auto metaInfo = createdNode.metaInfo()) {
            message << metaInfo.majorVersion() << "." << metaInfo.minorVersion();
            message << ModelUtils::componentFilePath(createdNode);
        }
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
        const QList<ModelNode> modelNodes = removedNode.allSubModelNodes();
        for (const ModelNode &modelNode : modelNodes) {
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

void DebugView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        for (const AbstractProperty &property : propertyList) {
            message << property;
            if (property.isNodeAbstractProperty())
                message << " is NodeAbstractProperty";
            if (property.isDefaultProperty())
                message << " is DefaultProperty";
        }
        log("::propertiesAboutToBeRemoved:", string);
    }
}

void DebugView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        for (const VariantProperty &property : propertyList) {
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
        for (const BindingProperty &property : propertyList) {
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
        for (const SignalHandlerProperty &property : propertyList) {
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

namespace {
QTextStream &operator<<(QTextStream &stream, AuxiliaryDataType type)
{
    switch (type) {
    case AuxiliaryDataType::None:
        stream << "None";
        break;
    case AuxiliaryDataType::NodeInstancePropertyOverwrite:
        stream << "NodeInstancePropertyOverwrite";
        break;
    case AuxiliaryDataType::NodeInstanceAuxiliary:
        stream << "NodeInstanceAuxiliary";
        break;
    case AuxiliaryDataType::Document:
        stream << "Permanent";
        break;
    case AuxiliaryDataType::Temporary:
        stream << "Temporary";
        break;
    }

    return stream;
}
} // namespace

static QString rectFToString(const QRectF &rect)
{
    return QString::number(rect.x()) + " " + QString::number(rect.y()) + " "
           + QString::number(rect.width()) + " " + QString::number(rect.height());
}

static QString sizeToString(const QSize &size)
{
    return QString::number(size.width()) + " " + QString::number(size.height());
}

void DebugView::selectedNodesChanged(const QList<ModelNode> &selectedNodes /*selectedNodeList*/,
                                     const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    for (const ModelNode &selectedNode : selectedNodes) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << selectedNode;
        message << " version: " << selectedNode.majorVersion() << '.' << selectedNode.minorVersion();
        for (const VariantProperty &property : selectedNode.variantProperties())
            message << property << lineBreak;

        message << lineBreak;

        for (const SignalHandlerProperty &property : selectedNode.signalProperties())
            message << property << lineBreak;

        message << lineBreak;

        message << lineBreak;
        message << "metaInfo valid: " << selectedNode.metaInfo().isValid();
        message << lineBreak;

        if (selectedNode.metaInfo().isValid()) {
            for (const NodeMetaInfo &metaInfo : selectedNode.metaInfo().selfAndPrototypes()) {
                message << metaInfo.typeName() << " " << metaInfo.majorVersion() << "."
                        << metaInfo.minorVersion() << lineBreak;
            }

            message << lineBreak;
            message << selectedNode.metaInfo().typeName();
            message << lineBreak;

            message << "Node Source" << selectedNode.nodeSource();
            message << lineBreak;

            message << "Is Component" << selectedNode.isComponent();
            message << lineBreak;

            message << "Node Source Type" << selectedNode.nodeSourceType();
            message << lineBreak;

            message << lineBreak;

            for (const PropertyName &name : selectedNode.metaInfo().slotNames())
                message << name << " ";

            message << lineBreak;
            message << "Is QtQuickItem: "
                    << selectedNode.metaInfo().isBasedOn(model()->qtQuickItemMetaInfo());
        }

        message << lineBreak;
        message << "Is item or window: " << QmlItemNode::isItemOrWindow(selectedNode);
        message << lineBreak;

        message << lineBreak;
        message << "Is graphical item: " << selectedNode.metaInfo().isGraphicalItem();
        message << lineBreak;

        message << lineBreak;
        message << "Is valid object node: " << QmlItemNode::isValidQmlObjectNode(selectedNode);
        message << lineBreak;

        message << "version: "
                << QString::number(selectedNode.metaInfo().majorVersion()) + "."
                       + QString::number(selectedNode.metaInfo().minorVersion());

        message << lineBreak;

        QmlItemNode itemNode(selectedNode);
        if (itemNode.isValid()) {
            message << lineBreak;
            message << "Item Node";
            message << lineBreak;
            message << "BR ";
            message << rectFToString(itemNode.instanceBoundingRect());
            message << lineBreak;
            message << "BR content ";
            message << rectFToString(itemNode.instanceContentItemBoundingRect());
            message << lineBreak;
            message << "PM ";
            message << sizeToString(itemNode.instanceRenderPixmap().size());
            message << lineBreak;
        }

        auto auxiliaryData = selectedNode.auxiliaryData();
        AuxiliaryDatas sortedaAuxiliaryData{auxiliaryData.begin(), auxiliaryData.end()};

        Utils::sort(sortedaAuxiliaryData, [](const auto &first, const auto &second) {
            return first.first < second.first;
        });
        for (const auto &element : sortedaAuxiliaryData) {
            message << element.first.type << ' ' << element.first.name.data() << ' '
                    << element.second.toString() << lineBreak;
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
        for (const AbstractProperty &property : propertyList) {
            message << property;
        }
        log("::propertiesRemoved:", string);
    }
}

void DebugView::auxiliaryDataChanged(const ModelNode &node,
                                     AuxiliaryDataKeyView key,
                                     const QVariant &data)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << node;
        message << key.type;
        message << QByteArray{key.name};
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

         for (const DocumentMessage &error : errors) {
             message << error.toString();
         }

         for (const DocumentMessage &warning : warnings) {
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
    return createWidgetInfo(m_debugViewWidget.data(),
                            QStringLiteral("DebugView"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("Debug View"));
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

        using Pair = QPair<ModelNode, PropertyName>;

        for (const Pair &pair : propertyList) {
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

        for (const ModelNode &modelNode : completedNodeList) {
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

        const QList<ModelNode> modelNodes = informationChangedHash.keys();
        for (const ModelNode &modelNode : modelNodes) {
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

        for (const ModelNode &modelNode : nodeList) {
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
        for (const ModelNode &node : nodeList) {
            message << node;
        }

        for (const QVariant &variant : data) {
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

void DebugView::currentStateChanged(const ModelNode &node)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << node;

        log("::currentStateChanged:", string);
    }
}

void DebugView::nodeOrderChanged([[maybe_unused]] const NodeListProperty &listProperty)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        log("::nodeSlide:", string);
    }
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
