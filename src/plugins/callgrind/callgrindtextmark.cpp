/**************************************************************************
**
** This file is part of Qt Creator Analyzer Tools
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

#include "callgrindtextmark.h"

#include "callgrindhelper.h"

#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindfunction.h>

#include <QDebug>
#include <QPainter>

#include <utils/qtcassert.h>

using namespace Callgrind::Internal;
using namespace Valgrind::Callgrind;

CallgrindTextMark::CallgrindTextMark(const QPersistentModelIndex &index,
                                     const QString &fileName, int lineNumber)
    : m_modelIndex(index)
{
    setLocation(fileName, lineNumber);
    setPriority(TextEditor::ITextMark::HighPriority);
}

CallgrindTextMark::~CallgrindTextMark()
{
}

void CallgrindTextMark::paint(QPainter *painter, const QRect &paintRect) const
{
    if (!m_modelIndex.isValid())
        return;

    bool ok;
    qreal costs = m_modelIndex.data(RelativeTotalCostRole).toReal(&ok);
    QTC_ASSERT(ok, return)
    QTC_ASSERT(costs >= 0.0 && costs <= 100.0, return)

    painter->save();

    // set up
    painter->setPen(Qt::black);

    // draw bar
    QRect fillRect = paintRect;
    fillRect.setWidth(paintRect.width() * costs);
    painter->fillRect(paintRect, Qt::white);
    painter->fillRect(fillRect, CallgrindHelper::colorForCostRatio(costs));
    painter->drawRect(paintRect);

    // draw text
    const QTextOption flags = Qt::AlignHCenter | Qt::AlignVCenter;
    const QString text = CallgrindHelper::toPercent(costs * 100.0f);

    // decrease font size if paint rect is too small (very unlikely, but may happen)
    QFont font = painter->font();
    QFontMetrics fm = font;
    while (fm.boundingRect(text).width() > paintRect.width()) {
        font.setPointSize(font.pointSize() - 1);
        fm = font;
    }
    painter->setFont(font);

    painter->drawText(paintRect, text, flags);

    painter->restore();
}

const Function* CallgrindTextMark::function() const
{
    if (!m_modelIndex.isValid())
        return 0;

    return m_modelIndex.data(DataModel::FunctionRole).value<const Function *>();
}

double CallgrindTextMark::widthFactor() const
{
    return 4.0;
}
