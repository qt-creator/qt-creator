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

#include "abstractview.h"

#include "model.h"
#include "model_p.h"
#include "internalnode_p.h"
#include "nodeinstanceview.h"
#include <qmlstate.h>

#include <coreplugin/helpmanager.h>
#include <utils/qtcassert.h>

#include <QRegExp>

namespace QmlDesigner {


/*!
\class QmlDesigner::AbstractView
\ingroup CoreModel
\brief The AbstractView class provides an abstract interface that views and
editors can implement to be notified about model changes.

\sa QmlDesigner::WidgetQueryView(), QmlDesigner::NodeInstanceView()
*/

AbstractView::~AbstractView()
{
    if (m_model)
        m_model.data()->detachView(this, Model::DoNotNotifyView);
}

/*!
    Sets the view of a new \a model. This is handled automatically by
    AbstractView::modelAttached().

    \sa AbstractView::modelAttached()
*/
void AbstractView::setModel(Model *model)
{
    Q_ASSERT(model != 0);
    if (model == m_model.data())
        return;

    if (m_model)
        m_model.data()->detachView(this);

    m_model = model;
}

RewriterTransaction AbstractView::beginRewriterTransaction(const QByteArray &identifier)
{
    return RewriterTransaction(this, identifier);
}

ModelNode AbstractView::createModelNode(const TypeName &typeName,
                            int majorVersion,
                            int minorVersion,
                            const QList<QPair<PropertyName, QVariant> > &propertyList,
                            const QList<QPair<PropertyName, QVariant> > &auxPropertyList,
                            const QString &nodeSource,
                            ModelNode::NodeSourceType nodeSourceType)
{
    return ModelNode(model()->d->createNode(typeName, majorVersion, minorVersion, propertyList, auxPropertyList, nodeSource, nodeSourceType), model(), this);
}


/*!
    Returns the constant root model node.
*/

const ModelNode AbstractView::rootModelNode() const
{
    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), const_cast<AbstractView*>(this));
}


/*!
    Returns the root model node.
*/

ModelNode AbstractView::rootModelNode()
{
    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), this);
}

/*!
    Sets the reference to a model to a null pointer.

*/
void AbstractView::removeModel()
{
    m_model.clear();
}

WidgetInfo AbstractView::createWidgetInfo(QWidget *widget,
                                          WidgetInfo::ToolBarWidgetFactoryInterface *toolBarWidgetFactory,
                                          const QString &uniqueId,
                                          WidgetInfo::PlacementHint placementHint,
                                          int placementPriority,
                                          const QString &tabName,
                                          DesignerWidgetFlags widgetFlags)
{
    WidgetInfo widgetInfo;

    widgetInfo.widget = widget;
    widgetInfo.toolBarWidgetFactory = toolBarWidgetFactory;
    widgetInfo.uniqueId = uniqueId;
    widgetInfo.placementHint = placementHint;
    widgetInfo.placementPriority = placementPriority;
    widgetInfo.tabName = tabName;
    widgetInfo.widgetFlags = widgetFlags;

    return widgetInfo;
}

/*!
    Returns the model of the view.
*/
Model* AbstractView::model() const
{
    return m_model.data();
}

bool AbstractView::isAttached() const
{
    return model();
}

/*!
Called if a view is being attached to \a model.
The default implementation is setting the reference of the model to the view.
\sa Model::attachView()
*/
void AbstractView::modelAttached(Model *model)
{
    setModel(model);
}

/*!
Called before a view is detached from \a model.

This function is not called if Model::detachViewWithOutNotification is used.
The default implementation
is removing the reference to the model from the view.

\sa Model::detachView()
*/
void AbstractView::modelAboutToBeDetached(Model *)
{
    removeModel();
}

/*!
    \enum QmlDesigner::AbstractView::PropertyChangeFlag

    Notifies about changes in the abstract properties of a node:

    \value  NoAdditionalChanges
            No changes were made.

    \value  PropertiesAdded
            Some properties were added.

    \value  EmptyPropertiesRemoved
            Empty properties were removed.
*/

void AbstractView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &/*propertyList*/)
{
}

void AbstractView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{
}

void AbstractView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void AbstractView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void AbstractView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void AbstractView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{
}

void AbstractView::nodeSourceChanged(const ModelNode &/*modelNode*/, const QString &/*newNodeSource*/)
{
}

void AbstractView::rewriterBeginTransaction()
{
}

void AbstractView::rewriterEndTransaction()
{
}

void AbstractView::instanceErrorChanged(const QVector<ModelNode> &/*errorNodeList*/)
{
}

void AbstractView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

// Node related functions

/*!
\fn void AbstractView::nodeCreated(const ModelNode &createdNode)
Called when the new node \a createdNode is created.
*/
void AbstractView::nodeCreated(const ModelNode &/*createdNode*/)
{
}

void AbstractView::currentStateChanged(const ModelNode &/*node*/)
{
}

/*!
Called when the file URL (that is needed to resolve relative paths against,
for example) is changed form \a oldUrl to \a newUrl.
*/
void AbstractView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/)
{
}

void AbstractView::nodeOrderChanged(const NodeListProperty &/*listProperty*/, const ModelNode &/*movedNode*/, int /*oldIndex*/)
{
}

/*!
\fn void AbstractView::nodeAboutToBeRemoved(const ModelNode &removedNode)
Called when the node specified by \a removedNode will be removed.
*/
void AbstractView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
}

void AbstractView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

/*!
Called when the properties specified by \a propertyList are removed.
*/
void AbstractView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

/*!
\fn void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
Called when the parent of \a node will be changed from \a oldPropertyParent to
\a newPropertyParent.
*/

/*!
\fn void QmlDesigner::AbstractView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                                         const QList<ModelNode> &lastSelectedNodeList)
Called when the selection is changed from \a lastSelectedNodeList to
\a selectedNodeList.
*/
void AbstractView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{
}

void AbstractView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::nodeIdChanged(const ModelNode& /*node*/, const QString& /*newId*/, const QString& /*oldId*/)
{
}

void AbstractView::variantPropertiesChanged(const QList<VariantProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::bindingPropertiesChanged(const QList<BindingProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void AbstractView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
}

void AbstractView::nodeTypeChanged(const ModelNode & /*node*/, const TypeName & /*type*/, int /*majorVersion*/, int /*minorVersion*/)
{

}

void AbstractView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
}

void AbstractView::auxiliaryDataChanged(const ModelNode &/*node*/, const PropertyName &/*name*/, const QVariant &/*data*/)
{
}

void AbstractView::customNotification(const AbstractView * /*view*/, const QString & /*identifier*/, const QList<ModelNode> & /*nodeList*/, const QList<QVariant> & /*data*/)
{
}

void AbstractView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{
}

void AbstractView::documentMessagesChanged(const QList<DocumentMessage> &/*errors*/, const QList<DocumentMessage> &/*warnings*/)
{
}

QList<ModelNode> AbstractView::toModelNodeList(const QList<Internal::InternalNode::Pointer> &nodeList) const
{
    return QmlDesigner::toModelNodeList(nodeList, const_cast<AbstractView*>(this));
}

QList<ModelNode> toModelNodeList(const QList<Internal::InternalNode::Pointer> &nodeList, AbstractView *view)
{
    QList<ModelNode> newNodeList;
    foreach (const Internal::InternalNode::Pointer &node, nodeList)
        newNodeList.append(ModelNode(node, view->model(), view));

    return newNodeList;
}

QList<Internal::InternalNode::Pointer> toInternalNodeList(const QList<ModelNode> &nodeList)
{
    QList<Internal::InternalNode::Pointer> newNodeList;
    foreach (const ModelNode &node, nodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
}

/*!
    Sets the list of nodes to the actual selected nodes specified by
    \a selectedNodeList.
*/
void AbstractView::setSelectedModelNodes(const QList<ModelNode> &selectedNodeList)
{
    model()->d->setSelectedNodes(toInternalNodeList(selectedNodeList));
}

void AbstractView::setSelectedModelNode(const ModelNode &modelNode)
{
    setSelectedModelNodes({modelNode});
}

/*!
    Clears the selection.
*/
void AbstractView::clearSelectedModelNodes()
{
    model()->d->clearSelectedNodes();
}

bool AbstractView::hasSelectedModelNodes() const
{
    return !model()->d->selectedNodes().isEmpty();
}

bool AbstractView::hasSingleSelectedModelNode() const
{
    return model()->d->selectedNodes().count() == 1;
}

bool AbstractView::isSelectedModelNode(const ModelNode &modelNode) const
{
    return model()->d->selectedNodes().contains(modelNode.internalNode());
}

/*!
    Sets the list of nodes to the actual selected nodes. Returns a list of the
    selected nodes.
*/
QList<ModelNode> AbstractView::selectedModelNodes() const
{
    return toModelNodeList(model()->d->selectedNodes());
}

ModelNode AbstractView::firstSelectedModelNode() const
{
    if (hasSelectedModelNodes())
        return ModelNode(model()->d->selectedNodes().first(), model(), this);

    return ModelNode();
}

ModelNode AbstractView::singleSelectedModelNode() const
{
    if (hasSingleSelectedModelNode())
        return ModelNode(model()->d->selectedNodes().first(), model(), this);

    return ModelNode();
}

/*!
    Adds \a node to the selection list.
*/
void AbstractView::selectModelNode(const ModelNode &modelNode)
{
    QTC_ASSERT(modelNode.isInHierarchy(), return);
    model()->d->selectNode(modelNode.internalNode());
}

/*!
    Removes \a node from the selection list.
*/
void AbstractView::deselectModelNode(const ModelNode &node)
{
    model()->d->deselectNode(node.internalNode());
}

ModelNode AbstractView::modelNodeForId(const QString &id)
{
    return ModelNode(model()->d->nodeForId(id), model(), this);
}

bool AbstractView::hasId(const QString &id) const
{
    return model()->d->hasId(id);
}

QString firstCharToLower(const QString &string)
{
    QString resultString = string;

    if (!resultString.isEmpty())
        resultString[0] = resultString.at(0).toLower();

    return resultString;
}

QString AbstractView::generateNewId(const QString &prefixName) const
{
    int counter = 1;

    /* First try just the prefixName without number as postfix, then continue with 2 and further as postfix
     * until id does not already exist.
     * Properties of the root node are not allowed for ids, because they are available in the complete context
     * without qualification.
     * The id "item" is explicitly not allowed, because it is too likely to clash.
    */

    QString newId = QString(QStringLiteral("%1")).arg(firstCharToLower(prefixName));
    newId.remove(QRegExp(QStringLiteral("[^a-zA-Z0-9_]")));

    while (!ModelNode::isValidId(newId) || hasId(newId) || rootModelNode().hasProperty(newId.toUtf8()) || newId == "item") {
        counter += 1;
        newId = QString(QStringLiteral("%1%2")).arg(firstCharToLower(prefixName)).arg(counter - 1);
        newId.remove(QRegExp(QStringLiteral("[^a-zA-Z0-9_]")));
    }

    return newId;
}

ModelNode AbstractView::modelNodeForInternalId(qint32 internalId) const
{
     return ModelNode(model()->d->nodeForInternalId(internalId), model(), this);
}

bool AbstractView::hasModelNodeForInternalId(qint32 internalId) const
{
    return model()->d->hasNodeForInternalId(internalId);
}

NodeInstanceView *AbstractView::nodeInstanceView() const
{
    if (model())
        return model()->d->nodeInstanceView();
    else
        return 0;
}

RewriterView *AbstractView::rewriterView() const
{
    if (model())
        return model()->d->rewriterView();
    else
        return 0;
}

void AbstractView::resetView()
{
    if (!model())
        return;
    Model *currentModel = model();

    currentModel->detachView(this);
    currentModel->attachView(this);
}

void AbstractView::resetPuppet()
{
    emitCustomNotification(QStringLiteral("reset QmlPuppet"));
}

bool AbstractView::hasWidget() const
{
    return false;
}

WidgetInfo AbstractView::widgetInfo()
{
    return createWidgetInfo();
}

QString AbstractView::contextHelpId() const
{
    QString helpId;

    if (hasSelectedModelNodes()) {
        QString className = firstSelectedModelNode().simplifiedTypeName();
        helpId = QStringLiteral("QML.") + className;
        if (Core::HelpManager::linksForIdentifier(helpId).isEmpty() && firstSelectedModelNode().metaInfo().isValid()) {

            foreach (className, firstSelectedModelNode().metaInfo().superClassNames()) {
                helpId = QStringLiteral("QML.") + className;
                if (Core::HelpManager::linksForIdentifier(helpId).isEmpty())
                    helpId = QString();
                else
                    break;
            }
        }
    }

    return helpId;
}

QList<ModelNode> AbstractView::allModelNodes() const
{
    return toModelNodeList(model()->d->allNodes());
}

void AbstractView::emitDocumentMessage(const QString &error)
{
    emitDocumentMessage({DocumentMessage(error)});
}

void AbstractView::emitDocumentMessage(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings)
{
    model()->d->setDocumentMessages(errors, warnings);
}

void AbstractView::emitCustomNotification(const QString &identifier)
{
    emitCustomNotification(identifier, QList<ModelNode>());
}

void AbstractView::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList)
{
    emitCustomNotification(identifier, nodeList, QList<QVariant>());
}

void AbstractView::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    model()->d->notifyCustomNotification(this, identifier, nodeList, data);
}

void AbstractView::emitInstancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancePropertyChange(propertyList);
}

void AbstractView::emitInstanceErrorChange(const QVector<qint32> &instanceIds)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstanceErrorChange(instanceIds);
}

void AbstractView::emitInstancesCompleted(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesCompleted(nodeVector);
}

void AbstractView::emitInstanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesInformationsChange(informationChangeHash);
}

void AbstractView::emitInstancesRenderImageChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesRenderImageChanged(nodeVector);
}

void AbstractView::emitInstancesPreviewImageChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesPreviewImageChanged(nodeVector);
}

void AbstractView::emitInstancesChildrenChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesChildrenChanged(nodeVector);
}

void AbstractView::emitRewriterBeginTransaction()
{
    if (model())
        model()->d->notifyRewriterBeginTransaction();
}

void AbstractView::sendTokenToInstances(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    if (nodeInstanceView())
        nodeInstanceView()->sendToken(token, number, nodeVector);
}

void AbstractView::emitInstanceToken(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    if (nodeInstanceView())
        model()->d->notifyInstanceToken(token, number, nodeVector);
}

void AbstractView::emitRewriterEndTransaction()
{
    if (model())
        model()->d->notifyRewriterEndTransaction();
}

void AbstractView::setCurrentStateNode(const ModelNode &node)
{
    Internal::WriteLocker locker(m_model.data());
    if (model())
        model()->d->notifyCurrentStateChanged(node);
}

void AbstractView::changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion)
{
    Internal::WriteLocker locker(m_model.data());

    m_model.data()->d->changeRootNodeType(type, majorVersion, minorVersion);
}

ModelNode AbstractView::currentStateNode() const
{
    if (model())
        return ModelNode(m_model.data()->d->currentStateNode(), m_model.data(), const_cast<AbstractView*>(this));

    return ModelNode();
}

QmlModelState AbstractView::currentState() const
{
    return QmlModelState(currentStateNode());
}

static int getMinorVersionFromImport(const Model *model)
{
    foreach (const Import &import, model->imports()) {
        if (import.isLibraryImport() && import.url() == "QtQuick") {
            const QString versionString = import.version();
            if (versionString.contains(".")) {
                const QString minorVersionString = versionString.split(".").last();
                return minorVersionString.toInt();
            }
        }
    }

    return -1;
}

static int getMajorVersionFromImport(const Model *model)
{
    foreach (const Import &import, model->imports()) {
        if (import.isLibraryImport() && import.url() == QStringLiteral("QtQuick")) {
            const QString versionString = import.version();
            if (versionString.contains(QStringLiteral("."))) {
                const QString majorVersionString = versionString.split(QStringLiteral(".")).first();
                return majorVersionString.toInt();
            }
        }
    }

    return -1;
}

static int getMajorVersionFromNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isValid()) {
        if (modelNode.type() == "QtQuick.QtObject" || modelNode.type() == "QtQuick.Item")
            return modelNode.majorVersion();

        foreach (const NodeMetaInfo &superClass,  modelNode.metaInfo().superClasses()) {
            if (modelNode.type() == "QtQuick.QtObject" || modelNode.type() == "QtQuick.Item")
                return superClass.majorVersion();
        }
    }

    return 1; //default
}

static int getMinorVersionFromNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isValid()) {
        if (modelNode.type() == "QtQuick.QtObject" || modelNode.type() == "QtQuick.Item")
            return modelNode.minorVersion();

        foreach (const NodeMetaInfo &superClass,  modelNode.metaInfo().superClasses()) {
            if (modelNode.type() == "QtQuick.QtObject" || modelNode.type() == "QtQuick.Item")
                return superClass.minorVersion();
        }
    }

    return 1; //default
}

int AbstractView::majorQtQuickVersion() const
{
    int majorVersionFromImport = getMajorVersionFromImport(model());
    if (majorVersionFromImport >= 0)
        return majorVersionFromImport;

    return getMajorVersionFromNode(rootModelNode());
}

int AbstractView::minorQtQuickVersion() const
{
    int minorVersionFromImport = getMinorVersionFromImport(model());
    if (minorVersionFromImport >= 0)
        return minorVersionFromImport;

    return getMinorVersionFromNode(rootModelNode());
}


} // namespace QmlDesigner
