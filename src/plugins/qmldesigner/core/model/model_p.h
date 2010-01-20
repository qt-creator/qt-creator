/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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

class ModelPrivate : QObject {
    Q_OBJECT

    Q_DISABLE_COPY(ModelPrivate)

    friend class QmlDesigner::Model;

public:
     ModelPrivate(Model *model);
    ~ModelPrivate();

    static Model *create(QString type, int major, int minor);

    QUrl fileUrl() const;
    void setFileUrl(const QUrl &url);

    InternalNodePointer createNode(const QString &typeString,
                                     int majorVersion,
                                     int minorVersion,
                                     const QList<QPair<QString, QVariant> > &propertyList);


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
    void notifyNodeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeAboutToBeRemoved(const InternalNodePointer &nodePointer);
    void notifyNodeRemoved(const InternalNodePointer &nodePointer, const InternalNodePointer &parentNodePointer, const QString &parentPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeIdChanged(const InternalNodePointer& nodePointer, const QString& newId, const QString& oldId);

    void notifyPropertiesRemoved(const QList<PropertyPair> &propertyList);
    void notifyPropertiesAboutToBeRemoved(const QList<InternalPropertyPointer> &propertyList);
    void notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &propertyList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyVariantPropertiesChanged(const InternalNodePointer &internalNodePointer, const QStringList& propertyNameList, AbstractView::PropertyChangeFlags propertyChange);

    void notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListPropertyPointer, const InternalNodePointer &internalNodePointer, int oldIndex);
    void notifyAuxiliaryDataChanged(const InternalNodePointer &internalNode, const QString &name, const QVariant &data);

    void notifyNodeTypeChanged(const InternalNodePointer &internalNode, const QString &type, int majorVersion, int minorVersion);

    void notifyCustomNotification(const AbstractView *senderView, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    void setSelectedNodes(const QList<InternalNodePointer> &selectedNodeList);
    void clearSelectedNodes();
    QList<InternalNodePointer> selectedNodes() const;
    void selectNode(const InternalNodePointer &node);
    void deselectNode(const InternalNodePointer &node);
    void changeSelectedNodes(const QList<InternalNodePointer> &newSelectedsNodeList,
                             const QList<InternalNodePointer> &oldSelectedsNodeList);

    void setRootNode(const InternalNodePointer& newRootNode);
    void setAuxiliaryData(const InternalNodePointer& node, const QString &name, const QVariant &data);
    void resetModelByRewriter();


    // Imports:
    QSet<Import> imports() const { return m_imports; }
    void setImports(const QSet<Import> &imports);
    void addImport(const Import &import);
    void removeImport(const Import &import);
    void notifyImportsChanged() const;


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
    void changeType(const InternalNodePointer & internalNode, const QString &type, int majorVersion, int minorVersion);

    InternalNodePointer nodeForId(const QString &id) const;
    bool hasId(const QString &id) const;

    QList<InternalNodePointer> allNodes() const;

private: //functions
    void removePropertyWithoutNotification(const InternalPropertyPointer &property);
    void removeAllSubNodes(const InternalNodePointer &node);
    void removeNodeFromModel(const InternalNodePointer &node);
    QList<InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList) const;
    QList<ModelNode> toModelNodeList(const QList<InternalNodePointer> &nodeList, AbstractView *view) const;

private:
    Model *m_q;
    MetaInfo m_metaInfo;

    QSet<Import> m_imports;
    QList<QWeakPointer<AbstractView> > m_viewList;
    QList<InternalNodePointer> m_selectedNodeList;
    QHash<QString,InternalNodePointer> m_idNodeHash;
    QSet<InternalNodePointer> m_nodeSet;

    InternalNodePointer m_rootInternalNode;

    QUrl m_fileUrl;

    QWeakPointer<Model> m_masterModel;
};

}
}
#endif // MODEL_P_H
