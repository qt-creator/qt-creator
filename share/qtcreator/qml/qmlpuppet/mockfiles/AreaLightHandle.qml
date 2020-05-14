/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick 2.0
import QtQuick3D 1.15

DirectionalDraggable {
    id: handleRoot

    property string currentLabel
    property point currentMousePos
    property string propName
    property real propValue: 0
    property real newValue: 0
    property real baseScale: 5

    scale: autoScaler.getScale(Qt.vector3d(baseScale, baseScale, baseScale))
    length: 3
    offset: -1.5

    Model {
        id: handle
        source: "#Sphere"
        materials: [ handleRoot.material ]
        scale: Qt.vector3d(0.02, 0.02, 0.02)
    }

    AutoScaleHelper {
        id: autoScaler
        active: handleRoot.active
        view3D: handleRoot.view3D
    }

    property real _startValue
    property real _startScale

    signal valueCommit()
    signal valueChange()

    function updateValue(relativeDistance, screenPos)
    {
        handleRoot.newValue = Math.round(Math.min(999999, Math.max(0, _startValue + (relativeDistance * _startScale))));
        var l = Qt.locale();
        handleRoot.currentLabel = propName + qsTr(": ") + Number(newValue).toLocaleString(l, 'f', 0);
        handleRoot.currentMousePos = screenPos;
    }

    onPressed: {
        _startScale = autoScaler.relativeScale * baseScale;
        _startValue = propValue;
        updateValue(0, screenPos);
    }

    onDragged: {
        updateValue(relativeDistance, screenPos);
        handleRoot.valueChange();
    }

    onReleased: {
        updateValue(relativeDistance, screenPos);
        handleRoot.valueCommit();
    }
}
