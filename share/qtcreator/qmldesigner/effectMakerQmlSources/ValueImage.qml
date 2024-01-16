// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Row {
    id: itemPane

    width: parent.width
    spacing: 5

    HelperWidgets.UrlChooser {
        backendValue: uniformBackendValue

        actionIndicatorVisible: false

        onAbsoluteFilePathChanged: uniformValue = absoluteFilePath

        function defaultAsString() {
            let urlStr = uniformDefaultValue.toString()
            urlStr = urlStr.replace(/^(file:\/{3})/, "")

            // Prepend slash if there is no drive letter
            if (urlStr.length > 1 && urlStr[1] !== ':')
                urlStr = '/' + urlStr;

            return urlStr
        }

        defaultItems: [uniformDefaultValue.split('/').pop()]
        defaultPaths: [defaultAsString(uniformDefaultValue)]
    }
}
