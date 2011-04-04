/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "callgrindcostdelegate.h"

#include "callgrindcostview.h"
#include "callgrindhelper.h"

#include <valgrind/callgrind/callgrindabstractmodel.h>
#include <valgrind/callgrind/callgrindparsedata.h>

#include <utils/qtcassert.h>

#include <QPainter>
#include <QDebug>
#include <QApplication>

using namespace Valgrind::Callgrind;

namespace Callgrind {
namespace Internal {

//BEGIN CostDelegate::Private
class CostDelegate::Private {
public:
    Private();
    ~Private();

    Valgrind::Callgrind::AbstractModel *m_model;
    CostDelegate::CostFormat m_format;

    static int toNativeRole(CostFormat format);
    float relativeCost(const QModelIndex &index) const;
    QString displayText(const QModelIndex &index, const QLocale &locale) const;
};

CostDelegate::Private::Private()
: m_model(0)
, m_format(CostDelegate::FormatAbsolute)
{

}

CostDelegate::Private::~Private()
{

}

int CostDelegate::Private::toNativeRole(CostDelegate::CostFormat format)
{
    switch(format)
    {
    case FormatAbsolute:
    case FormatRelative:
        return Valgrind::Callgrind::AbstractModel::RelativeTotalCostRole;
    case FormatRelativeToParent:
        return Valgrind::Callgrind::AbstractModel::RelativeParentCostRole;
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
//END CostDelegate::Private

//BEGIN CostDelegate
CostDelegate::CostDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , d(new Private)
{

}

CostDelegate::~CostDelegate()
{
    delete d;
}

void CostDelegate::setModel(Valgrind::Callgrind::AbstractModel *model)
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
    // init
    QStyleOptionViewItemV4 opt(option);
    initStyleOption(&opt, index);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    // draw controls, but no text
    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

    painter->save();

    // draw bar
    float ratio = qBound(0.0f, d->relativeCost(index), 1.0f);
    QRect barRect = opt.rect;
    barRect.setWidth(opt.rect.width() * ratio);
    painter->setPen(Qt::NoPen);
    painter->setBrush(CallgrindHelper::colorForCostRatio(ratio));
    painter->drawRect(barRect);

    // draw text
    const QString text = d->displayText(index, opt.locale);
    const QBrush &textBrush = (option.state & QStyle::State_Selected ? opt.palette.highlightedText() : opt.palette.text());
    painter->setBrush(Qt::NoBrush);
    painter->setPen(textBrush.color());
    painter->drawText(opt.rect, Qt::AlignRight, text);

    painter->restore();
}

QSize CostDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt(option);
    initStyleOption(&opt, index);

    const QString text = d->displayText(index, opt.locale);
    const QSize size = QSize(option.fontMetrics.width(text),
                             option.fontMetrics.height());
    return size;
}

} // Internal
} // Callgrind
