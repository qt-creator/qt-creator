// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtCreator.PerfProfiler
import QtCreator.Tracing

FlameGraphView {
    id: root

    model: flameGraphModel

    typeIdRole: PerfProfilerFlameGraphModel.TypeIdRole
    sourceFileRole: PerfProfilerFlameGraphModel.SourceFileRole
    sourceLineRole: PerfProfilerFlameGraphModel.LineRole
    sourceColumnRole: -1
    detailsTitleRole: PerfProfilerFlameGraphModel.DisplayNameRole
    summaryRole: PerfProfilerFlameGraphModel.FunctionRole

    modes: [
        PerfProfilerFlameGraphModel.SamplesRole,
        PerfProfilerFlameGraphModel.ResourcePeakRole,
        PerfProfilerFlameGraphModel.ResourceAllocationsRole,
        PerfProfilerFlameGraphModel.ResourceReleasesRole
    ]

    trRoleNames: [
        PerfProfilerFlameGraphModel.SamplesRole,                      qsTranslate("PerfProfiler", "Samples"),
        PerfProfilerFlameGraphModel.FunctionRole,                     qsTranslate("PerfProfiler", "Function"),
        PerfProfilerFlameGraphModel.SourceFileRole,                   qsTranslate("PerfProfiler", "Source"),
        PerfProfilerFlameGraphModel.ElfFileRole,                      qsTranslate("PerfProfiler", "Binary"),
        PerfProfilerFlameGraphModel.ResourceAllocationsRole,          qsTranslate("PerfProfiler", "Allocations"),
        PerfProfilerFlameGraphModel.ObservedResourceAllocationsRole,  qsTranslate("PerfProfiler", " observed"),
        PerfProfilerFlameGraphModel.LostResourceRequestsRole,         qsTranslate("PerfProfiler", " guessed"),
        PerfProfilerFlameGraphModel.ResourceReleasesRole,             qsTranslate("PerfProfiler", "Releases"),
        PerfProfilerFlameGraphModel.ObservedResourceReleasesRole,     qsTranslate("PerfProfiler", " observed"),
        PerfProfilerFlameGraphModel.GuessedResourceReleasesRole,      qsTranslate("PerfProfiler", " guessed"),
        PerfProfilerFlameGraphModel.ResourcePeakRole,                 qsTranslate("PerfProfiler", "Peak Usage")
    ].reduce(toMap, {})

    details: function(flameGraph) {
        var model = [];
        if (!flameGraph.dataValid) {
            model.push(trRoleNames[PerfProfilerFlameGraphModel.FunctionRole]);
            model.push(qsTranslate("PerfProfiler", "Various"));
        } else {
            function addDetail(role, format) { root.addDetail(role, format, model, flameGraph); }

            addDetail(PerfProfilerFlameGraphModel.FunctionRole, detailFormats.noop);
            addDetail(PerfProfilerFlameGraphModel.SamplesRole, detailFormats.noop);
            addDetail(PerfProfilerFlameGraphModel.SourceFileRole, detailFormats.addLine);
            addDetail(PerfProfilerFlameGraphModel.ElfFileRole, detailFormats.noop);

            if (flameGraph.data(PerfProfilerFlameGraphModel.ResourcePeakRole) !== 0)
                addDetail(PerfProfilerFlameGraphModel.ResourcePeakRole, detailFormats.printMemory);

            function printResources(role, observedRole, guessedRole, format) {
                if (flameGraph.data(role) !== 0) {
                    addDetail(role, format);
                    if (flameGraph.data(guessedRole) !== 0) {
                        addDetail(observedRole, format);
                        addDetail(guessedRole, format);
                    }
                }
            }

            printResources(PerfProfilerFlameGraphModel.ResourceAllocationsRole,
                           PerfProfilerFlameGraphModel.ObservedResourceAllocationsRole,
                           PerfProfilerFlameGraphModel.LostResourceRequestsRole,
                           detailFormats.noop);

            printResources(PerfProfilerFlameGraphModel.ResourceReleasesRole,
                           PerfProfilerFlameGraphModel.ObservedResourceReleasesRole,
                           PerfProfilerFlameGraphModel.GuessedResourceReleasesRole,
                           detailFormats.noop);
        }
        return model;
    }
}
