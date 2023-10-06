// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"

#include <QHash>
#include <QObject>
#include <QTimer>

#include <propertyeditorcomponentgenerator.h>

QT_BEGIN_NAMESPACE
class QEvent;
class QShortcut;
class QStackedWidget;
class QTimer;
QT_END_NAMESPACE

namespace QmlDesigner {

class CollapseButton;
class ModelNode;
class PropertyEditorQmlBackend;
class PropertyEditorView;
class PropertyEditorWidget;

class PropertyEditorView : public AbstractView
{
    Q_OBJECT

public:
    PropertyEditorView(class AsynchronousImageCache &imageCache,
                       ExternalDependenciesInterface &externalDependencies);
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
    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView key,
                              const QVariant &data) override;

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

    void dragStarted(QMimeData *mimeData) override;
    void dragEnded() override;

    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void exportPropertyAsAlias(const QString &name);
    void removeAliasExport(const QString &name);

    bool locked() const;

    void currentTimelineChanged(const ModelNode &node) override;

    void refreshMetaInfos(const TypeIds &deletedTypeIds) override;

protected:
    void timerEvent(QTimerEvent *event) override;
    void setupPane(const TypeName &typeName);
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);
    bool eventFilter(QObject *obj, QEvent *event) override;

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
    AsynchronousImageCache &m_imageCache;
    ModelNode m_selectedNode;
    QShortcut *m_updateShortcut;
    int m_timerId;
    PropertyEditorWidget* m_stackedWidget;
    QString m_qmlDir;
    QHash<QString, PropertyEditorQmlBackend *> m_qmlBackendHash;
    PropertyEditorQmlBackend *m_qmlBackEndForCurrentType;
    PropertyComponentGenerator m_propertyComponentGenerator;
    PropertyEditorComponentGenerator m_propertyEditorComponentGenerator{m_propertyComponentGenerator};
    bool m_locked;
    bool m_setupCompleted;
    QTimer *m_singleShotTimer;
};

} //QmlDesigner
