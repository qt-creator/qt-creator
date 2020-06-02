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

#pragma once

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
class ModelNode;

class PropertyEditorView: public AbstractView
{
    Q_OBJECT

public:
    PropertyEditorView(QWidget *parent = nullptr);
    ~PropertyEditorView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;

    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;

    void modelAttached(Model *model) override;

    void modelAboutToBeDetached(Model *model) override;

    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    void instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash) override;

    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;

    void resetView();
    void currentStateChanged(const ModelNode &node) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion) override;

    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;

    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void exportPopertyAsAlias(const QString &name);
    void removeAliasExport(const QString &name);

    bool locked() const;

    void currentTimelineChanged(const ModelNode &node) override;

protected:
    void timerEvent(QTimerEvent *event) override;
    void setupPane(const TypeName &typeName);
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);

private: //functions
    void reloadQml();
    void updateSize();
    void setupPanes();

    void select();
    void setSelelectedModelNode();

    void delayedResetView();
    void setupQmlBackend();

    void commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value);
    void commitAuxValueToModel(const PropertyName &propertyName, const QVariant &value);
    void removePropertyFromModel(const PropertyName &propertyName);

    bool noValidSelection() const;

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
