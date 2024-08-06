// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <selectioncontext.h>

#include <QAction>
#include <QList>

namespace QmlDesigner {

class ModelNode;

class Edit3DMaterialsAction : public QAction
{
    Q_OBJECT
public:
    Edit3DMaterialsAction(const QIcon &icon, QObject *parent);

    void updateMenu(const QList<ModelNode> &selecedNodes);

private slots:
    void removeMaterial(const QString &materialId, int nthMaterial);

private:
    QAction *createMaterialAction(const ModelNode &material, QMenu *parentMenu, int nthMaterial);

    QList<ModelNode> m_selectedNodes;
};

} // namespace QmlDesigner
