// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtCreator.QmlProfiler
import QtCreator.Tracing

FlameGraphView {
    id: root

    model: flameGraphModel

    typeIdRole: QmlProfilerFlameGraphModel.TypeIdRole
    sourceFileRole: QmlProfilerFlameGraphModel.FilenameRole
    sourceLineRole: QmlProfilerFlameGraphModel.LineRole
    sourceColumnRole: QmlProfilerFlameGraphModel.ColumnRole
    detailsTitleRole: QmlProfilerFlameGraphModel.TypeRole
    summaryRole: QmlProfilerFlameGraphModel.DetailsRole
    noteRole: QmlProfilerFlameGraphModel.NoteRole

    modes: [
        QmlProfilerFlameGraphModel.DurationRole,
        QmlProfilerFlameGraphModel.MemoryRole,
        QmlProfilerFlameGraphModel.AllocationsRole
    ]

    trRoleNames: [
        QmlProfilerFlameGraphModel.DurationRole,      qsTr("Total Time"),
        QmlProfilerFlameGraphModel.CallCountRole,     qsTr("Calls"),
        QmlProfilerFlameGraphModel.DetailsRole,       qsTr("Details"),
        QmlProfilerFlameGraphModel.TimePerCallRole,   qsTr("Mean Time"),
        QmlProfilerFlameGraphModel.TimeInPercentRole, qsTr("In Percent"),
        QmlProfilerFlameGraphModel.LocationRole,      qsTr("Location"),
        QmlProfilerFlameGraphModel.AllocationsRole,   qsTr("Allocations"),
        QmlProfilerFlameGraphModel.MemoryRole,        qsTr("Memory")
    ].reduce(toMap, {})

    details: function(flameGraph) {
        var model = [];
        if (!flameGraph.dataValid) {
            model.push(trRoleNames[QmlProfilerFlameGraphModel.DetailsRole]);
            model.push(qsTr("Various Events"));
        } else {
            function addDetail(role, format) { root.addDetail(role, format, model, flameGraph); }

            addDetail(QmlProfilerFlameGraphModel.DetailsRole, detailFormats.noop);
            addDetail(QmlProfilerFlameGraphModel.CallCountRole, detailFormats.noop);
            addDetail(QmlProfilerFlameGraphModel.DurationRole, detailFormats.printTime);
            addDetail(QmlProfilerFlameGraphModel.TimePerCallRole, detailFormats.printTime);
            addDetail(QmlProfilerFlameGraphModel.LocationRole, detailFormats.noop);
            addDetail(QmlProfilerFlameGraphModel.MemoryRole, detailFormats.printMemory);
            addDetail(QmlProfilerFlameGraphModel.AllocationsRole, detailFormats.noop);
        }
        return model;
    }

    summary: function(attached) {
        if (!attached.dataValid)
            return qsTr("others");

        return attached.data(QmlProfilerFlameGraphModel.DetailsRole) + " ("
                + attached.data(QmlProfilerFlameGraphModel.TypeRole) + ", "
                + root.percent(root.sizeRole, attached) + "%)";
    }

    isHighlighted: function(node) {
        function recurse(parentNode, typeId) {
            if (!parentNode)
                return false;
            if (parentNode.typeId === typeId) {
                parentNode.isHighlighted = true;
                return true;
            }
            return recurse(parentNode.parent, typeId);
        }

        return recurse(node.parent, node.typeId);
    }
}
