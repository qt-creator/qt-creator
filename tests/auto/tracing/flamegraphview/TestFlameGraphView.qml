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

import TestFlameGraphModel 1.0
import "../tracing/"

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
