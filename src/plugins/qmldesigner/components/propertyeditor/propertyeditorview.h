/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef PROPERTYEDITORVIEW_H
#define PROPERTYEDITORVIEW_H

#include <abstractview.h>
#include <QHash>
#include <QTimer>

#include "propertyeditorwidget.h"

QT_BEGIN_NAMESPACE
class QShortcut;
class QStackedWidget;
class QTimer;
QT_END_NAMESPACE

namespace QmlDesigner {

class PropertyEditorTransaction;
class CollapseButton;
class PropertyEditorWidget;
class PropertyEditorView;
class PropertyEditorQmlBackend;

class PropertyEditorView: public AbstractView
{
    Q_OBJECT

public:
    PropertyEditorView(QWidget *parent = 0);
    ~PropertyEditorView();

    bool hasWidget() const;
    WidgetInfo widgetInfo();

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) Q_DECL_OVERRIDE;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) Q_DECL_OVERRIDE;

    void propertiesRemoved(const QList<AbstractProperty>& propertyList) Q_DECL_OVERRIDE;

    void modelAttached(Model *model) Q_DECL_OVERRIDE;

    void modelAboutToBeDetached(Model *model) Q_DECL_OVERRIDE;

    ModelState modelState() const;

    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) Q_DECL_OVERRIDE;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) Q_DECL_OVERRIDE;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList,PropertyChangeFlags propertyChange) Q_DECL_OVERRIDE;

    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash) Q_DECL_OVERRIDE;

    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) Q_DECL_OVERRIDE;
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) Q_DECL_OVERRIDE;

    void resetView();
    void currentStateChanged(const ModelNode &node) Q_DECL_OVERRIDE;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) Q_DECL_OVERRIDE;

    void nodeCreated(const ModelNode &createdNode) Q_DECL_OVERRIDE;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) Q_DECL_OVERRIDE;
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, PropertyChangeFlags propertyChange) Q_DECL_OVERRIDE;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, PropertyChangeFlags propertyChange) Q_DECL_OVERRIDE;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList) Q_DECL_OVERRIDE;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) Q_DECL_OVERRIDE;
    void instancesCompleted(const QVector<ModelNode> &completedNodeList) Q_DECL_OVERRIDE;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) Q_DECL_OVERRIDE;
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) Q_DECL_OVERRIDE;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) Q_DECL_OVERRIDE;
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector) Q_DECL_OVERRIDE;
    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) Q_DECL_OVERRIDE;
    void rewriterBeginTransaction() Q_DECL_OVERRIDE;
    void rewriterEndTransaction() Q_DECL_OVERRIDE;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) Q_DECL_OVERRIDE;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) Q_DECL_OVERRIDE;

protected:
    void timerEvent(QTimerEvent *event);
    void setupPane(const TypeName &typeName);
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);

private slots:
    void reloadQml();
    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void updateSize();
    void setupPanes();

private: //functions

    void select(const ModelNode& node);

    void delayedResetView();
    void setupQmlBackend();

private: //variables
    ModelNode m_selectedNode;
    QWidget *m_parent;
    QShortcut *m_updateShortcut;
    int m_timerId;
    PropertyEditorWidget* m_stackedWidget;
    QString m_qmlDir;
    QHash<QString, PropertyEditorQmlBackend *> m_qmlBackendHash;
    PropertyEditorQmlBackend *m_qmlBackEndForCurrentType;
    bool m_locked;
    bool m_setupCompleted;
    QTimer *m_singleShotTimer;
};

} //QmlDesigner

#endif // PROPERTYEDITORVIEW_H
