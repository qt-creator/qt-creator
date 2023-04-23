// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofilertracemanager.h"
#include "perfresourcecounter.h"

#include <QAbstractItemModel>
#include <QScopedPointer>
#include <QtQml/qqml.h>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerFlameGraphData;
class PerfProfilerFlameGraphModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("use the context property")
    Q_DISABLE_COPY_MOVE(PerfProfilerFlameGraphModel);

public:

    enum Role {
        TypeIdRole = Qt::UserRole + 1, // Sort by data, not by displayed string
        DisplayNameRole,
        SamplesRole,
        FunctionRole,
        SourceFileRole,
        LineRole,
        ElfFileRole,
        ObservedResourceAllocationsRole,
        LostResourceRequestsRole,
        ResourceAllocationsRole,
        ObservedResourceReleasesRole,
        GuessedResourceReleasesRole,
        ResourceReleasesRole,
        ResourcePeakRole,
        MaxRole
    };
    Q_ENUM(Role)

    struct Data {
        Data *parent = nullptr;
        int typeId = -1;
        uint samples = 0;
        uint lastResourceChangeId = 0;

        uint observedResourceAllocations = 0;
        uint lostResourceRequests = 0;

        uint observedResourceReleases = 0;
        uint guessedResourceReleases = 0;

        qint64 resourceUsage = 0;
        qint64 resourcePeak = 0;

        std::vector<std::unique_ptr<Data>> children;
    };

    PerfProfilerFlameGraphModel(PerfProfilerTraceManager *manager);
    ~PerfProfilerFlameGraphModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void initialize();
    void finalize(PerfProfilerFlameGraphData *data);
    void clear(PerfProfilerFlameGraphData *data);

signals:
    void gotoSourceLocation(QString file, int line, int column);

private:
    QScopedPointer<Data> m_stackBottom;
    QScopedPointer<PerfProfilerFlameGraphData> m_offlineData;
};

} // namespace Internal
} // namespace PerfProfiler
