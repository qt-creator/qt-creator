/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QApplication>
#include <QDebug>
#include <QProcess>

#include "iconlister.h"

enum AppMode {
    GenerateAll,
    GenerateJson,
    GenerateIconsLowDpiLight,
    GenerateIconsLowDpiDark,
    GenerateIconsHighDpiLight,
    GenerateIconsHighDpiDark
};

AppMode appModeForString(const char* string)
{
    AppMode appMode;
    if (strcmp(string, "GenerateJson") == 0)
        appMode = GenerateJson;
    else if (strcmp(string, "GenerateIconsLowDpiLight") == 0)
        appMode = GenerateIconsLowDpiLight;
    else if (strcmp(string, "GenerateIconsLowDpiDark") == 0)
        appMode = GenerateIconsLowDpiDark;
    else if (strcmp(string, "GenerateIconsHighDpiLight") == 0)
        appMode = GenerateIconsHighDpiLight;
    else
        appMode = GenerateIconsHighDpiDark;
    return appMode;
}

int launchSubProcesses(const QString &command)
{
    for (auto launchAppMode : {
             "GenerateJson",
             "GenerateIconsLowDpiLight",
             "GenerateIconsLowDpiDark",
             "GenerateIconsHighDpiLight",
             "GenerateIconsHighDpiDark"}) {
        qDebug() << "Launching step:" << launchAppMode;
        const int result =
                QProcess::execute(command, {QString::fromLatin1(launchAppMode)});
        if (result != 0)
            return result;
    }
    return 0;
}

void exportData(AppMode appMode)
{
    if (appMode == GenerateJson) {
        IconLister::generateJson();
    } else {
        const int dpr = appMode ==
                GenerateIconsHighDpiLight || appMode == GenerateIconsHighDpiDark
                ? 2 : 1;
        const IconLister::ThemeKind theme =
                appMode == GenerateIconsLowDpiLight || appMode == GenerateIconsHighDpiLight
                ? IconLister::ThemeKindLight : IconLister::ThemeKindDark;
        IconLister::generateIcons(theme, dpr);
    }
}

int main(int argc, char *argv[])
{
    const AppMode appMode = (argc <= 1)
            ? GenerateAll : appModeForString(argv[1]);

    const bool highDpi =
            appMode == GenerateIconsHighDpiLight || appMode == GenerateIconsHighDpiDark;

    qputenv("QT_SCALE_FACTOR", QByteArray(highDpi ? "2" : "1"));

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, highDpi);
    QApplication app(argc, argv);

    if (appMode == GenerateAll)
        return launchSubProcesses(QString::fromLocal8Bit(argv[0]));
    else
        exportData(appMode);

    return 0;
}
