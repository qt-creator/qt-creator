/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
import QtQuick 2.0

Rectangle {
    Foo {
        name: "NestedInherited"

        function test_strConcat() {
            var tmp = String();
            tmp = tmp.concat("Hello", " ", "World");
            var dummy = String("Hello World");
            compare(dummy, tmp, "Comparing concat");
            verify(dummy === tmp, "Comparing concat equality")
        }
    }
}
