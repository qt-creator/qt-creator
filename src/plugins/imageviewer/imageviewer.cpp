/**************************************************************************
**
** Copyright (C) 2013 Denis Mingulov.
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

#include "imageviewer.h"
#include "imageviewerfile.h"
#include "imagevieweractionhandler.h"
#include "imageviewerconstants.h"
#include "imageview.h"
#include "ui_imageviewertoolbar.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <utils/qtcassert.h>

#include <QMap>
#include <QFileInfo>
#include <QDir>
#include <QWidget>
#include <QDebug>

namespace ImageViewer {
namespace Internal {

struct ImageViewerPrivate
{
    QString displayName;
    ImageViewerFile *file;
    ImageView *imageView;
    QWidget *toolbar;
    Ui::ImageViewerToolbar ui_toolbar;
};

ImageViewer::ImageViewer(QWidget *parent)
    : IEditor(parent),
    d(new ImageViewerPrivate)
{
    d->file = new ImageViewerFile(this);
    d->imageView = new ImageView();

    setContext(Core::Context(Constants::IMAGEVIEWER_ID));
    setWidget(d->imageView);

    // toolbar
    d->toolbar = new QWidget();
    d->ui_toolbar.setupUi(d->toolbar);

    // icons update - try to use system theme
    updateButtonIconByTheme(d->ui_toolbar.toolButtonZoomIn, QLatin1String("zoom-in"));
    updateButtonIconByTheme(d->ui_toolbar.toolButtonZoomOut, QLatin1String("zoom-out"));
    updateButtonIconByTheme(d->ui_toolbar.toolButtonOriginalSize, QLatin1String("zoom-original"));
    updateButtonIconByTheme(d->ui_toolbar.toolButtonFitToScreen, QLatin1String("zoom-fit-best"));
    // a display - something is on the background
    updateButtonIconByTheme(d->ui_toolbar.toolButtonBackground, QLatin1String("video-display"));
    // "emblem to specify the directory where the user stores photographs"
    // (photograph has outline - piece of paper)
    updateButtonIconByTheme(d->ui_toolbar.toolButtonOutline, QLatin1String("emblem-photos"));

    d->ui_toolbar.toolButtonZoomIn->setCommandId(Constants::ACTION_ZOOM_IN);
    d->ui_toolbar.toolButtonZoomOut->setCommandId(Constants::ACTION_ZOOM_OUT);
    d->ui_toolbar.toolButtonOriginalSize->setCommandId(Constants::ACTION_ORIGINAL_SIZE);
    d->ui_toolbar.toolButtonFitToScreen->setCommandId(Constants::ACTION_FIT_TO_SCREEN);
    d->ui_toolbar.toolButtonBackground->setCommandId(Constants::ACTION_BACKGROUND);
    d->ui_toolbar.toolButtonOutline->setCommandId(Constants::ACTION_OUTLINE);
    d->ui_toolbar.toolButtonPlayPause->setCommandId(Constants::ACTION_TOGGLE_ANIMATION);

    // connections
    connect(d->file, SIGNAL(changed()), this, SIGNAL(changed()));

    connect(d->ui_toolbar.toolButtonZoomIn, SIGNAL(clicked()),
            d->imageView, SLOT(zoomIn()));
    connect(d->ui_toolbar.toolButtonZoomOut, SIGNAL(clicked()),
            d->imageView, SLOT(zoomOut()));
    connect(d->ui_toolbar.toolButtonFitToScreen, SIGNAL(clicked()),
            d->imageView, SLOT(fitToScreen()));
    connect(d->ui_toolbar.toolButtonOriginalSize, SIGNAL(clicked()),
            d->imageView, SLOT(resetToOriginalSize()));
    connect(d->ui_toolbar.toolButtonBackground, SIGNAL(toggled(bool)),
            d->imageView, SLOT(setViewBackground(bool)));
    connect(d->ui_toolbar.toolButtonOutline, SIGNAL(toggled(bool)),
            d->imageView, SLOT(setViewOutline(bool)));
    connect(d->ui_toolbar.toolButtonPlayPause, SIGNAL(clicked()),
            this, SLOT(playToggled()));
    connect(d->imageView, SIGNAL(imageSizeChanged(QSize)),
            this, SLOT(imageSizeUpdated(QSize)));
    connect(d->imageView, SIGNAL(scaleFactorChanged(qreal)),
            this, SLOT(scaleFactorUpdate(qreal)));
}

ImageViewer::~ImageViewer()
{
    delete d->imageView;
    delete d->toolbar;
    delete d;
}

bool ImageViewer::createNew(const QString &contents)
{
    Q_UNUSED(contents)
    return false;
}

bool ImageViewer::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (!d->imageView->openFile(realFileName)) {
        *errorString = tr("Cannot open image file %1.").arg(QDir::toNativeSeparators(realFileName));
        return false;
    }
    setDisplayName(QFileInfo(fileName).fileName());
    d->file->setFileName(fileName);
    d->ui_toolbar.toolButtonPlayPause->setVisible(d->imageView->isAnimated());
    setPaused(!d->imageView->isAnimated());
    // d_ptr->file->setMimeType
    emit changed();
    return true;
}

Core::IDocument *ImageViewer::document()
{
    return d->file;
}

Core::Id ImageViewer::id() const
{
    return Core::Id(Constants::IMAGEVIEWER_ID);
}

QString ImageViewer::displayName() const
{
    return d->displayName;
}

void ImageViewer::setDisplayName(const QString &title)
{
    d->displayName = title;
    emit changed();
}

bool ImageViewer::duplicateSupported() const
{
    return false;
}

Core::IEditor *ImageViewer::duplicate(QWidget *parent)
{
    Q_UNUSED(parent);
    return 0;
}

QByteArray ImageViewer::saveState() const
{
    return QByteArray();
}

bool ImageViewer::restoreState(const QByteArray &state)
{
    Q_UNUSED(state);
    return true;
}

int ImageViewer::currentLine() const
{
    return 0;
}

int ImageViewer::currentColumn() const
{
    return 0;
}

bool ImageViewer::isTemporary() const
{
    return false;
}

QWidget *ImageViewer::toolBar()
{
    return d->toolbar;
}

void ImageViewer::imageSizeUpdated(const QSize &size)
{
    QString imageSizeText;
    if (size.isValid())
        imageSizeText = QString::fromLatin1("%1x%2").arg(size.width()).arg(size.height());
    d->ui_toolbar.labelImageSize->setText(imageSizeText);
}

void ImageViewer::scaleFactorUpdate(qreal factor)
{
    const QString info = QString::number(factor * 100, 'f', 2) + QLatin1Char('%');
    d->ui_toolbar.labelInfo->setText(info);
}

bool ImageViewer::updateButtonIconByTheme(QAbstractButton *button, const QString &name)
{
    QTC_ASSERT(button, return false);
    QTC_ASSERT(!name.isEmpty(), return false);

    if (QIcon::hasThemeIcon(name)) {
        button->setIcon(QIcon::fromTheme(name));
        return true;
    }

    return false;
}

void ImageViewer::switchViewBackground()
{
    d->ui_toolbar.toolButtonBackground->click();
}

void ImageViewer::switchViewOutline()
{
    d->ui_toolbar.toolButtonOutline->click();
}

void ImageViewer::zoomIn()
{
    d->ui_toolbar.toolButtonZoomIn->click();
}

void ImageViewer::zoomOut()
{
    d->ui_toolbar.toolButtonZoomOut->click();
}

void ImageViewer::resetToOriginalSize()
{
    d->ui_toolbar.toolButtonOriginalSize->click();
}

void ImageViewer::fitToScreen()
{
    d->ui_toolbar.toolButtonFitToScreen->click();
}

void ImageViewer::togglePlay()
{
    d->ui_toolbar.toolButtonPlayPause->click();
}

void ImageViewer::playToggled()
{
    bool paused = d->imageView->isPaused();
    setPaused(!paused);
}

void ImageViewer::setPaused(bool paused)
{
    d->imageView->setPaused(paused);
    if (paused) {
        d->ui_toolbar.toolButtonPlayPause->setToolTipBase(tr("Play Animation"));
        d->ui_toolbar.toolButtonPlayPause->setIcon(QPixmap(QLatin1String(":/imageviewer/images/play-small.png")));
    } else {
        d->ui_toolbar.toolButtonPlayPause->setToolTipBase(tr("Pause Animation"));
        d->ui_toolbar.toolButtonPlayPause->setIcon(QPixmap(QLatin1String(":/imageviewer/images/pause-small.png")));
    }
}

} // namespace Internal
} // namespace ImageViewer
