// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindcostdelegate.h"

#include "callgrind/callgrindabstractmodel.h"
#include "callgrindhelper.h"

#include <utils/qtcassert.h>

#include <QApplication>
#include <QPainter>

using namespace Valgrind::Callgrind;

namespace Valgrind::Internal {

class CostDelegate::Private
{
public:
    QAbstractItemModel *m_model = nullptr;
    CostDelegate::CostFormat m_format = CostDelegate::FormatAbsolute;

    float relativeCost(const QModelIndex &index) const;
    QString displayText(const QModelIndex &index, const QLocale &locale) const;
};

static int toNativeRole(CostDelegate::CostFormat format)
{
    switch (format)
    {
    case CostDelegate::FormatAbsolute:
    case CostDelegate::FormatRelative:
        return Callgrind::RelativeTotalCostRole;
    case CostDelegate::FormatRelativeToParent:
        return Callgrind::RelativeParentCostRole;
    default:
        return -1;
    }
}

float CostDelegate::Private::relativeCost(const QModelIndex &index) const
{
    bool ok = false;
    float cost = index.data(toNativeRole(m_format)).toFloat(&ok);
    QTC_ASSERT(ok, return 0);
    return cost;
}

QString CostDelegate::Private::displayText(const QModelIndex &index, const QLocale &locale) const
{
    switch (m_format) {
    case FormatAbsolute:
        return locale.toString(index.data().toULongLong());
    case FormatRelative:
    case FormatRelativeToParent:
        if (!m_model)
            break;
        float cost = relativeCost(index) * 100.0f;
        return CallgrindHelper::toPercent(cost, locale);
    }

    return QString();
}

CostDelegate::CostDelegate(QObject *parent)
    : QStyledItemDelegate(parent), d(new Private)
{
}

CostDelegate::~CostDelegate()
{
    delete d;
}

void CostDelegate::setModel(QAbstractItemModel *model)
{
    d->m_model = model;
}

void CostDelegate::setFormat(CostFormat format)
{
    d->m_format = format;
}

CostDelegate::CostFormat CostDelegate::format() const
{
    return d->m_format;
}

void CostDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    // Draw controls, but no text.
    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

    painter->save();

    // Draw bar.
    float ratio = qBound(0.0f, d->relativeCost(index), 1.0f);
    QRect barRect = opt.rect;
    barRect.setWidth(opt.rect.width() * ratio);
    painter->setPen(Qt::NoPen);
    painter->setBrush(CallgrindHelper::colorForCostRatio(ratio));
    painter->drawRect(barRect);

    // Draw text.
    QLocale loc = opt.locale;
    loc.setNumberOptions({});
    const QString text = d->displayText(index, loc);
    const QBrush &textBrush = (option.state & QStyle::State_Selected ? opt.palette.highlightedText() : opt.palette.text());
    painter->setBrush(Qt::NoBrush);
    painter->setPen(textBrush.color());
    painter->drawText(opt.rect, Qt::AlignRight, text);

    painter->restore();
}

QSize CostDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QString text = d->displayText(index, opt.locale);
    const QSize size = QSize(option.fontMetrics.horizontalAdvance(text),
                             option.fontMetrics.height());
    return size;
}

} // namespace Valgrind::Internal
