/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov.
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

#include "imageviewer.h"
#include "imageviewerfile.h"
#include "imageviewerconstants.h"
#include "imageview.h"
#include "ui_imageviewertoolbar.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

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
    d->ui_toolbar.toolButtonExportImage->setIcon(
                QIcon::fromTheme(QLatin1String("document-save"),
                                 Utils::Icons::SAVEFILE_TOOLBAR.icon()));
    const Utils::Icon backgroundIcon({
            {QLatin1String(":/utils/images/desktopdevicesmall.png"), Utils::Theme::IconsBaseColor}});
    d->ui_toolbar.toolButtonBackground->setIcon(backgroundIcon.icon());
    d->ui_toolbar.toolButtonOutline->setIcon(Utils::Icons::BOUNDING_RECT.icon());
    d->ui_toolbar.toolButtonZoomIn->setIcon(Utils::Icons::ZOOMIN_TOOLBAR.icon());
    d->ui_toolbar.toolButtonZoomOut->setIcon(Utils::Icons::ZOOMOUT_TOOLBAR.icon());
    d->ui_toolbar.toolButtonFitToScreen->setIcon(Utils::Icons::FITTOVIEW_TOOLBAR.icon());
    d->ui_toolbar.toolButtonOriginalSize->setIcon(Utils::Icons::EYE_OPEN_TOOLBAR.icon());
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

    d->ui_toolbar.toolButtonExportImage->setCommandId(Constants::ACTION_EXPORT_IMAGE);
    d->ui_toolbar.toolButtonZoomIn->setCommandId(Constants::ACTION_ZOOM_IN);
    d->ui_toolbar.toolButtonZoomOut->setCommandId(Constants::ACTION_ZOOM_OUT);
    d->ui_toolbar.toolButtonOriginalSize->setCommandId(Constants::ACTION_ORIGINAL_SIZE);
    d->ui_toolbar.toolButtonFitToScreen->setCommandId(Constants::ACTION_FIT_TO_SCREEN);
    d->ui_toolbar.toolButtonBackground->setCommandId(Constants::ACTION_BACKGROUND);
    d->ui_toolbar.toolButtonOutline->setCommandId(Constants::ACTION_OUTLINE);
    d->ui_toolbar.toolButtonPlayPause->setCommandId(Constants::ACTION_TOGGLE_ANIMATION);

    // connections
    connect(d->ui_toolbar.toolButtonExportImage, &QAbstractButton::clicked,
            d->imageView, &ImageView::exportImage);
    connect(d->ui_toolbar.toolButtonZoomIn, &QAbstractButton::clicked,
            d->imageView, &ImageView::zoomIn);
    connect(d->ui_toolbar.toolButtonZoomOut, &QAbstractButton::clicked,
            d->imageView, &ImageView::zoomOut);
    connect(d->ui_toolbar.toolButtonFitToScreen, &QAbstractButton::clicked,
            d->imageView, &ImageView::fitToScreen);
    connect(d->ui_toolbar.toolButtonOriginalSize, &QAbstractButton::clicked,
            d->imageView, &ImageView::resetToOriginalSize);
    connect(d->ui_toolbar.toolButtonBackground, &QAbstractButton::toggled,
            d->imageView, &ImageView::setViewBackground);
    connect(d->ui_toolbar.toolButtonOutline, &QAbstractButton::toggled,
            d->imageView, &ImageView::setViewOutline);
    connect(d->ui_toolbar.toolButtonPlayPause, &Core::CommandButton::clicked,
            this, &ImageViewer::playToggled);
    connect(d->file.data(), &ImageViewerFile::imageSizeChanged,
            this, &ImageViewer::imageSizeUpdated);
    connect(d->file.data(), &ImageViewerFile::openFinished,
            d->imageView, &ImageView::createScene);
    connect(d->file.data(), &ImageViewerFile::openFinished,
            this, &ImageViewer::updateToolButtons);
    connect(d->file.data(), &ImageViewerFile::aboutToReload,
            d->imageView, &ImageView::reset);
    connect(d->file.data(), &ImageViewerFile::reloadFinished,
            d->imageView, &ImageView::createScene);
    connect(d->file.data(), &ImageViewerFile::isPausedChanged,
            this, &ImageViewer::updatePauseAction);
    connect(d->imageView, &ImageView::scaleFactorChanged,
            this, &ImageViewer::scaleFactorUpdate);
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
    other->updateToolButtons();
    other->d->ui_toolbar.labelImageSize->setText(d->ui_toolbar.labelImageSize->text());
    return other;
}

void ImageViewer::exportImage()
{
    if (d->file->type() == ImageViewerFile::TypeSvg)
        d->ui_toolbar.toolButtonExportImage->click();
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

void ImageViewer::updateToolButtons()
{
    d->ui_toolbar.toolButtonExportImage->setEnabled(d->file->type() == ImageViewerFile::TypeSvg);
    updatePauseAction();
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
    if (isMovie && !d->file->isPaused()) {
        d->ui_toolbar.toolButtonPlayPause->setToolTipBase(tr("Pause Animation"));
        d->ui_toolbar.toolButtonPlayPause->setIcon(Utils::Icons::INTERRUPT_SMALL_TOOLBAR.icon());
    } else {
        d->ui_toolbar.toolButtonPlayPause->setToolTipBase(tr("Play Animation"));
        d->ui_toolbar.toolButtonPlayPause->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
        d->ui_toolbar.toolButtonPlayPause->setEnabled(isMovie);
    }
}

} // namespace Internal
} // namespace ImageViewer
