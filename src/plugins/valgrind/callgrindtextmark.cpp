// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindtextmark.h"

#include "callgrind/callgrinddatamodel.h"
#include "callgrind/callgrindfunction.h"
#include "callgrindhelper.h"
#include "valgrindtr.h"

#include <utils/qtcassert.h>

#include <QLabel>
#include <QLayout>

using namespace Utils;
using namespace Valgrind::Internal;
using namespace Valgrind::Callgrind;

namespace Constants { const char CALLGRIND_TEXT_MARK_CATEGORY[] = "Callgrind.Textmark"; }

CallgrindTextMark::CallgrindTextMark(const QPersistentModelIndex &index,
                                     const FilePath &fileName,
                                     int lineNumber)
    : TextEditor::TextMark(fileName,
                           lineNumber,
                           {Tr::tr("Callgrind"), Constants::CALLGRIND_TEXT_MARK_CATEGORY})
    , m_modelIndex(index)
{
    setPriority(TextEditor::TextMark::HighPriority);
    const Function *f = function();
    const QString inclusiveCost = QLocale::system().toString(f->inclusiveCost(0));
    setLineAnnotation(Tr::tr("%1 (Called: %2; Incl. Cost: %3)")
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
