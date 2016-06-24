/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQml 2.2
import QtQuick 2.0
import QtQuick.LocalStorage 2.0

Rectangle {
    width: 200
    height: 100

    Text {
        text: "?"
        anchors.horizontalCenter: parent.horizontalCenter

        function findGreetings() {
            var db = LocalStorage.openDatabaseSync("QQmlExampleDB", "1.0", "The Example QML SQL!", 1000000);

            db.transaction(
                function(tx) {
                    // Create the database if it doesn't already exist
                    tx.executeSql('CREATE TABLE IF NOT EXISTS Greeting(salutation TEXT, salutee TEXT)');

                    // Add (another) greeting row
                    tx.executeSql('INSERT INTO Greeting VALUES(?, ?)', [ 'hello', 'world' ]);

                    // Show all added greetings
                    var rs = tx.executeSql('SELECT * FROM Greeting');

                    var r = ""
                    for (var i = 0; i < rs.rows.length; i++) {
                        r += rs.rows.item(i).salutation + ", " + rs.rows.item(i).salutee + "\n"
                    }
                    text = r
                }
            )
        }

        Component.onCompleted: findGreetings()
    }
}
