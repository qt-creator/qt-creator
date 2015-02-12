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

#include "screenshotcropperwindow.h"
#include "ui_screenshotcropperwindow.h"
#include <QListWidget>
#include <QDebug>

using namespace QtSupport::Internal;

ScreenShotCropperWindow::ScreenShotCropperWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ScreenShotCropperWindow)
{
    ui->setupUi(this);
    connect(ui->m_filenamesList, SIGNAL(currentRowChanged(int)), SLOT(selectImage(int)));
    connect(ui->m_cropImageView, SIGNAL(cropAreaChanged(QRect)), SLOT(setArea(QRect)));
    connect(ui->m_buttonBox, SIGNAL(accepted()), SLOT(saveData()));
    connect(ui->m_buttonBox, SIGNAL(rejected()), SLOT(close()));
}

ScreenShotCropperWindow::~ScreenShotCropperWindow()
{
    delete ui;
}

void ScreenShotCropperWindow::loadData(const QString &areasXmlFile, const QString &imagesFolder)
{
    typedef QMap<QString, QRect>::ConstIterator StringRectConstIt;

    m_areasOfInterestFile = areasXmlFile;
    m_areasOfInterest = ScreenshotCropper::loadAreasOfInterest(m_areasOfInterestFile);
    m_imagesFolder = imagesFolder;
    const StringRectConstIt cend = m_areasOfInterest.constEnd();
    for (StringRectConstIt it = m_areasOfInterest.constBegin(); it != cend; ++it)
        ui->m_filenamesList->addItem(it.key());
}

void ScreenShotCropperWindow::selectImage(int index)
{
    const QString fileName = ui->m_filenamesList->item(index)->text();
    ui->m_cropImageView->setImage(QImage(m_imagesFolder + QLatin1Char('/') + fileName));
    ui->m_cropImageView->setArea(m_areasOfInterest.value(fileName));
}

void ScreenShotCropperWindow::setArea(const QRect &area)
{
    const QListWidgetItem *item = ui->m_filenamesList->currentItem();
    if (!item)
        return;
    const QString currentFile = item->text();
    m_areasOfInterest.insert(currentFile, area);
}

void ScreenShotCropperWindow::saveData()
{
    if (!ScreenshotCropper::saveAreasOfInterest(m_areasOfInterestFile, m_areasOfInterest))
        qFatal("Cannot write %s", qPrintable(m_areasOfInterestFile));
}
