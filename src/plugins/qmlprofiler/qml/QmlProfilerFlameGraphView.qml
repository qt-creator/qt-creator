/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QmlProfilerFlameGraphModel 1.0
import "../tracing/"

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
