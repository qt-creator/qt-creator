// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QSpacerItem>
#include <QToolBar>
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

    QToolButton *shareButton;
    CommandAction *actionExportImage;
    CommandAction *actionMultiExportImages;
    CommandAction *actionButtonCopyDataUrl;
    CommandAction *actionBackground;
    CommandAction *actionOutline;
    CommandAction *actionFitToScreen;
    CommandAction *actionOriginalSize;
    CommandAction *actionZoomIn;
    CommandAction *actionZoomOut;
    CommandAction *actionPlayPause;
    QLabel *labelImageSize;
    QLabel *labelInfo;
};

/*!
    Tries to change the \a button icon to the icon specified by \a name
    from the current theme. Returns \c true if icon is updated, \c false
    otherwise.
*/
static bool updateIconByTheme(QAction *action, const QString &name)
{
    QTC_ASSERT(!name.isEmpty(), return false);

    if (QIcon::hasThemeIcon(name)) {
        action->setIcon(QIcon::fromTheme(name));
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
    d->imageView->readSettings(ICore::settings());
    const ImageView::Settings settings = d->imageView->settings();

    setContext(Core::Context(Constants::IMAGEVIEWER_ID));
    setWidget(d->imageView);
    setDuplicateSupported(true);

    // toolbar
    d->toolbar = new StyledBar;

    d->actionExportImage = new CommandAction(Constants::ACTION_EXPORT_IMAGE, d->toolbar);
    d->actionMultiExportImages = new CommandAction(Constants::ACTION_EXPORT_MULTI_IMAGES,
                                                   d->toolbar);
    d->actionButtonCopyDataUrl = new CommandAction(Constants::ACTION_COPY_DATA_URL, d->toolbar);
    d->shareButton = new QToolButton;
    d->shareButton->setToolTip(Tr::tr("Export"));
    d->shareButton->setPopupMode(QToolButton::InstantPopup);
    d->shareButton->setIcon(Icons::EXPORTFILE_TOOLBAR.icon());
    d->shareButton->setProperty(StyleHelper::C_NO_ARROW, true);
    auto shareMenu = new QMenu(d->shareButton);
    shareMenu->addAction(d->actionExportImage);
    shareMenu->addAction(d->actionMultiExportImages);
    shareMenu->addAction(d->actionButtonCopyDataUrl);
    d->shareButton->setMenu(shareMenu);

    d->actionBackground = new CommandAction(Constants::ACTION_BACKGROUND, d->toolbar);
    d->actionOutline = new CommandAction(Constants::ACTION_OUTLINE, d->toolbar);
    d->actionFitToScreen = new CommandAction(Constants::ACTION_FIT_TO_SCREEN, d->toolbar);
    d->actionOriginalSize = new CommandAction(Core::Constants::ZOOM_RESET, d->toolbar);
    d->actionZoomIn = new CommandAction(Core::Constants::ZOOM_IN, d->toolbar);
    d->actionZoomOut = new CommandAction(Core::Constants::ZOOM_OUT, d->toolbar);
    d->actionPlayPause = new CommandAction(Constants::ACTION_TOGGLE_ANIMATION, d->toolbar);

    d->actionBackground->setCheckable(true);
    d->actionBackground->setChecked(settings.showBackground);

    d->actionOutline->setCheckable(true);
    d->actionOutline->setChecked(settings.showOutline);

    d->actionFitToScreen->setCheckable(true);
    d->actionFitToScreen->setChecked(settings.fitToScreen);

    d->actionZoomIn->setAutoRepeat(true);

    d->actionZoomOut->setAutoRepeat(true);

    const Icon backgroundIcon({{":/utils/images/desktopdevicesmall.png", Theme::IconsBaseColor}});
    d->actionBackground->setIcon(backgroundIcon.icon());
    d->actionOutline->setIcon(Icons::BOUNDING_RECT.icon());
    d->actionZoomIn->setIcon(ActionManager::command(Core::Constants::ZOOM_IN)->action()->icon());
    d->actionZoomOut->setIcon(ActionManager::command(Core::Constants::ZOOM_OUT)->action()->icon());
    d->actionOriginalSize->setIcon(
        ActionManager::command(Core::Constants::ZOOM_RESET)->action()->icon());
    d->actionFitToScreen->setIcon(Icons::FITTOVIEW_TOOLBAR.icon());

    // icons update - try to use system theme
    updateIconByTheme(d->actionFitToScreen, QLatin1String("zoom-fit-best"));
    // a display - something is on the background
    updateIconByTheme(d->actionBackground, QLatin1String("video-display"));
    // "emblem to specify the directory where the user stores photographs"
    // (photograph has outline - piece of paper)
    updateIconByTheme(d->actionOutline, QLatin1String("emblem-photos"));

    auto setAsDefault = new QAction(Tr::tr("Set as Default"), d->toolbar);
    const auto updateSetAsDefaultToolTip = [this, setAsDefault] {
        const ImageView::Settings settings = d->imageView->settings();
        const QString on = Tr::tr("on");
        const QString off = Tr::tr("off");
        setAsDefault->setToolTip(
            "<p>"
            + Tr::tr("Use the current settings for background, outline, and fitting "
                     "to screen as the default for new image viewers. Current default:")
            + "</p><p><ul><li>" + Tr::tr("Background: %1").arg(settings.showBackground ? on : off)
            + "</li><li>" + Tr::tr("Outline: %1").arg(settings.showOutline ? on : off) + "</li><li>"
            + Tr::tr("Fit to Screen: %1").arg(settings.fitToScreen ? on : off) + "</li></ul>");
    };
    updateSetAsDefaultToolTip();

    d->labelImageSize = new QLabel;
    d->labelInfo = new QLabel;

    auto bar = new QToolBar;
    bar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    bar->addWidget(d->shareButton);
    bar->addSeparator();
    bar->addAction(d->actionOriginalSize);
    bar->addAction(d->actionZoomIn);
    bar->addAction(d->actionZoomOut);
    bar->addAction(d->actionPlayPause);
    bar->addAction(d->actionPlayPause);
    bar->addSeparator();
    bar->addAction(d->actionBackground);
    bar->addAction(d->actionOutline);
    bar->addAction(d->actionFitToScreen);
    bar->addAction(setAsDefault);

    auto horizontalLayout = new QHBoxLayout(d->toolbar);
    horizontalLayout->setSpacing(0);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->addWidget(bar);
    horizontalLayout->addItem(
        new QSpacerItem(315, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    horizontalLayout->addWidget(new StyledSeparator);
    horizontalLayout->addWidget(d->labelImageSize);
    horizontalLayout->addWidget(new StyledSeparator);
    horizontalLayout->addWidget(d->labelInfo);

    // connections
    connect(d->actionExportImage, &QAction::triggered, d->imageView, &ImageView::exportImage);
    connect(d->actionMultiExportImages,
            &QAction::triggered,
            d->imageView,
            &ImageView::exportMultiImages);
    connect(d->actionButtonCopyDataUrl, &QAction::triggered, d->imageView, &ImageView::copyDataUrl);
    connect(d->actionZoomIn, &QAction::triggered, d->imageView, &ImageView::zoomIn);
    connect(d->actionZoomOut, &QAction::triggered, d->imageView, &ImageView::zoomOut);
    connect(d->actionFitToScreen, &QAction::triggered, d->imageView, &ImageView::setFitToScreen);
    connect(d->imageView,
            &ImageView::fitToScreenChanged,
            d->actionFitToScreen,
            &QAction::setChecked);
    connect(d->actionOriginalSize,
            &QAction::triggered,
            d->imageView,
            &ImageView::resetToOriginalSize);
    connect(d->actionBackground, &QAction::toggled, d->imageView, &ImageView::setViewBackground);
    connect(d->actionOutline, &QAction::toggled, d->imageView, &ImageView::setViewOutline);
    connect(d->actionPlayPause, &QAction::triggered, this, &ImageViewer::playToggled);
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
    connect(d->file.data(), &ImageViewerFile::movieStateChanged,
            this, &ImageViewer::updatePauseAction);
    connect(d->imageView, &ImageView::scaleFactorChanged,
            this, &ImageViewer::scaleFactorUpdate);
    connect(setAsDefault, &QAction::triggered, d->imageView, [this, updateSetAsDefaultToolTip] {
        d->imageView->writeSettings(ICore::settings());
        updateSetAsDefaultToolTip();
    });
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
        d->actionExportImage->trigger();
}

void ImageViewer::exportMultiImages()
{
    if (d->file->type() == ImageViewerFile::TypeSvg)
        d->actionMultiExportImages->trigger();
}

void ImageViewer::copyDataUrl()
{
    d->actionButtonCopyDataUrl->trigger();
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
    d->actionBackground->trigger();
}

void ImageViewer::switchViewOutline()
{
    d->actionOutline->trigger();
}

void ImageViewer::zoomIn()
{
    d->actionZoomIn->trigger();
}

void ImageViewer::zoomOut()
{
    d->actionZoomOut->trigger();
}

void ImageViewer::resetToOriginalSize()
{
    d->actionOriginalSize->trigger();
}

void ImageViewer::fitToScreen()
{
    d->actionFitToScreen->trigger();
}

void ImageViewer::updateToolButtons()
{
    const bool isSvg = d->file->type() == ImageViewerFile::TypeSvg;
    d->actionExportImage->setEnabled(isSvg);
    d->actionMultiExportImages->setEnabled(isSvg);
    updatePauseAction();
}

void ImageViewer::togglePlay()
{
    d->actionPlayPause->trigger();
}

void ImageViewer::playToggled()
{
    QMovie *m = d->file->movie();
    if (!m)
        return;
    const QMovie::MovieState state = d->file->movie()->state();
    switch (state) {
    case QMovie::NotRunning:
        m->start();
        break;
    case QMovie::Paused:
        m->setPaused(false);
        break;
    case QMovie::Running:
        m->setPaused(true);
        break;
    }
}

void ImageViewer::updatePauseAction()
{
    const bool isMovie = d->file->type() == ImageViewerFile::TypeMovie;
    const QMovie::MovieState state = isMovie ? d->file->movie()->state() : QMovie::NotRunning;
    CommandAction *a = d->actionPlayPause;
    switch (state) {
    case QMovie::NotRunning:
        a->setToolTipBase(Tr::tr("Play Animation"));
        a->setIcon(Icons::RUN_SMALL_TOOLBAR.icon());
        a->setEnabled(isMovie);
        break;
    case QMovie::Paused:
        a->setToolTipBase(Tr::tr("Resume Paused Animation"));
        a->setIcon(Icons::CONTINUE_SMALL_TOOLBAR.icon());
        break;
    case QMovie::Running:
        a->setToolTipBase(Tr::tr("Pause Animation"));
        a->setIcon(Icons::INTERRUPT_SMALL_TOOLBAR.icon());
        break;
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
