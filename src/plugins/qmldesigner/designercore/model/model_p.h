/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MODEL_P_H
#define MODEL_P_H

#include <QtCore/QList>
#include <QWeakPointer>
#include <QtCore/QSet>
#include <QtCore/QUrl>

#include "modelnode.h"
#include "abstractview.h"
#include "metainfo.h"

#include "corelib_global.h"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractProperty;
class RewriterView;
class NodeInstanceView;

namespace Internal {

class InternalNode;
class InternalProperty;
class InternalBindingProperty;
class InternalVariantProperty;
class InternalNodeAbstractProperty;
class InternalNodeListProperty;

typedef QSharedPointer<InternalNode> InternalNodePointer;
typedef QSharedPointer<InternalProperty> InternalPropertyPointer;
typedef QSharedPointer<InternalBindingProperty> InternalBindingPropertyPointer;
typedef QSharedPointer<InternalVariantProperty> InternalVariantPropertyPointer;
typedef QSharedPointer<InternalNodeAbstractProperty> InternalNodeAbstractPropertyPointer;
typedef QSharedPointer<InternalNodeListProperty> InternalNodeListPropertyPointer;
typedef QPair<InternalNodePointer, QString> PropertyPair;


class ModelPrivate;

class WriteLocker
{
public:
    ~WriteLocker();
    WriteLocker(ModelPrivate *model);
    WriteLocker(Model *model);
private: // variables
    QWeakPointer<ModelPrivate> m_model;
};

class ModelPrivate : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY(ModelPrivate)

    friend class QmlDesigner::Model;
    friend class QmlDesigner::Internal::WriteLocker;

public:
     ModelPrivate(Model *model);
    ~ModelPrivate();

    static Model *create(QString type, int major, int minor);

    QUrl fileUrl() const;
    void setFileUrl(const QUrl &url);

    InternalNodePointer createNode(const QString &typeString,
                                     int majorVersion,
                                     int minorVersion,
                                     const QList<QPair<QString, QVariant> > &propertyList,
                                     bool isRootNode = false);


    /*factory methods for internal use in model and rewriter*/

    void removeNode(const InternalNodePointer &node);
    void changeNodeId(const InternalNodePointer& internalNodePointer, const QString& id);

    InternalNodePointer rootNode() const;
    InternalNodePointer findNode(const QString &id) const;

    MetaInfo metaInfo() const;
    void setMetaInfo(const MetaInfo &metaInfo);

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, bool notifyView);
    void detachAllViews();


    Model *model() const { return m_q; }
    void setModel(Model *q) { m_q = q; }

    void notifyNodeCreated(const InternalNodePointer &newInternalNodePointer);
    void notifyNodeAboutToBeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeAboutToBeRemoved(const InternalNodePointer &nodePointer);
    void notifyNodeRemoved(const InternalNodePointer &nodePointer, const InternalNodePointer &parentNodePointer, const QString &parentPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeIdChanged(const InternalNodePointer& nodePointer, const QString& newId, const QString& oldId);

    void notifyPropertiesRemoved(const QList<PropertyPair> &propertyList);
    void notifyPropertiesAboutToBeRemoved(const QList<InternalPropertyPointer> &propertyList);
    void notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &propertyList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyVariantPropertiesChanged(const InternalNodePointer &internalNodePointer, const QStringList& propertyNameList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyScriptFunctionsChanged(const InternalNodePointer &internalNodePointer, const QStringList &scriptFunctionList);

    void notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListPropertyPointer, const InternalNodePointer &internalNodePointer, int oldIndex);
    void notifyAuxiliaryDataChanged(const InternalNodePointer &internalNode, const QString &name, const QVariant &data);

    void notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void notifyCustomNotification(const AbstractView *senderView, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void notifyInstancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void notifyInstancesCompleted(const QVector<ModelNode> &nodeList);


    void setSelectedNodes(const QList<InternalNodePointer> &selectedNodeList);
    void clearSelectedNodes();
    QList<InternalNodePointer> selectedNodes() const;
    void selectNode(const InternalNodePointer &node);
    void deselectNode(const InternalNodePointer &node);
    void changeSelectedNodes(const QList<InternalNodePointer> &newSelectedsNodeList,
                             const QList<InternalNodePointer> &oldSelectedsNodeList);

    void setAuxiliaryData(const InternalNodePointer& node, const QString &name, const QVariant &data);
    void resetModelByRewriter(const QString &description);


    // Imports:
    QList<Import> imports() const { return m_imports; }
    void addImport(const Import &import);
    void removeImport(const Import &import);
    void notifyImportAdded(const Import &import);
    void notifyImportRemoved(const Import &import);


    //node state property manipulation

    void addProperty(const InternalNodePointer &node, const QString &name);
    void setPropertyValue(const InternalNodePointer &node,const QString &name, const QVariant &value);
    void removeProperty(const InternalPropertyPointer &property);

    void setBindingProperty(const InternalNodePointer &internalNode, const QString &name, const QString &expression);
    void setVariantProperty(const InternalNodePointer &internalNode, const QString &name, const QVariant &value);
    void setDynamicVariantProperty(const InternalNodePointer &internalNode, const QString &name, const QString &propertyType, const QVariant &value);
    void setDynamicBindingProperty(const InternalNodePointer &internalNode, const QString &name, const QString &dynamicPropertyType, const QString &expression);
    void reparentNode(const InternalNodePointer &internalNode, const QString &name, const InternalNodePointer &internalNodeToBeAppended, bool list = true);
    void changeNodeOrder(const InternalNodePointer &internalParentNode, const QString &listPropertyName, int from, int to);
    void checkPropertyName(const QString &propertyName);
    void clearParent(const InternalNodePointer &internalNode);
    void changeRootNodeType(const QString &type, int majorVersion, int minorVersion);
    void setScriptFunctions(const InternalNodePointer &internalNode, const QStringList &scriptFunctionList);

    InternalNodePointer nodeForId(const QString &id) const;
    bool hasId(const QString &id) const;

    InternalNodePointer nodeForInternalId(qint32 internalId) const;
    bool hasNodeForInternalId(qint32 internalId) const;

    QList<InternalNodePointer> allNodes() const;

    bool isWriteLocked() const;

    WriteLocker createWriteLocker() const;

    void setRewriterView(RewriterView *rewriterView);
    RewriterView *rewriterView() const;
    void setNodeInstanceView(NodeInstanceView *nodeInstanceView);
    NodeInstanceView *nodeInstanceView() const;

private: //functions
    void removePropertyWithoutNotification(const InternalPropertyPointer &property);
    void removeAllSubNodes(const InternalNodePointer &node);
    void removeNodeFromModel(const InternalNodePointer &node);
    QList<InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList) const;
    QList<ModelNode> toModelNodeList(const QList<InternalNodePointer> &nodeList, AbstractView *view) const;
    QVector<ModelNode> toModelNodeVector(const QVector<InternalNodePointer> &nodeVector, AbstractView *view) const;
    QVector<InternalNodePointer> toInternalNodeVector(const QVector<ModelNode> &nodeVector) const;

private:
    Model *m_q;
    MetaInfo m_metaInfo;

    QList<Import> m_imports;
    QList<QWeakPointer<AbstractView> > m_viewList;
    QList<InternalNodePointer> m_selectedNodeList;
    QHash<QString,InternalNodePointer> m_idNodeHash;
    QHash<qint32, InternalNodePointer> m_internalIdNodeHash;
    QSet<InternalNodePointer> m_nodeSet;

    InternalNodePointer m_rootInternalNode;

    QUrl m_fileUrl;

    QWeakPointer<Model> m_masterModel;
    QWeakPointer<RewriterView> m_rewriterView;
    QWeakPointer<NodeInstanceView> m_nodeInstanceView;
    bool m_writeLock;
    qint32 m_internalIdCounter;
};

}
}
#endif // MODEL_P_H
