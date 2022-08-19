// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15

Item {
    id: root
    state: "start"

    width: 480
    height: 272

    StartScreen {
        id: startScreen

        visible: true

        onStartClicked: {
            root.state = "presets"
        }

        onSettingsClicked: {
            root.state = "settings"
        }
    }

    SettingsScreen {
        id: settingsScreen

        visible: false

        onSettingsClosed: {
            root.state = "start"
        }
    }

    PresetsScreen {
        id: presetsScreen

        visible: false

        onStartRun: {
            root.state = "running"
            runningScreen.startRun()
        }

        onCancelPresets: {
            root.state = "start"
        }
    }

    RunningScreen {
        id: runningScreen

        runDuration: presetsScreen.runDuration
        visible: false

        onRunFinished: {
            root.state = "start"
        }
    }

    states: [
        State {
            name: "start"

            PropertyChanges {
                target: startScreen
                visible: true
            }

            PropertyChanges {
                target: settingsScreen
                visible: false
            }

            PropertyChanges {
                target: presetsScreen
                visible: false
            }

            PropertyChanges {
                target: runningScreen
                visible: false
                activated: false
            }
        },
        State {
            name: "settings"

            PropertyChanges {
                target: startScreen
                visible: false
            }

            PropertyChanges {
                target: settingsScreen
                visible: true
            }

            PropertyChanges {
                target: presetsScreen
                visible: false
            }

            PropertyChanges {
                target: runningScreen
                visible: false
                activated: false
            }
        },
        State {
            name: "presets"

            PropertyChanges {
                target: startScreen
                visible: false
            }

            PropertyChanges {
                target: settingsScreen
                visible: false
            }

            PropertyChanges {
                target: presetsScreen
                visible: true
            }

            PropertyChanges {
                target: runningScreen
                visible: false
                activated: false
            }
        },
        State {
            name: "running"

            PropertyChanges {
                target: startScreen
                visible: false
            }

            PropertyChanges {
                target: settingsScreen
                visible: false
            }

            PropertyChanges {
                target: presetsScreen
                visible: false
            }

            PropertyChanges {
                target: runningScreen
                visible: true
                activated: true
            }
        }

    ]
}
