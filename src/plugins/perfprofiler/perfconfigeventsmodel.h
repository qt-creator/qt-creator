// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfsettings.h"

#include <QAbstractTableModel>

namespace PerfProfiler {
namespace Internal {

class PerfConfigEventsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    PerfConfigEventsModel(PerfSettings *settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    enum Column {
        ColumnEventType,
        ColumnSubType,
        ColumnOperation,
        ColumnResult,
        ColumnInvalid
    };
    Q_ENUM(Column)

    enum EventType {
        EventTypeHardware,
        EventTypeSoftware,
        EventTypeCache,
        EventTypeRaw,
        EventTypeBreakpoint,
        EventTypeCustom,
        EventTypeInvalid
    };
    Q_ENUM(EventType)

    enum SubType {
        SubTypeCpuCycles,
        SubTypeInstructions,
        SubTypeCacheReferences,
        SubTypeCacheMisses,
        SubTypeBranchInstructions,
        SubTypeBranchMisses,
        SubTypeBusCycles,
        SubTypeStalledCyclesFrontend,
        SubTypeStalledCyclesBackend,
        SubTypeRefCycles,
        SubTypeCpuClock,
        SubTypeTaskClock,
        SubTypePageFaults,
        SubTypeContextSwitches,
        SubTypeCpuMigrations,
        SubTypeMinorFaults,
        SubTypeMajorFaults,
        SubTypeAlignmentFaults,
        SubTypeEmulationFaults,
        SubTypeDummy,
        SubTypeL1Dcache,
        SubTypeL1Icache,
        SubTypeLLC,
        SubTypeDTLB,
        SubTypeITLB,
        SubTypeBranch,
        SubTypeNode,
        SubTypeInvalid
    };
    Q_ENUM(SubType)

    enum SubTypeEventType {
        SubTypeEventTypeHardware = SubTypeCpuCycles,
        SubTypeEventTypeSoftware = SubTypeCpuClock,
        SubTypeEventTypeCache    = SubTypeL1Dcache
    };

    enum Operation {
        OperationInvalid  = 0x0,
        OperationLoad     = 0x1,
        OperationStore    = 0x2,
        OperationPrefetch = 0x4,
        OperationExecute  = 0x8,
    };
    Q_ENUM(Operation)

    enum Result {
        ResultRefs,
        ResultMisses,
        ResultInvalid
    };
    Q_ENUM(Result)

    struct EventDescription {
        EventType eventType  = EventTypeInvalid;
        SubType subType      = SubTypeInvalid;
        int operation        = OperationInvalid;
        Result result        = ResultInvalid;
        quint64 numericEvent = 0;
        QString customEvent;
    };

    static QString subTypeString(EventType eventType, SubType subType);

private:
    PerfSettings *m_settings;
    EventDescription parseEvent(const QString &event) const;
    QString generateEvent(const EventDescription &description) const;
    void reset();
};

} // namespace Internal
} // namespace PerfProfiler
