/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "callgrinddatamodel.h"

#include "callgrindparsedata.h"
#include "callgrindfunction.h"
#include "callgrindcostitem.h"

#include <utils/qtcassert.h>

#include <QChar>
#include <QDebug>
#include <QStringList>
#include <QVector>

namespace Valgrind {
namespace Callgrind {

//BEGIN Helper

namespace {
    // minimum amount of columns, i.e.:
    // function name
    // file name
    // object name
    // num called
    // Additional to this, 2 * ParseData::events().size will be shown (inclusive + self cost)
    const int MinColumnSize = 4;
}

//BEGIN DataModel::Private

class DataModel::Private
{
public:
    Private()
    : m_data(0)
    , m_event(0)
    , m_verboseToolTips(true)
    , m_cycleDetection(false)
    {
    }

    void updateFunctions();

    const ParseData *m_data;
    int m_event;
    bool m_verboseToolTips;
    bool m_cycleDetection;
    QVector<const Function *> m_functions;
};

struct SortFunctions {
    SortFunctions(int event)
    : m_event(event)
    {
    }
    bool operator()(const Function *left, const Function *right)
    {
        return left->inclusiveCost(m_event) > right->inclusiveCost(m_event);
    }
    int m_event;
};

void DataModel::Private::updateFunctions()
{
    if (m_data) {
        m_functions = m_data->functions(m_cycleDetection);
        qSort(m_functions.begin(), m_functions.end(), SortFunctions(m_event));
    } else {
        m_functions.clear();
    }
}

//BEGIN DataModel

DataModel::DataModel(QObject *parent)
   : QAbstractItemModel(parent), d(new Private)
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
 * Evil workaround for https://bugreports.qt-project.org/browse/QTBUG-1135
 * Just replace the bad hyphens by a 'NON-BREAKING HYPHEN' unicode char
 */
static QString noWrap(const QString &str)
{
    QString escapedStr = str;
    return escapedStr.replace(QLatin1Char('-'), "&#8209;");
}

QVariant DataModel::data(const QModelIndex &index, int role) const
{
    //QTC_ASSERT(index.isValid() && index.model() == this, return QVariant());
    //QTC_ASSERT(index.column() >= 0 && index.column() < columnCount(index.parent()), return QVariant());
    //QTC_ASSERT(index.row() >= 0 && index.row() < rowCount(index.parent()), return QVariant());

    const Function *func = d->m_functions.at(index.row());

    if (role == Qt::DisplayRole) {
        if (index.column() == NameColumn)
            return func->name();
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

        QString ret = "<html><head><style>\
            dt { font-weight: bold; }\
            dd { font-family: monospace; }\
            tr.head, td.head { font-weight: bold; }\
            tr.head { text-decoration: underline; }\
            td.group { padding-left: 20px; }\
            td { white-space: nowrap; }\
            </style></head>\n";

        // body, function info first
        ret += "<body><dl>";
        ret += "<dt>" + tr("Function:") + "</dt><dd>" + func->name() + "</dd>\n";
        ret += "<dt>" + tr("File:") + "</dt><dd>" + func->file() + "</dd>\n";
        if (!func->costItems().isEmpty()) {
            const CostItem *firstItem = func->costItems().first();
            for (int i = 0; i < d->m_data->positions().size(); ++i) {
                ret += "<dt>" + ParseData::prettyStringForPosition(d->m_data->positions().at(i)) + "</dt>";
                ret += "<dd>" + QString::number(firstItem->position(i)) + "</dd>\n";
            }
        }
        ret += "<dt>" + tr("Object:") + "</dt><dd>" + func->object() + "</dd>\n";
        ret += "<dt>" + tr("Called:") + "</dt><dd>" + tr("%n time(s)", 0, func->called()) + "</dd>\n";
        ret += "</dl><p/>";

        // self/inclusive costs
        ret += "<table>";
        ret += "<thead><tr class='head'><td>" + tr("Events") + "</td>";
        ret += "<td class='group'>" + tr("Self costs") + "</td><td>" + tr("(%)") + "</td>";
        ret += "<td class='group'>" + tr("Incl. costs") + "</td><td>" + tr("(%)") +  "</td>";
        ret += "</tr></thead>";
        ret += "<tbody>";
        for (int i = 0; i < d->m_data->events().size(); ++i) {
            quint64 selfCost = func->selfCost(i);
            quint64 inclCost = func->inclusiveCost(i);
            quint64 totalCost = d->m_data->totalCost(i);
            // 0.00% format
            const float relSelfCost = (float)qRound((float)selfCost / totalCost * 10000) / 100;
            const float relInclCost = (float)qRound((float)inclCost / totalCost * 10000) / 100;

            ret += "<tr>";
            ret += "<td class='head'><nobr>" + noWrap(ParseData::prettyStringForEvent(d->m_data->events().at(i))) + "</nobr></td>";
            ret += "<td class='group'>" + tr("%1").arg(selfCost) + "</td>";
            ret += "<td>" + tr("(%1%)").arg(relSelfCost) + "</td>";
            ret += "<td class='group'>" + tr("%1").arg(inclCost) + "</td>";
            ret += "<td>" + tr("(%1%)").arg(relInclCost) + "</td>";
            ret += "</tr>";
        }
        ret += "</tbody></table>";
        ret += "</body></html>";
        return ret;
    }

    if (role == FunctionRole) {
        return QVariant::fromValue(func);
    }

    if (role == ParentCostRole) {
        const quint64 totalCost = d->m_data->totalCost(d->m_event);
        return totalCost;
    }

    // the data model does not know about parent<->child relationship
    if (role == RelativeParentCostRole || role == RelativeTotalCostRole) {
        const quint64 totalCost = d->m_data->totalCost(d->m_event);
        if (index.column() == SelfCostColumn)
            return double(func->selfCost(d->m_event)) / totalCost;
        if (index.column() == InclusiveCostColumn) {
            return double(func->inclusiveCost(d->m_event)) / totalCost;
        }
    }

    if (role == LineNumberRole) {
        return func->lineNumber();
    }

    if (role == FileNameRole) {
        return func->file();
    }

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

} // namespace Valgrind
} // namespace Callgrind
