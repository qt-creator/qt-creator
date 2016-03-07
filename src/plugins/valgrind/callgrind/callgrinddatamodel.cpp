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

#include "callgrinddatamodel.h"

#include "callgrindparsedata.h"
#include "callgrindfunction.h"
#include "callgrindcostitem.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QChar>
#include <QDebug>
#include <QStringList>
#include <QTextDocument>
#include <QVector>

namespace Valgrind {
namespace Callgrind {

class DataModel::Private
{
public:
    void updateFunctions();

    const ParseData *m_data = 0;
    int m_event = 0;
    bool m_verboseToolTips = true;
    bool m_cycleDetection = false;
    bool m_shortenTemplates = false;
    QVector<const Function *> m_functions;
};

void DataModel::Private::updateFunctions()
{
    if (m_data) {
        m_functions = m_data->functions(m_cycleDetection);
        Utils::sort(m_functions, [this](const Function *l, const Function *r) {
            return l->inclusiveCost(m_event) > r->inclusiveCost(m_event);
        });
    } else {
        m_functions.clear();
    }
}

DataModel::DataModel()
   : d(new Private)
{
}

DataModel::~DataModel()
{
    delete d;
}

void DataModel::setParseData(const ParseData *data)
{
    if (d->m_data == data)
        return;

    beginResetModel();
    d->m_data = data;
    d->m_event = 0;
    d->updateFunctions();
    endResetModel();
}

void DataModel::setVerboseToolTipsEnabled(bool enabled)
{
    d->m_verboseToolTips = enabled;
}

bool DataModel::verboseToolTipsEnabled() const
{
    return d->m_verboseToolTips;
}

const ParseData *DataModel::parseData() const
{
    return d->m_data;
}

void DataModel::setCostEvent(int event)
{
    if (!d->m_data)
        return;

    QTC_ASSERT(event >= 0 && d->m_data->events().size() > event, return);
    beginResetModel();
    d->m_event = event;
    d->updateFunctions();
    endResetModel();
    emit dataChanged(index(0, SelfCostColumn), index(qMax(0, rowCount() - 1), InclusiveCostColumn));
}

int DataModel::costEvent() const
{
    return d->m_event;
}

int DataModel::rowCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);

    if (!d->m_data || parent.isValid())
        return 0;

    return d->m_functions.size();
}

int DataModel::columnCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);
    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QModelIndex DataModel::index(int row, int column, const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return QModelIndex());
    if (row == 0 && rowCount(parent) == 0) // happens with empty models
        return QModelIndex();
    QTC_ASSERT(row >= 0 && row < rowCount(parent), return QModelIndex());
    return createIndex(row, column);
}

QModelIndex DataModel::parent(const QModelIndex &child) const
{
    QTC_ASSERT(!child.isValid() || child.model() == this, return QModelIndex());
    return QModelIndex();
}

QModelIndex DataModel::indexForObject(const Function *function) const
{
    if (!function)
        return QModelIndex();

    const int row = d->m_functions.indexOf(function);
    if (row < 0)
        return QModelIndex();

    return createIndex(row, 0);
}

/**
 * Evil workaround for https://bugreports.qt.io/browse/QTBUG-1135
 * Just replace the bad hyphens by a 'NON-BREAKING HYPHEN' unicode char
 */
static QString noWrap(const QString &str)
{
    QString escapedStr = str;
    return escapedStr.replace(QLatin1Char('-'), QLatin1String("&#8209;"));
}

static QString shortenTemplate(QString str)
{
    int depth = 0;
    int  j = 0;
    for (int i = 0, n = str.size(); i != n; ++i) {
        int c = str.at(i).unicode();
        if (c == '>')
            --depth;
        if (depth == 0)
            str[j++] = str.at(i);
        if (c == '<')
            ++depth;
    }
    str.truncate(j);
    return str;
}

QVariant DataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Function *func = d->m_functions.at(index.row());

    if (role == Qt::DisplayRole) {
        if (index.column() == NameColumn)
            return d->m_shortenTemplates ? shortenTemplate(func->name()) : func->name();
        if (index.column() == LocationColumn)
            return func->location();
        if (index.column() == CalledColumn)
            return func->called();
        if (index.column() == SelfCostColumn)
            return func->selfCost(d->m_event);
        if (index.column() == InclusiveCostColumn)
            return func->inclusiveCost(d->m_event);
        return QVariant();
    }

    if (role == Qt::ToolTipRole) {
        if (!d->m_verboseToolTips)
            return data(index, Qt::DisplayRole);

        QString ret = QLatin1String("<html><head><style>\
            dt { font-weight: bold; }\
            dd { font-family: monospace; }\
            tr.head, td.head { font-weight: bold; }\
            tr.head { text-decoration: underline; }\
            td.group { padding-left: 20px; }\
            td { white-space: nowrap; }\
            </style></head>\n<body><dl>");

        QString entry = QLatin1String("<dt>%1</dt><dd>%2</dd>\n");
        // body, function info first
        ret += entry.arg(tr("Function:")).arg(func->name().toHtmlEscaped());
        ret += entry.arg(tr("File:")).arg(func->file());
        if (!func->costItems().isEmpty()) {
            const CostItem *firstItem = func->costItems().first();
            for (int i = 0; i < d->m_data->positions().size(); ++i) {
                ret += entry.arg(ParseData::prettyStringForPosition(d->m_data->positions().at(i)))
                       .arg(firstItem->position(i));
            }
        }
        ret += entry.arg(tr("Object:")).arg(func->object());
        ret += entry.arg(tr("Called:")).arg(tr("%n time(s)", 0, func->called()));
        ret += QLatin1String("</dl><p/>");

        // self/inclusive costs
        entry = QLatin1String("<td class='group'>%1</td><td>%2</td>");
        ret += QLatin1String("<table>");
        ret += QLatin1String("<thead><tr class='head'>");
        ret += QLatin1String("<td>") + tr("Events") + QLatin1String("</td>");
        ret += entry.arg(tr("Self costs")).arg(tr("(%)"));
        ret += entry.arg(tr("Incl. costs")).arg(tr("(%)"));
        ret += QLatin1String("</tr></thead>");
        ret += QLatin1String("<tbody>");
        for (int i = 0; i < d->m_data->events().size(); ++i) {
            quint64 selfCost = func->selfCost(i);
            quint64 inclCost = func->inclusiveCost(i);
            quint64 totalCost = d->m_data->totalCost(i);
            // 0.00% format
            const float relSelfCost = (float)qRound((float)selfCost / totalCost * 10000) / 100;
            const float relInclCost = (float)qRound((float)inclCost / totalCost * 10000) / 100;

            ret += QLatin1String("<tr>");
            ret += QLatin1String("<td class='head'><nobr>") +
                    noWrap(ParseData::prettyStringForEvent(d->m_data->events().at(i)))
                    + QLatin1String("</nobr></td>");
            ret += entry.arg(selfCost).arg(tr("(%1%)").arg(relSelfCost));
            ret += entry.arg(inclCost).arg(tr("(%1%)").arg(relInclCost));
            ret += QLatin1String("</tr>");
        }
        ret += QLatin1String("</tbody></table>");
        ret += QLatin1String("</body></html>");
        return ret;
    }

    if (role == FunctionRole)
        return QVariant::fromValue(func);

    if (role == ParentCostRole) {
        const quint64 totalCost = d->m_data->totalCost(d->m_event);
        return totalCost;
    }

    // the data model does not know about parent<->child relationship
    if (role == RelativeParentCostRole || role == RelativeTotalCostRole) {
        const quint64 totalCost = d->m_data->totalCost(d->m_event);
        if (index.column() == SelfCostColumn)
            return totalCost ? (double(func->selfCost(d->m_event)) / totalCost) : 0.0;
        if (index.column() == InclusiveCostColumn)
            return totalCost ? (double(func->inclusiveCost(d->m_event)) / totalCost) : 0.0;
    }

    if (role == LineNumberRole)
        return func->lineNumber();

    if (role == FileNameRole)
        return func->file();

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == CalledColumn)
            return Qt::AlignRight;
    }

    return QVariant();
}

QVariant DataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
        return QVariant();

    QTC_ASSERT(section >= 0 && section < columnCount(), return QVariant());

    if (role == Qt::ToolTipRole) {
        if (!d->m_data)
            return QVariant();

        const QString prettyCostStr = ParseData::prettyStringForEvent(d->m_data->events().at(d->m_event));
        if (section == SelfCostColumn)
            return tr("%1 cost spent in a given function excluding costs from called functions.").arg(prettyCostStr);
        if (section == InclusiveCostColumn)
            return tr("%1 cost spent in a given function including costs from called functions.").arg(prettyCostStr);
        return QVariant();
    }

    if (section == NameColumn)
        return tr("Function");
    if (section == LocationColumn)
        return tr("Location");
    if (section == CalledColumn)
        return tr("Called");
    if (section == SelfCostColumn)
        return tr("Self Cost: %1").arg(d->m_data ? d->m_data->events().value(d->m_event) : QString());
    if (section == InclusiveCostColumn)
        return tr("Incl. Cost: %1").arg(d->m_data ? d->m_data->events().value(d->m_event) : QString());

    return QVariant();
}

void DataModel::enableCycleDetection(bool enabled)
{
    beginResetModel();
    d->m_cycleDetection = enabled;
    d->updateFunctions();
    endResetModel();
}

void DataModel::setShortenTemplates(bool enabled)
{
    beginResetModel();
    d->m_shortenTemplates = enabled;
    d->updateFunctions();
    endResetModel();
}

} // namespace Valgrind
} // namespace Callgrind
