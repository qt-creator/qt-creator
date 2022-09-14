// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "imageviewer.h"

#include "imageview.h"
#include "imageviewerconstants.h"
#include "imageviewerfile.h"
#include "imageviewertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/commandbutton.h>

#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/styledbar.h>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMap>
#include <QSpacerItem>
#include <QWidget>
#include <QWidget>

using namespace Core;
using namespace Utils;

namespace ImageViewer::Internal{

struct ImageViewerPrivate
{
    QString displayName;
    QSharedPointer<ImageViewerFile> file;
    ImageView *imageView;
    QWidget *toolbar;

    CommandButton *toolButtonExportImage;
    CommandButton *toolButtonMultiExportImages;
    CommandButton *toolButtonCopyDataUrl;
    CommandButton *toolButtonBackground;
    CommandButton *toolButtonOutline;
    CommandButton *toolButtonFitToScreen;
    CommandButton *toolButtonOriginalSize;
    CommandButton *toolButtonZoomIn;
    CommandButton *toolButtonZoomOut;
    CommandButton *toolButtonPlayPause;
    QLabel *labelImageSize;
    QLabel *labelInfo;
};

/*!
    Tries to change the \a button icon to the icon specified by \a name
    from the current theme. Returns \c true if icon is updated, \c false
    otherwise.
*/
static bool updateButtonIconByTheme(QAbstractButton *button, const QString &name)
{
    QTC_ASSERT(!name.isEmpty(), return false);

    if (QIcon::hasThemeIcon(name)) {
        button->setIcon(QIcon::fromTheme(name));
        return true;
    }

    return false;
}

ImageViewer::ImageViewer()
    : d(new ImageViewerPrivate)
{
    d->file.reset(new ImageViewerFile);
    ctor();
}

ImageViewer::ImageViewer(const QSharedPointer<ImageViewerFile> &document)
    : d(new ImageViewerPrivate)
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
    d->toolbar = new QWidget;

    d->toolButtonExportImage = new CommandButton;
    d->toolButtonMultiExportImages = new CommandButton;
    d->toolButtonCopyDataUrl = new CommandButton;
    d->toolButtonBackground = new CommandButton;
    d->toolButtonOutline = new CommandButton;
    d->toolButtonFitToScreen = new CommandButton;
    d->toolButtonOriginalSize = new CommandButton;
    d->toolButtonZoomIn = new CommandButton;
    d->toolButtonZoomOut = new CommandButton;
    d->toolButtonPlayPause = new CommandButton;

    d->toolButtonBackground->setCheckable(true);
    d->toolButtonBackground->setChecked(false);

    d->toolButtonOutline->setCheckable(true);
    d->toolButtonOutline->setChecked(true);

    d->toolButtonFitToScreen->setCheckable(true);

    d->toolButtonZoomIn->setAutoRepeat(true);

    d->toolButtonZoomOut->setAutoRepeat(true);

    d->toolButtonExportImage->setToolTipBase(Tr::tr("Export as Image"));
    d->toolButtonMultiExportImages->setToolTipBase(Tr::tr("Export Images of Multiple Sizes"));
    d->toolButtonOutline->setToolTipBase(Tr::tr("Show Outline"));
    d->toolButtonFitToScreen->setToolTipBase(Tr::tr("Fit to Screen"));
    d->toolButtonOriginalSize->setToolTipBase(Tr::tr("Original Size"));
    d->toolButtonZoomIn->setToolTipBase(Tr::tr("Zoom In"));
    d->toolButtonZoomOut->setToolTipBase(Tr::tr("Zoom Out"));

    d->toolButtonExportImage->setIcon(Icons::EXPORTFILE_TOOLBAR.icon());
    d->toolButtonMultiExportImages->setIcon(Icons::MULTIEXPORTFILE_TOOLBAR.icon());
    d->toolButtonCopyDataUrl->setIcon(Icons::COPY_TOOLBAR.icon());
    const Icon backgroundIcon({{":/utils/images/desktopdevicesmall.png", Theme::IconsBaseColor}});
    d->toolButtonBackground->setIcon(backgroundIcon.icon());
    d->toolButtonOutline->setIcon(Icons::BOUNDING_RECT.icon());
    d->toolButtonZoomIn->setIcon(
                ActionManager::command(Core::Constants::ZOOM_IN)->action()->icon());
    d->toolButtonZoomOut->setIcon(
                ActionManager::command(Core::Constants::ZOOM_OUT)->action()->icon());
    d->toolButtonOriginalSize->setIcon(
                ActionManager::command(Core::Constants::ZOOM_RESET)->action()->icon());
    d->toolButtonFitToScreen->setIcon(Icons::FITTOVIEW_TOOLBAR.icon());

    // icons update - try to use system theme
    updateButtonIconByTheme(d->toolButtonFitToScreen, QLatin1String("zoom-fit-best"));
    // a display - something is on the background
    updateButtonIconByTheme(d->toolButtonBackground, QLatin1String("video-display"));
    // "emblem to specify the directory where the user stores photographs"
    // (photograph has outline - piece of paper)
    updateButtonIconByTheme(d->toolButtonOutline, QLatin1String("emblem-photos"));

    d->toolButtonExportImage->setCommandId(Constants::ACTION_EXPORT_IMAGE);
    d->toolButtonMultiExportImages->setCommandId(Constants::ACTION_EXPORT_MULTI_IMAGES);
    d->toolButtonCopyDataUrl->setCommandId(Constants::ACTION_COPY_DATA_URL);
    d->toolButtonZoomIn->setCommandId(Core::Constants::ZOOM_IN);
    d->toolButtonZoomOut->setCommandId(Core::Constants::ZOOM_OUT);
    d->toolButtonOriginalSize->setCommandId(Core::Constants::ZOOM_RESET);
    d->toolButtonFitToScreen->setCommandId(Constants::ACTION_FIT_TO_SCREEN);
    d->toolButtonBackground->setCommandId(Constants::ACTION_BACKGROUND);
    d->toolButtonOutline->setCommandId(Constants::ACTION_OUTLINE);
    d->toolButtonPlayPause->setCommandId(Constants::ACTION_TOGGLE_ANIMATION);

    d->labelImageSize = new QLabel;
    d->labelInfo = new QLabel;

    auto horizontalLayout = new QHBoxLayout(d->toolbar);
    horizontalLayout->setSpacing(0);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->addWidget(d->toolButtonExportImage);
    horizontalLayout->addWidget(d->toolButtonMultiExportImages);
    horizontalLayout->addWidget(d->toolButtonCopyDataUrl);
    horizontalLayout->addWidget(d->toolButtonBackground);
    horizontalLayout->addWidget(d->toolButtonOutline);
    horizontalLayout->addWidget(d->toolButtonFitToScreen);
    horizontalLayout->addWidget(d->toolButtonOriginalSize);
    horizontalLayout->addWidget(d->toolButtonZoomIn);
    horizontalLayout->addWidget(d->toolButtonZoomOut);
    horizontalLayout->addWidget(d->toolButtonPlayPause);
    horizontalLayout->addWidget(d->toolButtonPlayPause);
    horizontalLayout->addWidget(new StyledSeparator);
    horizontalLayout->addItem(new QSpacerItem(315, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    horizontalLayout->addWidget(new StyledSeparator);
    horizontalLayout->addWidget(d->labelImageSize);
    horizontalLayout->addWidget(new StyledSeparator);
    horizontalLayout->addWidget(d->labelInfo);

    // connections
    connect(d->toolButtonExportImage, &QAbstractButton::clicked,
            d->imageView, &ImageView::exportImage);
    connect(d->toolButtonMultiExportImages, &QAbstractButton::clicked,
            d->imageView, &ImageView::exportMultiImages);
    connect(d->toolButtonCopyDataUrl, &QAbstractButton::clicked,
            d->imageView, &ImageView::copyDataUrl);
    connect(d->toolButtonZoomIn, &QAbstractButton::clicked,
            d->imageView, &ImageView::zoomIn);
    connect(d->toolButtonZoomOut, &QAbstractButton::clicked,
            d->imageView, &ImageView::zoomOut);
    connect(d->toolButtonFitToScreen,
            &QAbstractButton::toggled,
            d->imageView,
            &ImageView::setFitToScreen);
    connect(d->imageView,
            &ImageView::fitToScreenChanged,
            d->toolButtonFitToScreen,
            &QAbstractButton::setChecked);
    connect(d->toolButtonOriginalSize,
            &QAbstractButton::clicked,
            d->imageView,
            &ImageView::resetToOriginalSize);
    connect(d->toolButtonBackground, &QAbstractButton::toggled,
            d->imageView, &ImageView::setViewBackground);
    connect(d->toolButtonOutline, &QAbstractButton::toggled,
            d->imageView, &ImageView::setViewOutline);
    connect(d->toolButtonPlayPause, &CommandButton::clicked,
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

IDocument *ImageViewer::document() const
{
    return d->file.data();
}

QWidget *ImageViewer::toolBar()
{
    return d->toolbar;
}

IEditor *ImageViewer::duplicate()
{
    auto other = new ImageViewer(d->file);
    other->d->imageView->createScene();
    other->updateToolButtons();
    other->d->labelImageSize->setText(d->labelImageSize->text());

    emit editorDuplicated(other);

    return other;
}

void ImageViewer::exportImage()
{
    if (d->file->type() == ImageViewerFile::TypeSvg)
        d->toolButtonExportImage->click();
}

void ImageViewer::exportMultiImages()
{
    if (d->file->type() == ImageViewerFile::TypeSvg)
        d->toolButtonMultiExportImages->click();
}

void ImageViewer::copyDataUrl()
{
    d->toolButtonCopyDataUrl->click();
}

void ImageViewer::imageSizeUpdated(const QSize &size)
{
    QString imageSizeText;
    if (size.isValid())
        imageSizeText = QString::fromLatin1("%1x%2").arg(size.width()).arg(size.height());
    d->labelImageSize->setText(imageSizeText);
}

void ImageViewer::scaleFactorUpdate(qreal factor)
{
    const QString info = QString::number(factor * 100, 'f', 2) + QLatin1Char('%');
    d->labelInfo->setText(info);
}

void ImageViewer::switchViewBackground()
{
    d->toolButtonBackground->click();
}

void ImageViewer::switchViewOutline()
{
    d->toolButtonOutline->click();
}

void ImageViewer::zoomIn()
{
    d->toolButtonZoomIn->click();
}

void ImageViewer::zoomOut()
{
    d->toolButtonZoomOut->click();
}

void ImageViewer::resetToOriginalSize()
{
    d->toolButtonOriginalSize->click();
}

void ImageViewer::fitToScreen()
{
    d->toolButtonFitToScreen->click();
}

void ImageViewer::updateToolButtons()
{
    const bool isSvg = d->file->type() == ImageViewerFile::TypeSvg;
    d->toolButtonExportImage->setEnabled(isSvg);
    d->toolButtonMultiExportImages->setEnabled(isSvg);
    updatePauseAction();
}

void ImageViewer::togglePlay()
{
    d->toolButtonPlayPause->click();
}

void ImageViewer::playToggled()
{
    d->file->setPaused(!d->file->isPaused());
}

void ImageViewer::updatePauseAction()
{
    bool isMovie = d->file->type() == ImageViewerFile::TypeMovie;
    if (isMovie && !d->file->isPaused()) {
        d->toolButtonPlayPause->setToolTipBase(Tr::tr("Pause Animation"));
        d->toolButtonPlayPause->setIcon(Icons::INTERRUPT_SMALL_TOOLBAR.icon());
    } else {
        d->toolButtonPlayPause->setToolTipBase(Tr::tr("Play Animation"));
        d->toolButtonPlayPause->setIcon(Icons::RUN_SMALL_TOOLBAR.icon());
        d->toolButtonPlayPause->setEnabled(isMovie);
    }
}

// Factory

ImageViewerFactory::ImageViewerFactory()
{
    setId(Constants::IMAGEVIEWER_ID);
    setDisplayName(Tr::tr("Image Viewer"));
    setEditorCreator([] { return new ImageViewer; });

    const QList<QByteArray> supportedMimeTypes = QImageReader::supportedMimeTypes();
    for (const QByteArray &format : supportedMimeTypes)
        addMimeType(QString::fromLatin1(format));
}

} // ImageViewer::Internal
