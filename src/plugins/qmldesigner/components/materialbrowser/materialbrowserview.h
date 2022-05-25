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

#include <QPointer>

namespace QmlDesigner {

class MaterialBrowserWidget;

class MaterialBrowserView : public AbstractView
{
    Q_OBJECT

public:
    MaterialBrowserView(QObject *parent = nullptr);
    ~MaterialBrowserView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap) override;
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

private:
    void refreshModel();
    bool isMaterial(const ModelNode &node) const;

    QPointer<MaterialBrowserWidget> m_widget;
    bool m_hasQuick3DImport = false;
    bool m_autoSelectModelMaterial = false; // TODO: wire this to some action

private slots:
    void handleSelectedMaterialChanged(int idx);
    void handleApplyToSelectedTriggered(const QmlDesigner::ModelNode &material, bool add = false);
    void handleRenameMaterial(const QmlDesigner::ModelNode &material, const QString &newName);
    void handleAddNewMaterial();
};

} // namespace QmlDesigner
