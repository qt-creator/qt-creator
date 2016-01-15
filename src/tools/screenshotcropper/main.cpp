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

#include <QApplication>
#include <QFileDialog>
#include <QSettings>

#include "screenshotcropperwindow.h"

using namespace QtSupport::Internal;

const QString settingsKeyAreasXmlFile = QLatin1String("areasXmlFile");
const QString settingsKeyImagesFolder = QLatin1String("imagesFolder");

static void promptPaths(QString &areasXmlFile, QString &imagesFolder)
{
    QSettings settings(QLatin1String("QtProject"), QLatin1String("Qt Creator Screenshot Cropper"));

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
