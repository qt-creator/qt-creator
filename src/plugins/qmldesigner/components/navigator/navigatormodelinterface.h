
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

QT_BEGIN_NAMESPACE
class QItemSelection;
class QModelIndex;
QT_END_NAMESPACE

namespace QmlDesigner {

class NavigatorModelInterface
{
public:
    virtual QModelIndex indexForModelNode(const ModelNode &modelNode) const = 0;
    virtual void notifyDataChanged(const ModelNode &modelNode) = 0;
    virtual void notifyModelNodesRemoved(const QList<ModelNode> &modelNodes) = 0;
    virtual void notifyModelNodesInserted(const QList<ModelNode> &modelNodes) = 0;
    virtual void notifyModelNodesMoved(const QList<ModelNode> &modelNodes) = 0;
    virtual void notifyIconsChanged() = 0;
    virtual void setFilter(bool showObjects) = 0;
    virtual void setNameFilter(const QString &filter) = 0;
    virtual void setOrder(bool reverse) = 0;
    virtual void resetModel() = 0;
};

} //QmlDesigner

