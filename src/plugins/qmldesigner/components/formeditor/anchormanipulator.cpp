/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "anchormanipulator.h"

#include "formeditoritem.h"
#include "formeditorscene.h"
#include "formeditorview.h"
#include <model.h>
#include <rewritertransaction.h>

namespace QmlDesigner {

AnchorManipulator::AnchorManipulator(FormEditorView *view)
  : m_beginFormEditorItem(0),
    m_beginAnchorLine(AnchorLine::Invalid),
    m_view(view)
{
}

AnchorManipulator::~AnchorManipulator()
{
    clear();
}

void AnchorManipulator::begin(FormEditorItem *beginItem, AnchorLine::Type anchorLine)
{
   m_beginFormEditorItem = beginItem;
   m_beginAnchorLine = anchorLine;
}

static double offset(const QPointF &topLeft, const QPointF &bottomRight, AnchorLine::Type anchorLine)
{
     switch(anchorLine) {
        case AnchorLine::Top    : return topLeft.y();
        case AnchorLine::Left   : return topLeft.x();
        case AnchorLine::Bottom : return bottomRight.y();
        case AnchorLine::Right  : return bottomRight.x();
        default: break;
    }

     return 0.0;
}

void AnchorManipulator::setMargin(FormEditorItem *endItem, AnchorLine::Type endAnchorLine)
{
    QPointF beginItemTopLeft(m_beginFormEditorItem->mapToParent(m_beginFormEditorItem->qmlItemNode().instanceBoundingRect().topLeft()));
    QPointF endItemTopLeft(m_beginFormEditorItem->parentItem()->mapFromItem(endItem, endItem->qmlItemNode().instanceBoundingRect().topLeft()));

    QPointF beginItemBottomRight(m_beginFormEditorItem->mapToParent(m_beginFormEditorItem->qmlItemNode().instanceBoundingRect().bottomRight()));
    QPointF endItemBottomRight(m_beginFormEditorItem->parentItem()->mapFromItem(endItem, endItem->qmlItemNode().instanceBoundingRect().bottomRight()));

    QPointF topLeftAnchorOffset = beginItemTopLeft - endItemTopLeft;
    QPointF bottomRightAnchorOffset = endItemBottomRight - beginItemBottomRight;


    double anchorOffset = 0.0;
    if (m_beginAnchorLine & (AnchorLine::Bottom | AnchorLine::Right)) {
        anchorOffset = offset(endItemTopLeft, endItemBottomRight, endAnchorLine) -
                       offset(beginItemTopLeft, beginItemBottomRight, m_beginAnchorLine);
    } else {
        anchorOffset = offset(beginItemTopLeft, beginItemBottomRight, m_beginAnchorLine) -
                       offset(endItemTopLeft, endItemBottomRight, endAnchorLine);
    }

    m_beginFormEditorItem->qmlItemNode().anchors().setMargin(m_beginAnchorLine, anchorOffset);
}
void AnchorManipulator::addAnchor(FormEditorItem *endItem, AnchorLine::Type endAnchorLine)
{
    RewriterTransaction m_rewriterTransaction = m_view->beginRewriterTransaction();
    setMargin(endItem, endAnchorLine);

    m_beginFormEditorItem->qmlItemNode().anchors().setAnchor(m_beginAnchorLine,
                                                              endItem->qmlItemNode(),
                                                              endAnchorLine);
}

void AnchorManipulator::removeAnchor()
{
    RewriterTransaction transaction = m_view->beginRewriterTransaction();
    QmlAnchors anchors(m_beginFormEditorItem->qmlItemNode().anchors());
    if (anchors.instanceHasAnchor(m_beginAnchorLine)) {
        anchors.removeAnchor(m_beginAnchorLine);
        anchors.removeMargin(m_beginAnchorLine);
    }
}

void AnchorManipulator::clear()
{
    m_beginFormEditorItem = 0;
    m_beginAnchorLine = AnchorLine::Invalid;
}

bool AnchorManipulator::isActive() const
{
    return m_beginFormEditorItem && m_beginAnchorLine != AnchorLine::Invalid;
}

AnchorLine::Type AnchorManipulator::beginAnchorLine() const
{
    return m_beginAnchorLine;
}

bool AnchorManipulator::beginAnchorLineIsHorizontal() const
{
    return beginAnchorLine() & AnchorLine::HorizontalMask;
}
bool AnchorManipulator::beginAnchorLineIsVertical() const
{
    return beginAnchorLine() & AnchorLine::HorizontalMask;
}

FormEditorItem *AnchorManipulator::beginFormEditorItem() const
{
    return m_beginFormEditorItem;
}

} // namespace QmlDesigner
