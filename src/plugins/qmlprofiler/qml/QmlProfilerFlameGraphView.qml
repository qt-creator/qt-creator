// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        QmlProfilerFlameGraphModel.DurationRole,      qsTranslate("QtC::QmlProfiler", "Total Time"),
        QmlProfilerFlameGraphModel.CallCountRole,     qsTranslate("QtC::QmlProfiler", "Calls"),
        QmlProfilerFlameGraphModel.DetailsRole,       qsTranslate("QtC::QmlProfiler", "Details"),
        QmlProfilerFlameGraphModel.TimePerCallRole,   qsTranslate("QtC::QmlProfiler", "Mean Time"),
        QmlProfilerFlameGraphModel.LocationRole,      qsTranslate("QtC::QmlProfiler", "Location"),
        QmlProfilerFlameGraphModel.AllocationsRole,   qsTranslate("QtC::QmlProfiler", "Allocations"),
        QmlProfilerFlameGraphModel.MemoryRole,        qsTranslate("QtC::QmlProfiler", "Memory")
    ].reduce(toMap, {})

    details: function(flameGraph) {
        var model = [];
        if (!flameGraph.dataValid) {
            model.push(trRoleNames[QmlProfilerFlameGraphModel.DetailsRole]);
            model.push(qsTranslate("QtC::QmlProfiler", "Various Events"));
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
            return qsTranslate("QtC::QmlProfiler", "others");

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
