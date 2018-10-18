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

import PerfProfilerFlameGraphModel 1.0
import "../tracing/"

FlameGraphView {
    id: root
    sizeRole: PerfProfilerFlameGraphModel.SamplesRole

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
        PerfProfilerFlameGraphModel.SamplesRole,                      qsTr("Samples"),
        PerfProfilerFlameGraphModel.FunctionRole,                     qsTr("Function"),
        PerfProfilerFlameGraphModel.SourceFileRole,                   qsTr("Source"),
        PerfProfilerFlameGraphModel.ElfFileRole,                      qsTr("Binary"),
        PerfProfilerFlameGraphModel.ResourceAllocationsRole,          qsTr("Allocations"),
        PerfProfilerFlameGraphModel.ObservedResourceAllocationsRole,  qsTr(" observed"),
        PerfProfilerFlameGraphModel.LostResourceRequestsRole,         qsTr(" guessed"),
        PerfProfilerFlameGraphModel.ResourceReleasesRole,             qsTr("Releases"),
        PerfProfilerFlameGraphModel.ObservedResourceReleasesRole,     qsTr(" observed"),
        PerfProfilerFlameGraphModel.GuessedResourceReleasesRole,      qsTr(" guessed"),
        PerfProfilerFlameGraphModel.ResourcePeakRole,                 qsTr("Peak Usage")
    ].reduce(toMap, {})

    details: function(flameGraph) {
        var model = [];
        if (!flameGraph.dataValid) {
            model.push(trRoleNames[PerfProfilerFlameGraphModel.FunctionRole]);
            model.push(qsTr("Various"));
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
