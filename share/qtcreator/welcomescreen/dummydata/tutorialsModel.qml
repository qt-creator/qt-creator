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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

ListModel {
    ListElement {
        name: "Main Windows"
        description: "All the standard features of application main windows are provided by Qt. Main windows can have pull down menus, tool bars, and dock windows. These separate forms of user input are unified in an integrated action system that also supports keyboard shortcuts and accelerator keys in menu items."
        imageSource: "images/mockup/mainwindow-examples.png"
        isVideo: false
    }

    ListElement {
        name: "Layouts"
        description: "t uses a layout-based approach to widget management. Widgets are arranged in the optimal positions in windows based on simple layout rules, leading to a consistent look and feel. Custom layouts can be used to provide more control over the positions and sizes of child widgets."
        imageSource: "images/mockup/layout-examples.png"
        isVideo: false
    }

    ListElement {
        name: "Some Video"
        description: "tem views are widgets that typically display data sets. Qt 4's model/view framework lets you handle large data sets by separating the underlying data from the way it is represented to the user, and provides support for customized rendering through the use of delegates."
        imageSource: "images/mockup/"
        isVideo: true
        videoLength: "2:20"
    }

    ListElement {
        name: "Drag and Drop"
        description: "Qt supports native drag and drop on all platforms via an extensible MIME-based system that enables applications to send data to each other in the most appropriate formats. Drag and drop can also be implemented for internal use by applications."
        imageSource: "images/mockup/draganddrop-examples.png"
        isVideo: false
    }
    ListElement {
        name: "Some Video"
        description: "Qt 4 makes it easier than ever to write multithreaded applications. More classes have been made usable from non-GUI threads, and the signals and slots mechanism can now be used to communicate between threads. The QtConcurrent namespace includes a collection of classes and functions for straightforward concurrent programming."
        imageSource: "images/mockup/"
        isVideo: true
        videoLength: "6:40"
    }

    ListElement {
        name: "Some Video"
        description: "Qt provides support for integration with OpenGL implementations on all platforms, giving developers the opportunity to display hardware accelerated 3D graphics alongside a more conventional user interface. Qt provides support for integration with OpenVG implementations on platforms with suitable drivers."
        imageSource: ""
        isVideo: true
        videoLength: "7:40"
    }

    ListElement {
        name: "Some Video"
        description: "Qt is provided with an extensive set of network classes to support both client-based and server side network programming."
        imageSource: "images/mockup/"
        isVideo: true
        videoLength: "2:30"
    }

    ListElement {
        name: "Qt Designer"
        description: "Qt Designer is a capable graphical user interface designer that lets you create and configure forms without writing code. GUIs created with Qt Designer can be compiled into an application or created at run-time."
        imageSource: "images/mockup/designer-examples.png"
        isVideo: false
    }
    ListElement {
        name: "Qt Script"
        description: "Qt is provided with a powerful embedded scripting environment through the QtScript classes."
        imageSource: "images/mockup/qtscript-examples.png"
        isVideo: false
    }

    ListElement {
        name: "Desktop"
        description: "Qt provides features to enable applications to integrate with the user's preferred desktop environment. Features such as system tray icons, access to the desktop widget, and support for desktop services can be used to improve the appearance of applications and take advantage of underlying desktop facilities."
        imageSource: "images/mockup/desktop-examples.png"
        isVideo: false
    }

    ListElement {
        name: "Caption"
        description: "Description"
        imageSource: "image/mockup/penguin.png"
        isVideo: false
    }

    ListElement {
        name: "Caption"
        description: "Description"
        imageSource: "images/mockup/penguin.png"
        isVideo: false
    }
}
