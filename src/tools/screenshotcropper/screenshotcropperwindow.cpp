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
    connect(ui->m_filenamesList, &QListWidget::currentRowChanged, this, &ScreenShotCropperWindow::selectImage);
    connect(ui->m_cropImageView, &CropImageView::cropAreaChanged, this, &ScreenShotCropperWindow::setArea);
    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &ScreenShotCropperWindow::saveData);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);
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
