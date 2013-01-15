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

#include "qmlmodelview.h"
#include "qmlobjectnode.h"
#include "qmlitemnode.h"
#include "itemlibraryinfo.h"
#include "mathutils.h"
#include "invalididexception.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <utils/fileutils.h>
#include "nodeabstractproperty.h"
#include "variantproperty.h"
#include "rewritingexception.h"
#include "rewriterview.h"
#include "plaintexteditmodifier.h"
#include "modelmerger.h"
#include "nodemetainfo.h"

#include <utils/qtcassert.h>

namespace QmlDesigner {

QmlModelView::QmlModelView(QObject *parent)
    : AbstractView(parent)
{
}

void QmlModelView::setCurrentState(const QmlModelState &state)
{
    if (!state.isValid())
        return;

    if (!model())
        return;

    if (actualStateNode() != state.modelNode())
        setAcutalStateNode(state.modelNode());
}

QmlModelState QmlModelView::currentState() const
{
    return QmlModelState(actualStateNode());
}

QmlModelState QmlModelView::baseState() const
{
    return QmlModelState::createBaseState(this);
}

QmlModelStateGroup QmlModelView::rootStateGroup() const
{
    return QmlModelStateGroup(rootModelNode());
}

QmlObjectNode QmlModelView::createQmlObjectNode(const QString &typeString,
                     int majorVersion,
                     int minorVersion,
                     const PropertyListType &propertyList)
{
    return QmlObjectNode(createModelNode(typeString, majorVersion, minorVersion, propertyList));
}

QmlItemNode QmlModelView::createQmlItemNode(const QString &typeString,
                                int majorVersion,
                                int minorVersion,
                                const PropertyListType &propertyList)
{
    return createQmlObjectNode(typeString, majorVersion, minorVersion, propertyList).toQmlItemNode();
}

QmlItemNode QmlModelView::createQmlItemNodeFromImage(const QString &imageName, const QPointF &position, QmlItemNode parentNode)
{
    if (!parentNode.isValid())
        parentNode = rootQmlItemNode();

    if (!parentNode.isValid())
        return QmlItemNode();

    QmlItemNode newNode;
    RewriterTransaction transaction = beginRewriterTransaction();
    {
        const QString newImportUrl = QLatin1String("QtQuick");
        const QString newImportVersion = QLatin1String("1.1");
        Import newImport = Import::createLibraryImport(newImportUrl, newImportVersion);

        foreach (const Import &import, model()->imports()) {
            if (import.isLibraryImport()
                && import.url() == newImport.url()
                && import.version() == newImport.version()) {
                // reuse this import
                newImport = import;
                break;
            }
        }

        if (!model()->imports().contains(newImport))
            model()->changeImports(QList<Import>() << newImport, QList<Import>());

        QList<QPair<QString, QVariant> > propertyPairList;
        propertyPairList.append(qMakePair(QString("x"), QVariant( round(position.x(), 4))));
        propertyPairList.append(qMakePair(QString("y"), QVariant( round(position.y(), 4))));

        QString relativeImageName = imageName;

        //use relative path
        if (QFileInfo(model()->fileUrl().toLocalFile()).exists()) {
            QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
            relativeImageName = fileDir.relativeFilePath(imageName);
        }

        propertyPairList.append(qMakePair(QString("source"), QVariant(relativeImageName)));
        newNode = createQmlItemNode("QtQuick.Image", -1, -1, propertyPairList);
        parentNode.nodeAbstractProperty("data").reparentHere(newNode);

        Q_ASSERT(newNode.isValid());

        QString id;
        int i = 1;
        QString name("image");
        name.remove(QLatin1Char(' '));
        do {
            id = name + QString::number(i);
            i++;
        } while (hasId(id)); //If the name already exists count upwards

        newNode.setId(id);
        if (!currentState().isBaseState()) {
            newNode.modelNode().variantProperty("opacity") = 0;
            newNode.setVariantProperty("opacity", 1);
        }

        Q_ASSERT(newNode.isValid());
    }

    Q_ASSERT(newNode.isValid());

    return newNode;
}

QmlItemNode QmlModelView::createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry, const QPointF &position, QmlItemNode parentNode)
{
    if (!parentNode.isValid())
        parentNode = rootQmlItemNode();

    Q_ASSERT(parentNode.isValid());

    QmlItemNode newNode;

    try {
        RewriterTransaction transaction = beginRewriterTransaction();

        NodeMetaInfo metaInfo = model()->metaInfo(itemLibraryEntry.typeName());

        int minorVersion = metaInfo.minorVersion();
        int majorVersion = metaInfo.majorVersion();

        if (itemLibraryEntry.typeName().contains('.')) {

            const QString newImportUrl = itemLibraryEntry.requiredImport();

            if (!itemLibraryEntry.requiredImport().isEmpty()) {
                const QString newImportVersion = QString("%1.%2").arg(QString::number(itemLibraryEntry.majorVersion()), QString::number(itemLibraryEntry.minorVersion()));

                Import newImport = Import::createLibraryImport(newImportUrl, newImportVersion);
                if (itemLibraryEntry.majorVersion() == -1 && itemLibraryEntry.minorVersion() == -1)
                    newImport = Import::createFileImport(newImportUrl, QString());
                else
                    newImport = Import::createLibraryImport(newImportUrl, newImportVersion);

                foreach (const Import &import, model()->imports()) {
                    if (import.isLibraryImport()
                            && import.url() == newImport.url()
                            && import.version() == newImport.version()) {
                        // reuse this import
                        newImport = import;
                        break;
                    }
                }

                if (!model()->hasImport(newImport, true, true))
                    model()->changeImports(QList<Import>() << newImport, QList<Import>());
            }
        }

        QList<QPair<QString, QVariant> > propertyPairList;
        propertyPairList.append(qMakePair(QString("x"), QVariant(round(position.x(), 4))));
        propertyPairList.append(qMakePair(QString("y"), QVariant(round(position.y(), 4))));

        if (itemLibraryEntry.qml().isEmpty()) {
            foreach (const PropertyContainer &property, itemLibraryEntry.properties())
                propertyPairList.append(qMakePair(property.name(), property.value()));

            newNode = createQmlItemNode(itemLibraryEntry.typeName(), majorVersion, minorVersion, propertyPairList);
        } else {
            QScopedPointer<Model> inputModel(Model::create("QtQuick.Rectangle", 1, 0, model()));
            inputModel->setFileUrl(model()->fileUrl());
            QPlainTextEdit textEdit;


            textEdit.setPlainText(Utils::FileReader::fetchQrc(itemLibraryEntry.qml()));
            NotIndentingTextEditModifier modifier(&textEdit);

            QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, 0));
            rewriterView->setCheckSemanticErrors(false);
            rewriterView->setTextModifier(&modifier);
            inputModel->setRewriterView(rewriterView.data());

            if (rewriterView->errors().isEmpty() && rewriterView->rootModelNode().isValid()) {
                ModelNode rootModelNode = rewriterView->rootModelNode();
                inputModel->detachView(rewriterView.data());

                rootModelNode.variantProperty("x") = propertyPairList.first().second;
                rootModelNode.variantProperty("y") = propertyPairList.at(1).second;

                ModelMerger merger(this);
                newNode = merger.insertModel(rootModelNode);
            }
        }

        if (parentNode.hasDefaultProperty())
            parentNode.nodeAbstractProperty(parentNode.defaultProperty()).reparentHere(newNode);

        if (!newNode.isValid())
            return newNode;

        QString id;
        int i = 1;
        QString name(itemLibraryEntry.name().toLower());
        //remove forbidden characters
        name.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_]")), QLatin1String("_"));
        do {
            id = name + QString::number(i);
            i++;
        } while (hasId(id)); //If the name already exists count upwards

        newNode.setId(id);

        if (!currentState().isBaseState()) {
            newNode.modelNode().variantProperty("opacity") = 0;
            newNode.setVariantProperty("opacity", 1);
        }

        Q_ASSERT(newNode.isValid());
    }
    catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
    catch (InvalidIdException &e) {
        // should never happen
        QMessageBox::warning(0, tr("Invalid Id"), e.description());
    }

    Q_ASSERT(newNode.isValid());

    return newNode;
}

QmlObjectNode QmlModelView::rootQmlObjectNode() const
{
    return QmlObjectNode(rootModelNode());
}

QmlItemNode QmlModelView::rootQmlItemNode() const
{
    return rootQmlObjectNode().toQmlItemNode();
}

void QmlModelView::setSelectedQmlObjectNodes(const QList<QmlObjectNode> &selectedNodeList)
{
    setSelectedModelNodes(QmlDesigner::toModelNodeList(selectedNodeList));
}

void QmlModelView::selectQmlObjectNode(const QmlObjectNode &node)
{
     selectModelNode(node.modelNode());
}

void QmlModelView::deselectQmlObjectNode(const QmlObjectNode &node)
{
    deselectModelNode(node.modelNode());
}

QList<QmlObjectNode> QmlModelView::selectedQmlObjectNodes() const
{
    return toQmlObjectNodeList(selectedModelNodes());
}

QList<QmlItemNode> QmlModelView::selectedQmlItemNodes() const
{
    return toQmlItemNodeList(selectedModelNodes());
}

void QmlModelView::setSelectedQmlItemNodes(const QList<QmlItemNode> &selectedNodeList)
{
    return setSelectedModelNodes(QmlDesigner::toModelNodeList(selectedNodeList));
}

QmlObjectNode QmlModelView::fxObjectNodeForId(const QString &id)
{
    return QmlObjectNode(modelNodeForId(id));
}

NodeInstance QmlModelView::instanceForModelNode(const ModelNode &modelNode)
{
    return nodeInstanceView()->instanceForNode(modelNode);
}

bool QmlModelView::hasInstanceForModelNode(const ModelNode &modelNode)
{
    return nodeInstanceView() && nodeInstanceView()->hasInstanceForNode(modelNode);
}

void QmlModelView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
}

void QmlModelView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

static bool isTransformProperty(const QString &name)
{
    static QStringList transformProperties(QStringList() << "x"
                                                         << "y"
                                                         << "width"
                                                         << "height"
                                                         << "z"
                                                         << "rotation"
                                                         << "scale"
                                                         << "transformOrigin"
                                                         << "paintedWidth"
                                                         << "paintedHeight"
                                                         << "border.width");

    return transformProperties.contains(name);
}

void QmlModelView::nodeOrderChanged(const NodeListProperty &/*listProperty*/, const ModelNode &/*movedNode*/, int /*oldIndex*/)
{

}

void QmlModelView::nodeCreated(const ModelNode &/*createdNode*/) {}
void QmlModelView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/) {}
void QmlModelView::nodeRemoved(const ModelNode &/*removedNode*/, const NodeAbstractProperty &/*parentProperty*/, PropertyChangeFlags /*propertyChange*/) {}
void QmlModelView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {}
void QmlModelView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {}
void QmlModelView::nodeIdChanged(const ModelNode& /*node*/, const QString& /*newId*/, const QString& /*oldId*/) {}
void QmlModelView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/) {}
void QmlModelView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/) {}
void QmlModelView::variantPropertiesChanged(const QList<VariantProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/) {}
void QmlModelView::bindingPropertiesChanged(const QList<BindingProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/) {}
void QmlModelView::rootNodeTypeChanged(const QString &/*type*/, int, int /*minorVersion*/) {}
void QmlModelView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/) {}
void QmlModelView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/) {}

void QmlModelView::instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList)
{
    typedef QPair<ModelNode, QString> ModelNodePropertyPair;
    foreach (const ModelNodePropertyPair &propertyPair, propertyList) {
        nodeInstancePropertyChanged(propertyPair.first, propertyPair.second);
    }
}
void QmlModelView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

void QmlModelView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{

}

void QmlModelView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void QmlModelView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void QmlModelView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void QmlModelView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{

}

void QmlModelView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{

}

void QmlModelView::nodeSourceChanged(const ModelNode &, const QString & /*newNodeSource*/)
{

}

void QmlModelView::rewriterBeginTransaction()
{

}

void QmlModelView::rewriterEndTransaction()
{

}

void QmlModelView::actualStateChanged(const ModelNode & /*node*/)
{

}

void QmlModelView::nodeInstancePropertyChanged(const ModelNode &node, const QString &propertyName)
{
    QmlObjectNode qmlObjectNode(node);

    if (!qmlObjectNode.isValid())
        return;

    if (isTransformProperty(propertyName))
        transformChanged(qmlObjectNode, propertyName);
    else if (propertyName == "parent")
        parentChanged(qmlObjectNode);
    else
        otherPropertyChanged(qmlObjectNode, propertyName);
}

void QmlModelView::transformChanged(const QmlObjectNode &/*qmlObjectNode*/, const QString &/*propertyName*/)
{
}

void QmlModelView::parentChanged(const QmlObjectNode &/*qmlObjectNode*/)
{
}

void QmlModelView::otherPropertyChanged(const QmlObjectNode &/*qmlObjectNode*/, const QString &/*propertyName*/)
{
}

ModelNode QmlModelView::createQmlState(const QmlDesigner::PropertyListType &propertyList)
{

    QTC_CHECK(rootModelNode().majorQtQuickVersion() < 3);

    if (rootModelNode().majorQtQuickVersion() > 1)
        return createModelNode("QtQuick.State", 2, 0, propertyList);
    else
        return createModelNode("QtQuick.State", 1, 0, propertyList);
}


} //QmlDesigner
