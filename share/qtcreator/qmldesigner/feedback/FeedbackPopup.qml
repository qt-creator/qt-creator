// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

Rectangle {
    id: root_rectangle

    property int rating: 0

    signal submitFeedback(string feedback, int rating)
    signal closeClicked()

    width: 740
    height: 382
    border { color: "#0094ce"; width: 1 }

    Text {
        id: h1
        objectName: "title"
        color: "#333333"
        text: "Enjoying Qt Design Studio?"
        font { family: "Titillium"; pixelSize: 21 }
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 50 }
    }

    Text {
        id: h2
        color: "#333333"
        text: "Select the level of your satisfaction."
        font { family: "Titillium"; pixelSize: 21 }
        anchors { horizontalCenter: parent.horizontalCenter; top: h1.bottom; topMargin: 12 }
    }

    Row {
        id: starRow
        width: 246; height: 42; spacing: 6.5
        anchors { horizontalCenter: parent.horizontalCenter; top: h2.bottom; topMargin: 32 }

        Repeater { // create the stars
            id: rep
            model: 5
            Image {
                source: "star_empty.png"
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        for (var i = 0; i < 5; ++i) {
                            rep.itemAt(i).source = i <= index ? "star_filled.png"
                                                              : "star_empty.png"
                        }
                        rating = index + 1
                    }
                }
            }
        }
    }

    ScrollView {
        id: scroll_textarea
        width: 436
        height: 96
        anchors { horizontalCenter: parent.horizontalCenter; top: starRow.bottom; topMargin: 28 }

        TextEdit {
            id: textarea
            width: 426
            height: 90
            color: "#333333";
            font { pixelSize: 14; family: "Titillium" }
            wrapMode: Text.Wrap
            property string placeholderText: "We highly appreciate additional feedback.\nBouquets, brickbats, or suggestions, all feedback is welcome!"

            Text {
                text: textarea.placeholderText
                color: "gray"
                visible: !textarea.text
                font: parent.font
            }
        }

        background: Rectangle {
            border { color: "#e6e6e6"; width: 1 }
        }
    }

    Row {
        id: buttonRow
        anchors { horizontalCenter: parent.horizontalCenter; top: scroll_textarea.bottom; topMargin: 28 }
        spacing: 10

        Button {
            id: buttonSkip
            width: 80
            height: 28

            contentItem: Text {
                text: "Skip"
                color: parent.hovered ? Qt.darker("#999999", 1.9) : Qt.darker("#999999", 1.2)
                font { family: "Titillium"; pixelSize: 14 }
                horizontalAlignment: Text.AlignHCenter
            }

            background: Rectangle {
                anchors.fill: parent
                color: "#ffffff"
                border { color: "#999999"; width: 1 }
            }

            onClicked: root_rectangle.closeClicked()
        }

        Button {
            id: buttonSubmit

            width: 80
            height: 28
            enabled: rating > 0

            contentItem: Text {
                text: "Submit";
                color: enabled ? "white" : Qt.lighter("#999999", 1.3)
                font { family: "Titillium"; pixelSize: 14 }
                horizontalAlignment: Text.AlignHCenter
            }

            background: Rectangle {
                anchors.fill: parent
                color: enabled ? parent.hovered ? Qt.lighter("#0094ce", 1.2) : "#0094ce" : "white"
                border { color: enabled ? "#999999" : Qt.lighter("#999999", 1.3); width: 1 }
            }

            onClicked: {
                root_rectangle.submitFeedback(textarea.text, rating);
                root_rectangle.closeClicked();
            }
        }
    }
}
