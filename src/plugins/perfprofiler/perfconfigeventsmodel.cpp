/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfconfigeventsmodel.h"

#include <utils/algorithm.h>

#include <QMetaEnum>

namespace PerfProfiler {
namespace Internal {

PerfConfigEventsModel::PerfConfigEventsModel(PerfSettings *settings, QObject *parent) :
    QAbstractTableModel(parent), m_settings(settings)
{
    connect(m_settings, &PerfSettings::changed, this, &PerfConfigEventsModel::reset);
}

int PerfConfigEventsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_settings->events().length();
}

int PerfConfigEventsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnInvalid;
}

QVariant PerfConfigEventsModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        break; // Retrieve the actual value
    default:
        return QVariant(); // ignore
    }

    QString event = m_settings->events().value(index.row());
    const EventDescription description = parseEvent(event);
    switch (index.column()) {
    case ColumnEventType: {
        if (role == Qt::DisplayRole) {
            if (description.eventType == EventTypeInvalid)
                return QVariant();
            auto meta = QMetaEnum::fromType<EventType>();
            return QString::fromLatin1(meta.valueToKey(description.eventType))
                    .mid(static_cast<int>(strlen("EventType"))).toLower();
        }
        return description.eventType;
    }
    case ColumnSubType: {
        switch (description.eventType) {
        case EventTypeHardware:
        case EventTypeSoftware:
        case EventTypeCache:
            if (role == Qt::DisplayRole)
                return subTypeString(description.eventType, description.subType);
            return description.subType;
        case EventTypeRaw:
            if (role == Qt::DisplayRole)
                return QString("r%1").arg(description.numericEvent, 3, 16, QLatin1Char('0'));
            else
                return description.numericEvent;
        case EventTypeBreakpoint:
            if (role == Qt::DisplayRole)
                return QString("0x%1").arg(description.numericEvent, 16, 16, QLatin1Char('0'));
            else
                return description.numericEvent;
        case EventTypeCustom:
            return description.customEvent;
        default:
            return QVariant();
        }
    }
    case ColumnOperation:
        if (role == Qt::DisplayRole) {
            if (description.eventType == EventTypeBreakpoint) {
                QString result;
                if (description.operation & OperationLoad)
                    result += 'r';
                if (description.operation & OperationStore)
                    result += 'w';
                if (description.operation & OperationExecute)
                    result += 'x';
                return result;
            }

            if (description.eventType == EventTypeCache) {
                if (description.operation == OperationInvalid)
                    return QVariant();
                auto meta = QMetaEnum::fromType<Operation>();
                return QString::fromLatin1(meta.valueToKey(description.operation)).mid(
                            static_cast<int>(strlen("Operation"))).toLower();
            }

            return QVariant();
        }

        return description.operation;
    case ColumnResult:
        if (role == Qt::DisplayRole) {
            if (description.result == ResultInvalid)
                return QVariant();
            auto meta = QMetaEnum::fromType<Result>();
            return QString::fromLatin1(meta.valueToKey(description.result)).mid(
                        static_cast<int>(strlen("Result"))).toLower();
        }
        return description.result;
    default:
        return QVariant();
    }
}

bool PerfConfigEventsModel::setData(const QModelIndex &dataIndex, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    const int row = dataIndex.row();
    const int column = dataIndex.column();

    QStringList events = m_settings->events();
    EventDescription description = parseEvent(events[row]);
    switch (column) {
    case ColumnEventType:
        description.eventType = qvariant_cast<EventType>(value);
        break;
    case ColumnSubType:
        switch (description.eventType) {
        case EventTypeHardware:
        case EventTypeSoftware:
        case EventTypeCache:
            description.subType = qvariant_cast<SubType>(value);
            break;
        case EventTypeRaw:
            description.numericEvent = value.toULongLong();
            break;
        case EventTypeBreakpoint:
            description.numericEvent = value.toULongLong();
            break;
        case EventTypeCustom:
            description.customEvent = value.toString();
            break;
        default:
            break;
        }
        break;
    case ColumnOperation:
        description.operation = value.toInt();
        break;
    case ColumnResult:
        description.result = qvariant_cast<Result>(value);
        break;
    }
    events[row] = generateEvent(description);
    m_settings->setEvents(events);
    emit dataChanged(index(row, ColumnEventType), index(row, ColumnResult));
    return true;
}

QVariant PerfConfigEventsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case ColumnEventType: return tr("Event Type");
    case ColumnSubType:   return tr("Counter");
    case ColumnOperation: return tr("Operation");
    case ColumnResult:    return tr("Result");
    default:              return QVariant();
    }

}

bool PerfConfigEventsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    QStringList events = m_settings->events();
    for (int i = 0; i < count; ++i)
        events.insert(row, "dummy");
    beginInsertRows(parent, row, row + count - 1);
    m_settings->setEvents(events);
    endInsertRows();
    return true;
}

bool PerfConfigEventsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    QStringList events = m_settings->events();
    for (int i = 0; i < count; ++i)
        events.removeAt(row);
    beginRemoveRows(parent, row, row + count - 1);
    m_settings->setEvents(events);
    endRemoveRows();

    if (events.isEmpty()) {
        beginInsertRows(parent, 0, 0);
        events.append("dummy");
        m_settings->setEvents(events);
        endInsertRows();
    }

    return true;
}

Qt::ItemFlags PerfConfigEventsModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

QString PerfConfigEventsModel::subTypeString(EventType eventType, SubType subType)
{
    switch (eventType) {
    case EventTypeHardware:
        switch (subType) {
        case SubTypeCpuCycles:             return QLatin1String("cpu-cycles");
        case SubTypeInstructions:          return QLatin1String("instructions");
        case SubTypeCacheReferences:       return QLatin1String("cache-references");
        case SubTypeCacheMisses:           return QLatin1String("cache-misses");
        case SubTypeBranchInstructions:    return QLatin1String("branch-instructions");
        case SubTypeBranchMisses:          return QLatin1String("branch-misses");
        case SubTypeBusCycles:             return QLatin1String("bus-cycles");
        case SubTypeStalledCyclesFrontend: return QLatin1String("stalled-cycles-frontend");
        case SubTypeStalledCyclesBackend:  return QLatin1String("stalled-cycles-backend");
        case SubTypeRefCycles:             return QLatin1String("ref-cycles");
        default:                           return QLatin1String("cpu-cycles");
        }
    case EventTypeSoftware:
        switch (subType) {
        case SubTypeCpuClock:              return QLatin1String("cpu-clock");
        case SubTypeTaskClock:             return QLatin1String("task-clock");
        case SubTypePageFaults:            return QLatin1String("page-faults");
        case SubTypeContextSwitches:       return QLatin1String("context-switches");
        case SubTypeCpuMigrations:         return QLatin1String("cpu-migrations");
        case SubTypeMinorFaults:           return QLatin1String("minor-faults");
        case SubTypeMajorFaults:           return QLatin1String("major-faults");
        case SubTypeAlignmentFaults:       return QLatin1String("alignment-faults");
        case SubTypeEmulationFaults:       return QLatin1String("emulation-faults");
        case SubTypeDummy:                 return QLatin1String("dummy");
        default:                           return QLatin1String("cpu-clock");
        }
    case EventTypeCache:
        switch (subType) {
        case SubTypeL1Dcache:              return QLatin1String("L1-dcache");
        case SubTypeL1Icache:              return QLatin1String("L1-icache");
        case SubTypeLLC:                   return QLatin1String("LLC");
        case SubTypeDTLB:                  return QLatin1String("dTLB");
        case SubTypeITLB:                  return QLatin1String("iTLB");
        case SubTypeBranch:                return QLatin1String("branch");
        case SubTypeNode:                  return QLatin1String("node");
        default:                           return QLatin1String("L1-dcache");
        }
    default: return QString();
    }
}

QString PerfConfigEventsModel::generateEvent(const EventDescription &description) const
{
    switch (description.eventType) {
    case EventTypeHardware:
    case EventTypeSoftware:
        return subTypeString(description.eventType, description.subType);
    case EventTypeCache: {
        QString result = subTypeString(description.eventType, description.subType);
        switch (description.operation) {
        case OperationLoad:     result += "-load";     break;
        case OperationStore:    result += "-store";    break;
        case OperationPrefetch: result += "-prefetch"; break;
        default:                result += "-load";     break;
        }
        switch (description.result) {
        case ResultRefs:   return result + "-refs";
        case ResultMisses: return result + "-misses";
        default:           return result + "-misses";
        };
    }
    case EventTypeRaw:
        return QString::fromLatin1("r%1").arg(description.numericEvent, 3, 16, QLatin1Char('0'));
    case EventTypeBreakpoint: {
        QString rwx;
        if (description.operation & OperationLoad)
            rwx += 'r';
        if (description.operation & OperationStore)
            rwx += 'w';
        if (description.operation & OperationExecute)
            rwx += 'x';
        return QString::fromLatin1("mem:%1:%2")
                .arg(description.numericEvent, 16, 16, QLatin1Char('0'))
                .arg(rwx.isEmpty() ? "r" : rwx);
    }
    case EventTypeCustom:
        return description.customEvent;
    default:
        return QLatin1String("cpu-cycles");
    }
}

void PerfConfigEventsModel::reset()
{
    beginResetModel();
    endResetModel();
}

PerfConfigEventsModel::EventDescription PerfConfigEventsModel::parseEvent(
        const QString &event) const
{
    EventDescription description;

    if (event.startsWith("mem:")) {
        description.eventType = EventTypeBreakpoint;
        QStringList parts = event.split(':');
        if (parts.length() > 1) {
            description.numericEvent = parts[1].toULongLong(nullptr, 16);
            if (parts.length() > 2) {
                QString rwx = parts[2];
                if (rwx.contains('r'))
                    description.operation = description.operation | OperationLoad;
                if (rwx.contains('w'))
                    description.operation = description.operation | OperationStore;
                if (rwx.contains('x'))
                    description.operation = description.operation | OperationExecute;
            }
        }
        return description;
    }

    if (event.startsWith('r') && event.length() == 4) {
        bool ok = false;
        const uint eventNumber = event.midRef(1).toUInt(&ok, 16);
        if (ok) {
            description.eventType = EventTypeRaw;
            description.numericEvent = eventNumber;
            return description;
        }
    }

    QStringList parts = Utils::transform(event.split("-"), [](const QString &part) {
        if (part.isEmpty())
            return part;
        QString ret = part;
        ret[0] = part[0].toUpper();
        return ret;
    });

    QMetaEnum subTypeMeta = QMetaEnum::fromType<SubType>();
    QStringList extras;

    int subtype = -1;
    while (!parts.isEmpty()) {
        QString key = QString("SubType") + parts.join(QString());
        subtype = subTypeMeta.keyToValue(key.toLatin1());
        if (subtype == -1)
            extras.prepend(parts.takeLast());
        else
            break;
    }

    if (subtype != -1) {
        description.subType = SubType(subtype);
        if (subtype < int(SubTypeEventTypeSoftware))
            description.eventType = EventTypeHardware;
        else if (subtype < int(SubTypeEventTypeCache))
            description.eventType = EventTypeSoftware;
        else
            description.eventType = EventTypeCache;

        if (!extras.isEmpty()) {
            QMetaEnum operationMeta = QMetaEnum::fromType<Operation>();
            int operation = operationMeta.keyToValue(QByteArray("Operation")
                                                     + extras.takeFirst().toLatin1());
            if (operation != -1)
                description.operation = operation;
        }

        if (!extras.isEmpty()) {
            QMetaEnum resultMeta = QMetaEnum::fromType<Result>();
            int result = resultMeta.keyToValue(QByteArray("Result") + extras.takeFirst().toLatin1());
            if (result != -1)
                description.result = Result(result);
        }

        if (extras.isEmpty())
            return description;
    }

    description.eventType = EventTypeCustom;
    description.subType = SubTypeInvalid;
    description.operation = OperationInvalid;
    description.result = ResultInvalid;
    description.customEvent = event;
    return description;

}

} // namespace Internal
} // namespace PerfProfiler
