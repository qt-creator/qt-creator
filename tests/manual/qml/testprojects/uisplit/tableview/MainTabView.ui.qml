/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2
import "tabs"

TabView {
    id:frame
    focus:true

    width: 540
    height: 360

    property bool genteratedTabFrameVisible: false
    property bool genteratedTabHeaderVisible: false
    property bool genteratedTabSortIndicatorVisible: false
    property bool genteratedTabAlternatingRowColors: false

    Tab {
        title: "XmlListModel"

        TabXmlListModel {
            anchors.fill: parent
            anchors.margins: 12

            frameVisible: genteratedTabFrameVisible
            headerVisible: genteratedTabHeaderVisible
            sortIndicatorVisible: genteratedTabSortIndicatorVisible
            alternatingRowColors: genteratedTabAlternatingRowColors
        }
    }

    Tab {
        title: "Multivalue"

        TabMultivalue {
            anchors.fill: parent
            anchors.margins: 12

            frameVisible: genteratedTabFrameVisible
            headerVisible: genteratedTabHeaderVisible
            sortIndicatorVisible: genteratedTabSortIndicatorVisible
            alternatingRowColors: genteratedTabAlternatingRowColors
        }
    }

    Tab {
        title: "Generated"
        id: generatedTab

        TabGenerated {
            anchors.margins: 12
            anchors.fill: parent

            frameVisible: genteratedTabFrameVisible
            headerVisible: genteratedTabHeaderVisible
            sortIndicatorVisible: genteratedTabSortIndicatorVisible
            alternatingRowColors: genteratedTabAlternatingRowColors
        }
    }

    Tab {
        title: "Delegates"

        TabDelegates {
        }
    }
}
