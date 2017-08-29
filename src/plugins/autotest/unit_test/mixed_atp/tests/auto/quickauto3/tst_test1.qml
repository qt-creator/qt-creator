/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of Qt Creator.
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

Bar {
    name: "InheritanceTest"

    function test_func() {
        compare(5 + 5, 10, "verifying 5 + 5 = 10");
        compare(10 - 5, 5, "verifying 10 - 5 = 5");
    }
}
