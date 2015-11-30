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

#ifndef QmlAnchors_H
#define QmlAnchors_H

#include <qmldesignercorelib_global.h>
#include <qmlitemnode.h>
#include "anchorline.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT QmlAnchors
{
public:
    QmlAnchors(const QmlItemNode &fxItemNode);

    bool isValid() const;

    void setAnchor(AnchorLineType sourceAnchorLineType,
                   const QmlItemNode &targetModelNode,
                   AnchorLineType targetAnchorLineType);
    bool canAnchor(const QmlItemNode &targetModelNode) const;
    AnchorLineType possibleAnchorLines(AnchorLineType sourceAnchorLineType,
                                         const QmlItemNode &targetModelNode) const;
    AnchorLine instanceAnchor(AnchorLineType sourceAnchorLineType) const;

    void removeAnchor(AnchorLineType sourceAnchorLineType);
    void removeAnchors();
    bool instanceHasAnchor(AnchorLineType sourceAnchorLineType) const;
    bool instanceHasAnchors() const;
    double instanceLeftAnchorLine() const;
    double instanceTopAnchorLine() const;
    double instanceRightAnchorLine() const;
    double instanceBottomAnchorLine() const;
    double instanceHorizontalCenterAnchorLine() const;
    double instanceVerticalCenterAnchorLine() const;
    double instanceAnchorLine(AnchorLineType anchorLine) const;

    void setMargin(AnchorLineType sourceAnchorLineType, double margin) const;
    bool instanceHasMargin(AnchorLineType sourceAnchorLineType) const;
    double instanceMargin(AnchorLineType sourceAnchorLineType) const;
    void removeMargin(AnchorLineType sourceAnchorLineType);
    void removeMargins();

    void fill();
    void centerIn();

    bool checkForCycle(AnchorLineType anchorLineTyp, const QmlItemNode &sourceItem) const;
    bool checkForHorizontalCycle(const QmlItemNode &sourceItem) const;
    bool checkForVerticalCycle(const QmlItemNode &sourceItem) const;

    QmlItemNode qmlItemNode() const;

    bool modelHasAnchors() const;
    bool modelHasAnchor(AnchorLineType sourceAnchorLineType) const;
    AnchorLine modelAnchor(AnchorLineType sourceAnchorLineType) const;


private:
    QmlItemNode m_qmlItemNode;
};

} //QmlDesigner

#endif // QmlAnchors_H
