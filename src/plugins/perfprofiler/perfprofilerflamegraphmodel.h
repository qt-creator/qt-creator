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

#pragma once

#include "perfprofilertracemanager.h"
#include "perfresourcecounter.h"

#include <QAbstractItemModel>
#include <QScopedPointer>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerFlameGraphData;
class PerfProfilerFlameGraphModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_DISABLE_COPY(PerfProfilerFlameGraphModel);
    Q_ENUMS(Role)
public:
    PerfProfilerFlameGraphModel(PerfProfilerFlameGraphModel &&) = delete;
    PerfProfilerFlameGraphModel &operator=(PerfProfilerFlameGraphModel &&) = delete;

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
