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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "modelnodepositionrecalculator.h"

#include <QDebug>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

void ModelNodePositionRecalculator::connectTo(TextModifier *textModifier)
{
    Q_ASSERT(textModifier);

    connect(textModifier, SIGNAL(moved(TextModifier::MoveInfo)), this, SLOT(moved(TextModifier::MoveInfo)));
    connect(textModifier, SIGNAL(replaced(int,int,int)), this, SLOT(replaced(int,int,int)));
}

void ModelNodePositionRecalculator::moved(const TextModifier::MoveInfo &moveInfo)
{
    const int from = moveInfo.objectStart;
    const int to = moveInfo.destination;
    const int length = moveInfo.objectEnd - moveInfo.objectStart;
    const int prefixLength = moveInfo.prefixToInsert.length();
    const int suffixLength = moveInfo.suffixToInsert.length();

    foreach (const ModelNode &node, m_nodesToTrack) {
        const int nodeLocation = m_positionStore->nodeOffset(node);

        if (nodeLocation == ModelNodePositionStorage::INVALID_LOCATION)
            continue;

        int newLocation = nodeLocation;
        if (from <= nodeLocation && moveInfo.objectEnd > nodeLocation) {
            if (to > from)
                if (length == (to - from))
                    newLocation = nodeLocation + prefixLength - moveInfo.leadingCharsToRemove;
                else
                    newLocation = to - length + nodeLocation - from + prefixLength - moveInfo.leadingCharsToRemove - moveInfo.trailingCharsToRemove;
            else
                newLocation = to          + nodeLocation - from + prefixLength;
        } else if (from < nodeLocation && to > nodeLocation) {
            newLocation = nodeLocation - length - moveInfo.leadingCharsToRemove - moveInfo.trailingCharsToRemove;
        } else if (from > nodeLocation && to <= nodeLocation) {
            newLocation = nodeLocation + length + prefixLength + suffixLength;
        } else if (from < nodeLocation && to <= nodeLocation) {
            newLocation = nodeLocation + prefixLength + suffixLength - moveInfo.leadingCharsToRemove - moveInfo.trailingCharsToRemove;
        }
        m_positionStore->setNodeOffset(node, newLocation);
    }

    const int indentLength = length + prefixLength + suffixLength;
    int indentOffset;
    if (to - prefixLength <= from - moveInfo.leadingCharsToRemove)
        indentOffset = to - prefixLength;
    else
        indentOffset = to - length - prefixLength - moveInfo.leadingCharsToRemove - moveInfo.trailingCharsToRemove;

    m_dirtyAreas.insert(indentOffset, indentLength);
}

void ModelNodePositionRecalculator::replaced(int offset, int oldLength, int newLength)
{
    Q_ASSERT(offset >= 0);

    const int growth = newLength - oldLength;
    if (growth == 0)
        return;

    foreach (const ModelNode &node, m_nodesToTrack) {
        const int nodeLocation = m_positionStore->nodeOffset(node);

        if (nodeLocation == ModelNodePositionStorage::INVALID_LOCATION)
            continue;

        if (offset < nodeLocation || (offset == nodeLocation && oldLength == 0)) {
            const int newPosition = nodeLocation + growth;
            if (newPosition < 0)
                m_positionStore->removeNodeOffset(node);
            else
                m_positionStore->setNodeOffset(node, newPosition);
        }
    }
    m_dirtyAreas.insert(offset - newLength + oldLength, newLength);
}
