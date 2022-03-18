/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

QT_BEGIN_NAMESPACE
class QShortcut;
class QStackedWidget;
class QTimer;
QT_END_NAMESPACE

namespace QmlDesigner {

class ModelNode;
class MaterialEditorQmlBackend;

class MaterialEditorView : public AbstractView
{
    Q_OBJECT

public:
    MaterialEditorView(QWidget *parent = nullptr);
    ~MaterialEditorView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags propertyChange) override;
    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    void resetView();
    void currentStateChanged(const ModelNode &node) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion) override;
    void modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void exportPropertyAsAlias(const QString &name);
    void removeAliasExport(const QString &name);

    bool locked() const;

    void currentTimelineChanged(const ModelNode &node) override;

public slots:
    void handleToolBarAction(int action);

protected:
    void timerEvent(QTimerEvent *event) override;
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);

private:
    static QString materialEditorResourcesPath();

    void reloadQml();
    QString generateIdFromName(const QString &name);

    void ensureMaterialLibraryNode();
    void requestPreviewRender();
    void applyMaterialToSelectedModels(const ModelNode &material, bool add = false);

    void delayedResetView();
    void setupQmlBackend();

    void commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value);
    void commitAuxValueToModel(const PropertyName &propertyName, const QVariant &value);
    void removePropertyFromModel(const PropertyName &propertyName);
    void renameMaterial(ModelNode &material, const QString &newName);

    bool noValidSelection() const;

    ModelNode m_selectedMaterial;
    ModelNode m_materialLibrary;
    QShortcut *m_updateShortcut = nullptr;
    int m_timerId = 0;
    QStackedWidget *m_stackedWidget = nullptr;
    QList<ModelNode> m_selectedModels;
    QHash<QString, MaterialEditorQmlBackend *> m_qmlBackendHash;
    MaterialEditorQmlBackend *m_qmlBackEnd = nullptr;
    bool m_locked = false;
    bool m_setupCompleted = false;
    bool m_hasQuick3DImport = false;
};

} // namespace QmlDesigner
