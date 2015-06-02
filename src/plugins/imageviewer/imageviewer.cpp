/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov.
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

#include "imageviewer.h"
#include "imageviewerfile.h"
#include "imagevieweractionhandler.h"
#include "imageviewerconstants.h"
#include "imageview.h"
#include "ui_imageviewertoolbar.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <utils/fileutils.h>
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
    QSharedPointer<ImageViewerFile> file;
    ImageView *imageView;
    QWidget *toolbar;
    Ui::ImageViewerToolbar ui_toolbar;
};

/*!
    Tries to change the \a button icon to the icon specified by \a name
    from the current theme. Returns \c true if icon is updated, \c false
    otherwise.
*/
static bool updateButtonIconByTheme(QAbstractButton *button, const QString &name)
{
    QTC_ASSERT(button, return false);
    QTC_ASSERT(!name.isEmpty(), return false);

    if (QIcon::hasThemeIcon(name)) {
        button->setIcon(QIcon::fromTheme(name));
        return true;
    }

    return false;
}

ImageViewer::ImageViewer(QWidget *parent)
    : IEditor(parent),
    d(new ImageViewerPrivate)
{
    d->file.reset(new ImageViewerFile);
    ctor();
}

ImageViewer::ImageViewer(const QSharedPointer<ImageViewerFile> &document, QWidget *parent)
    : IEditor(parent),
      d(new ImageViewerPrivate)
{
    d->file = document;
    ctor();
}

void ImageViewer::ctor()
{
    d->imageView = new ImageView(d->file.data());

    setContext(Core::Context(Constants::IMAGEVIEWER_ID));
    setWidget(d->imageView);
    setDuplicateSupported(true);

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
    connect(d->ui_toolbar.toolButtonPlayPause, &Core::CommandButton::clicked,
            this, &ImageViewer::playToggled);
    connect(d->file.data(), &ImageViewerFile::imageSizeChanged,
            this, &ImageViewer::imageSizeUpdated);
    connect(d->file.data(), &ImageViewerFile::openFinished,
            d->imageView, &ImageView::createScene);
    connect(d->file.data(), &ImageViewerFile::aboutToReload,
            d->imageView, &ImageView::reset);
    connect(d->file.data(), &ImageViewerFile::reloadFinished,
            d->imageView, &ImageView::createScene);
    connect(d->file.data(), &ImageViewerFile::isPausedChanged,
            this, &ImageViewer::updatePauseAction);
    connect(d->imageView, SIGNAL(scaleFactorChanged(qreal)),
            this, SLOT(scaleFactorUpdate(qreal)));
}

ImageViewer::~ImageViewer()
{
    delete d->imageView;
    delete d->toolbar;
    delete d;
}

Core::IDocument *ImageViewer::document()
{
    return d->file.data();
}

QWidget *ImageViewer::toolBar()
{
    return d->toolbar;
}

Core::IEditor *ImageViewer::duplicate()
{
    auto other = new ImageViewer(d->file);
    other->d->imageView->createScene();
    return other;
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
    d->file->setPaused(!d->file->isPaused());
}

void ImageViewer::updatePauseAction()
{
    bool isMovie = d->file->type() == ImageViewerFile::TypeMovie;
    d->ui_toolbar.toolButtonPlayPause->setVisible(isMovie);
    if (isMovie) {
        if (d->file->isPaused()) {
            d->ui_toolbar.toolButtonPlayPause->setToolTipBase(tr("Play Animation"));
            d->ui_toolbar.toolButtonPlayPause->setIcon(QPixmap(QLatin1String(":/imageviewer/images/play-small.png")));
        } else {
            d->ui_toolbar.toolButtonPlayPause->setToolTipBase(tr("Pause Animation"));
            d->ui_toolbar.toolButtonPlayPause->setIcon(QPixmap(QLatin1String(":/imageviewer/images/pause-small.png")));
        }
    }
}

} // namespace Internal
} // namespace ImageViewer
