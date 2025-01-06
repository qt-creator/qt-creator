// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import TableModules as TableModules

QtObject{
    id: controller

    required property TableView view

    readonly property TableModules.TableHeaderLengthModel model: TableModules.TableHeaderLengthModel {
        id: internalModel

        orientation: Qt.Horizontal
        sourceModel: controller.view.model

        onSectionVisibilityChanged: controller.view.forceLayout()

        Component.onCompleted: {
            controller.view.columnWidthProvider = controller.lengthProvider
        }
    }

    function lengthProvider(section) {
        if (!internalModel.isVisible(section))
            return 0

        let len = controller.view.explicitColumnWidth(section)
        if (len < 0)
            len = controller.view.implicitColumnWidth(section)
        len = Math.max(len, internalModel.minimumLength(section))

        internalModel.setLength(section, len)
        return len
    }
}
