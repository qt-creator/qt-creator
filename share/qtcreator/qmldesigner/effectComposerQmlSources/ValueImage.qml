// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import EffectComposerBackend

Row {
    id: itemPane

    spacing: 5

    HelperWidgets.UrlChooser {
        backendValue: uniformBackendValue

        actionIndicatorVisible: false
        comboBox.width: Math.min(parent.width - 70, 300)

        onAbsoluteFilePathChanged: uniformValue = absoluteFilePath

        function defaultAsString(defaultPath) {
            if (!defaultPath)
                return undefined
            let urlStr = defaultPath.toString()
            urlStr = urlStr.replace(/^(file:\/{3})/, "")

            // Prepend slash if there is no drive letter
            if (urlStr.length > 1 && urlStr[1] !== ':')
                urlStr = '/' + urlStr;

            return urlStr
        }

        Component.onCompleted: {
            let originalPath = defaultAsString(
                        EffectComposerBackend.rootView.uniformDefaultImage(nodeName, uniformName))
            let originalName = originalPath ? originalPath.split('/').pop() : undefined
            if (originalName) {
                defaultItems = [originalName]
                defaultPaths = [originalPath]
            } else {
                let currentPath = uniformDefaultValue ? defaultAsString(uniformDefaultValue) : undefined
                let currentName = currentPath ? currentPath.split('/').pop() : undefined
                if (currentName) {
                    defaultItems = [currentName]
                    defaultPaths = [currentPath]
                } else {
                    defaultItems = []
                    defaultPaths = []
                }
            }
        }
    }
}
