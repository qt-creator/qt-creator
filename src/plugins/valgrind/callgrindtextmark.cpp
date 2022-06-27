/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "callgrindtextmark.h"

#include "callgrindhelper.h"

#include "callgrind/callgrinddatamodel.h"
#include "callgrind/callgrindfunction.h"

#include <QDebug>
#include <QLabel>
#include <QLayout>
#include <QPainter>

#include <utils/qtcassert.h>

using namespace Utils;
using namespace Valgrind::Internal;
using namespace Valgrind::Callgrind;

namespace Constants { const char CALLGRIND_TEXT_MARK_CATEGORY[] = "Callgrind.Textmark"; }

CallgrindTextMark::CallgrindTextMark(const QPersistentModelIndex &index,
                                     const FilePath &fileName, int lineNumber)
    : TextEditor::TextMark(fileName, lineNumber, Constants::CALLGRIND_TEXT_MARK_CATEGORY)
    , m_modelIndex(index)
{
    setPriority(TextEditor::TextMark::HighPriority);
    const Function *f = function();
    const QString inclusiveCost = QLocale::system().toString(f->inclusiveCost(0));
    setLineAnnotation(tr("%1 (Called: %2; Incl. Cost: %3)")
                      .arg(CallgrindHelper::toPercent(costs() * 100.0f))
                      .arg(f->called())
                      .arg(inclusiveCost));
}

const Function *CallgrindTextMark::function() const
{
    if (!m_modelIndex.isValid())
        return nullptr;

    return m_modelIndex.data(DataModel::FunctionRole).value<const Function *>();
}

bool CallgrindTextMark::addToolTipContent(QLayout *target) const
{
    if (!m_modelIndex.isValid())
        return false;

    const QString tooltip = m_modelIndex.data(Qt::ToolTipRole).toString();
    if (tooltip.isEmpty())
        return false;

    target->addWidget(new QLabel(tooltip));
    return true;
}

qreal CallgrindTextMark::costs() const
{
    bool ok;
    qreal costs = m_modelIndex.data(RelativeTotalCostRole).toReal(&ok);
    QTC_ASSERT(ok, return 0.0);
    QTC_ASSERT(costs >= 0.0 && costs <= 100.0, return 0.0);

    return costs;
}
