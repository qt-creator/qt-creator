/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include <QApplication>
#include <QFileDialog>
#include <QSettings>

#include "screenshotcropperwindow.h"

using namespace QtSupport::Internal;

const QString settingsKeyAreasXmlFile = QLatin1String("areasXmlFile");
const QString settingsKeyImagesFolder = QLatin1String("imagesFolder");

static void promptPaths(QString &areasXmlFile, QString &imagesFolder)
{
    QSettings settings(QLatin1String("Nokia"), QLatin1String("Qt Creator Screenshot Cropper"));

    areasXmlFile = settings.value(settingsKeyAreasXmlFile).toString();
    areasXmlFile = QFileDialog::getOpenFileName(0, QLatin1String("Select the 'images_areaofinterest.xml' file in Qt Creator's sources"), areasXmlFile);
    settings.setValue(settingsKeyAreasXmlFile, areasXmlFile);

    imagesFolder = settings.value(settingsKeyImagesFolder).toString();
    imagesFolder = QFileDialog::getExistingDirectory(0, QLatin1String("Select the 'doc/src/images' folder in Qt's sources"), imagesFolder);
    settings.setValue(settingsKeyImagesFolder, imagesFolder);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString areasXmlFile;
    QString imagesFolder;
    promptPaths(areasXmlFile, imagesFolder);

    ScreenShotCropperWindow w;
    w.show();
    w.loadData(areasXmlFile, imagesFolder);
    w.selectImage(0);

    return a.exec();
}
