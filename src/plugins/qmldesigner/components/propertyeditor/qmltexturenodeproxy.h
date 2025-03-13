// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlobjectnode.h>

#include <QObject>

namespace QmlDesigner {

class QmlTextureNodeProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QmlObjectNode textureNode READ textureNode NOTIFY textureNodeChanged)
    Q_PROPERTY(bool hasTexture READ hasTexture NOTIFY textureNodeChanged)
    Q_PROPERTY(
        bool selectedNodeAcceptsMaterial
        READ selectedNodeAcceptsMaterial
        NOTIFY selectedNodeAcceptsMaterialChanged)

public:
    enum ToolBarAction {
        ApplyToSelected,
        AddNewTexture,
        DeleteCurrentTexture,
        OpenMaterialBrowser
    };
    Q_ENUM(ToolBarAction)

    explicit QmlTextureNodeProxy();
    ~QmlTextureNodeProxy() override;

    void setup(const QmlObjectNode &objectNode);

    void updateSelectionDetails();
    void handlePropertyChanged(const AbstractProperty &property);
    void handleBindingPropertyChanged(const BindingProperty &property);
    void handlePropertiesRemoved(const AbstractProperty &property);

    QmlObjectNode textureNode() const;
    bool hasTexture() const;
    bool selectedNodeAcceptsMaterial() const;

    Q_INVOKABLE QString resolveResourcePath(const QString &path) const;
    Q_INVOKABLE void toolbarAction(int action);

    static void registerDeclarativeType();

signals:
    void textureNodeChanged();
    void selectedNodeAcceptsMaterialChanged();

private:
    void setTextureNode(const QmlObjectNode &node);
    void setSelectedNodeAcceptsMaterial(bool value);
    bool hasQuick3DImport() const;

    QmlObjectNode m_textureNode;
    bool m_selectedNodeAcceptsMaterial = false;
};

} // namespace QmlDesigner
