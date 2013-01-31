/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "callgrindtextmark.h"

#include "callgrindhelper.h"

#include "callgrind/callgrinddatamodel.h"
#include "callgrind/callgrindfunction.h"

#include <QDebug>
#include <QPainter>

#include <utils/qtcassert.h>

using namespace Valgrind::Internal;
using namespace Valgrind::Callgrind;

CallgrindTextMark::CallgrindTextMark(const QPersistentModelIndex &index,
                                     const QString &fileName, int lineNumber)
    : TextEditor::BaseTextMark(fileName, lineNumber), m_modelIndex(index)
{
    setPriority(TextEditor::ITextMark::HighPriority);
    setWidthFactor(4.0);
}

void CallgrindTextMark::paint(QPainter *painter, const QRect &paintRect) const
{
    if (!m_modelIndex.isValid())
        return;

    bool ok;
    qreal costs = m_modelIndex.data(RelativeTotalCostRole).toReal(&ok);
    QTC_ASSERT(ok, return);
    QTC_ASSERT(costs >= 0.0 && costs <= 100.0, return);

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
    QFontMetrics fm = QFontMetrics(font);
    while (fm.boundingRect(text).width() > paintRect.width()) {
        font.setPointSize(font.pointSize() - 1);
        fm = QFontMetrics(font);
    }
    painter->setFont(font);

    painter->drawText(paintRect, text, flags);

    painter->restore();
}

const Function *CallgrindTextMark::function() const
{
    if (!m_modelIndex.isValid())
        return 0;

    return m_modelIndex.data(DataModel::FunctionRole).value<const Function *>();
}

