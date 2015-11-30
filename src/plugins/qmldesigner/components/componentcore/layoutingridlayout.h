/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef LAYOUTINGRIDLAYOUT_H
#define LAYOUTINGRIDLAYOUT_H

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
    typedef std::function<bool(const ModelNode &node1, const ModelNode &node2)> LessThan;

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

#endif // LAYOUTINGRIDLAYOUT_H
