/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color text: "#ffe7e7e7"
    readonly property color foregroundPrimary: "#ffa3a3a3"
    readonly property color foregroundSecondary: "#ff808080"
    readonly property color backgroundPrimary: "#ff333333"
    readonly property color backgroundSecondary: "#ff232323"
    readonly property color hover: "#ff404040"
    readonly property color accent: "#ff57d658"
    readonly property color link: "#ff67e668"
    readonly property color disabledLink: "#7fffffff"
}
