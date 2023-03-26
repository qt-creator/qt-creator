// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtCreator.TstTracingFlameGraphView
import QtCreator.Tracing

FlameGraphView {
    id: root

    model: flameGraphModel

    typeIdRole: TestFlameGraphModel.TypeIdRole
    sourceFileRole: TestFlameGraphModel.SourceFileRole
    sourceLineRole: TestFlameGraphModel.SourceLineRole
    sourceColumnRole: TestFlameGraphModel.SourceColumnRole
    detailsTitleRole: TestFlameGraphModel.DetailsTitleRole
    summaryRole: TestFlameGraphModel.SummaryRole

    modes: [
        TestFlameGraphModel.SizeRole,
        TestFlameGraphModel.SourceLineRole,
        TestFlameGraphModel.SourceColumnRole,
    ]

    trRoleNames: [
        TestFlameGraphModel.SizeRole,         qsTr("Size"),
        TestFlameGraphModel.SourceFileRole,   qsTr("Source File"),
        TestFlameGraphModel.SourceLineRole,   qsTr("Source Line"),
        TestFlameGraphModel.SourceColumnRole, qsTr("Source Column"),
    ].reduce(toMap, {})

    details: function(flameGraph) {
        var model = [];
        root.addDetail(TestFlameGraphModel.SizeRole, detailFormats.noop,
                       model, flameGraph);
        root.addDetail(TestFlameGraphModel.SourceFileRole, detailFormats.addLine,
                       model, flameGraph);
        return model;
    }
}
