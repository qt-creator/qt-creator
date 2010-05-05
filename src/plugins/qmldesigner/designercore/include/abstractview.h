/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ABSTRACTVIEW_H
#define ABSTRACTVIEW_H

#include <corelib_global.h>

#include <model.h>
#include <modelnode.h>
#include <abstractproperty.h>
#include <rewritertransaction.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QStyle;
QT_END_NAMESPACE

namespace QmlDesigner {
    namespace Internal {
        class InternalNode;
        typedef QSharedPointer<InternalNode> InternalNodePointer;
        typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;
    }
}

namespace QmlDesigner {

class QmlModelView;

class CORESHARED_EXPORT AbstractView : public QObject
{
    Q_OBJECT
public:
    Q_FLAGS(PropertyChangeFlag PropertyChangeFlags);
    typedef QWeakPointer<AbstractView> Pointer;

    enum PropertyChangeFlag {
      NoAdditionalChanges = 0x0,
      PropertiesAdded = 0x1,
      EmptyPropertiesRemoved = 0x2,
    };
    Q_DECLARE_FLAGS(PropertyChangeFlags, PropertyChangeFlag);
    AbstractView(QObject *parent = 0)
            : QObject(parent) {}

    virtual ~AbstractView();

    Model* model() const;

    RewriterTransaction beginRewriterTransaction();

    ModelNode createModelNode(const QString &typeString,
                         int majorVersion,
                         int minorVersion,
                         const PropertyListType &propertyList = PropertyListType());

    const ModelNode rootModelNode() const;
    ModelNode rootModelNode();

    void setSelectedModelNodes(const QList<ModelNode> &selectedNodeList);
    void selectModelNode(const ModelNode &node);
    void deselectModelNode(const ModelNode &node);
    void clearSelectedModelNodes();

    QList<ModelNode> selectedModelNodes() const;

    ModelNode modelNodeForId(const QString &id);
    bool hasId(const QString &id) const;

    QList<ModelNode> allModelNodes();

    void emitCustomNotification(const QString &identifier);
    void emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList);
    void emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    virtual void modelAttached(Model *model);
    virtual void modelAboutToBeDetached(Model *model);

    virtual void nodeCreated(const ModelNode &createdNode) = 0;
    virtual void nodeAboutToBeRemoved(const ModelNode &removedNode) = 0;
    virtual void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) = 0;
    virtual void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) = 0;
    virtual void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) = 0;
    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) = 0;
    virtual void propertiesRemoved(const QList<AbstractProperty>& propertyList) = 0;
    virtual void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) = 0;
    virtual void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) = 0;
    virtual void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) = 0;

    virtual void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList) = 0;

    virtual void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    virtual void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) = 0;

    virtual void importAdded(const Import &import);
    virtual void importRemoved(const Import &import);

    virtual void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    virtual void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    QmlModelView *toQmlModelView();

    void changeRootNodeType(const QString &type, int majorVersion, int minorVersion);

protected:
    void setModel(Model * model);
    void removeModel();

private: //functions
    QList<ModelNode> toModelNodeList(const QList<Internal::InternalNodePointer> &nodeList) const;


private:
    QWeakPointer<Model> m_model;
};

CORESHARED_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);
CORESHARED_EXPORT QList<ModelNode> toModelNodeList(const QList<Internal::InternalNodePointer> &nodeList, AbstractView *view);

}

#endif // ABSTRACTVIEW_H
