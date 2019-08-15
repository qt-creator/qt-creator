/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.1
import QtQuick.Layouts 1.0
import QtQuickDesignerTheme 1.0
import StudioControls 1.0 as StudioControls

StudioControls.RealSpinBox {
    id: spinBox
    width: 82
    Layout.minimumWidth: 82

    property string propertyName
    actionIndicatorVisible: false

    realFrom: -9999
    realTo: 9999
    realStepSize: 1

    Component.onCompleted: spinBox.realValue = gradientLine.model.readGradientProperty(propertyName)
    onCompressedRealValueModified: gradientLine.model.setGradientProperty(propertyName, spinBox.realValue)
}
