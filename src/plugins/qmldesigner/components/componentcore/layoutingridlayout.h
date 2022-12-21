// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "selectioncontext.h"

#include <modelnode.h>
#include <qmlitemnode.h>

#include <QList>
#include <QVector>

#include <functional>

namespace QmlDesigner {

class LayoutInGridLayout
{
public:
    static void ensureLayoutImport(const SelectionContext &context);
    static void layout(const SelectionContext &context);
    static void reparentToNodeAndRemovePositionForModelNodes(const ModelNode &parentModelNode,
                                                                                 const QList<ModelNode> &modelNodeList);
    static void setSizeAsPreferredSize(const QList<ModelNode> &modelNodeList);
private:
    using LessThan = std::function<bool (const ModelNode&, const ModelNode&)>;

    LayoutInGridLayout(const SelectionContext &selectionContext);
    void doIt();
    int columnCount() const;
    int rowCount() const;
    void collectItemNodes();
    void collectOffsets();
    void sortOffsets();
    void calculateGridOffsets();
    void removeEmtpyRowsAndColumns();
    void initializeCells();
    void markUsedCells();
    void fillEmptyCells();
    void setSpanning(const ModelNode &layoutNode);
    void removeSpacersBySpanning(QList<ModelNode> &nodes);
    LessThan lessThan();

    const SelectionContext m_selectionContext;
    QList<QmlItemNode> m_qmlItemNodes;
    QmlItemNode m_parentNode;
    QList<ModelNode> m_layoutedNodes;
    QList<ModelNode> m_spacerNodes;
    QVector<int> m_xTopOffsets;
    QVector<int> m_xBottomOffsets;
    QVector<int> m_yTopOffsets;
    QVector<int> m_yBottomOffsets;
    QVector<bool> m_cells;
    QVector<bool> m_rows;
    QVector<bool> m_columns;
    int m_startX;
    int m_startY;
};

} //QmlDesigner
