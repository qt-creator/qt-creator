/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
