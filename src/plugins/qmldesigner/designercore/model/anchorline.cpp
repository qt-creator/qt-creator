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

#include "anchorline.h"

namespace QmlDesigner {

AnchorLine::AnchorLine()
    : m_qmlItemNode(QmlItemNode())
    , m_type(AnchorLineInvalid)
{}

AnchorLine::AnchorLine(const QmlItemNode &qmlItemNode, AnchorLineType type)
    : m_qmlItemNode(qmlItemNode),
      m_type(type)
{}

AnchorLineType AnchorLine::type() const
{
    return m_type;
}

bool AnchorLine::isValid() const
{
    return m_type != AnchorLineInvalid && m_qmlItemNode.isValid();
}

bool AnchorLine::isHorizontalAnchorLine(AnchorLineType anchorline)
{
    return anchorline & AnchorLineHorizontalMask;
}

bool AnchorLine::isVerticalAnchorLine(AnchorLineType anchorline)
{
     return anchorline & AnchorLineVerticalMask;
}

QmlItemNode AnchorLine::qmlItemNode() const
{
    return m_qmlItemNode;
}

} // namespace QmlDesigner
