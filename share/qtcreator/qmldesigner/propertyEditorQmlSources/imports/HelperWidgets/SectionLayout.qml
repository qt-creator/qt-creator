// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

GridLayout {
    columns: 2
    columnSpacing: StudioTheme.Values.sectionColumnSpacing
    rowSpacing: StudioTheme.Values.sectionRowSpacing
    width: parent.width - StudioTheme.Values.sectionLayoutRightPadding
}
