// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imageviewer.h"

#include "imageview.h"
#include "imageviewerconstants.h"
#include "imageviewerfile.h"
#include "imageviewertr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMenu>
#include <QSpacerItem>
#include <QToolBar>

using namespace Core;
using namespace Utils;

namespace ImageViewer::Internal{

class ImageViewer : public IEditor
{
    Q_OBJECT

public:
    ImageViewer();
    ~ImageViewer() override;

    Core::IDocument *document() const override;
    QWidget *toolBar() override;

    IEditor *duplicate() override;

    void exportImage();
    void exportMultiImages();
    void copyDataUrl();
    void imageSizeUpdated(const QSize &size);
    void scaleFactorUpdate(qreal factor);

    void switchViewBackground();
    void switchViewOutline();
    void zoomIn();
    void zoomOut();
    void resetToOriginalSize();
    void fitToScreen();
    void updateToolButtons();
    void togglePlay();

private:
    ImageViewer(const QSharedPointer<ImageViewerFile> &document);
    void ctor();
    void playToggled();
    void updatePauseAction();

    QSharedPointer<ImageViewerFile> m_file;
    ImageView *m_imageView;
    QWidget *m_toolbar;

    QToolButton *m_shareButton;
    CommandAction *m_actionExportImage;
    CommandAction *m_actionMultiExportImages;
    CommandAction *m_actionButtonCopyDataUrl;
    CommandAction *m_actionBackground;
    CommandAction *m_actionOutline;
    CommandAction *m_actionFitToScreen;
    CommandAction *m_actionOriginalSize;
    CommandAction *m_actionZoomIn;
    CommandAction *m_actionZoomOut;
    CommandAction *m_actionPlayPause;
    QLabel *m_labelImageSize;
    QLabel *m_labelInfo;
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
{
    m_file.reset(new ImageViewerFile);
    ctor();
}

ImageViewer::ImageViewer(const QSharedPointer<ImageViewerFile> &document)
{
    m_file = document;
    ctor();
}

void ImageViewer::ctor()
{
    m_imageView = new ImageView(m_file.data());
    m_imageView->readSettings(ICore::settings());
    const ImageView::Settings settings = m_imageView->settings();

    setContext(Core::Context(Constants::IMAGEVIEWER_ID));
    setWidget(m_imageView);
    setDuplicateSupported(true);

    // toolbar
    m_toolbar = new StyledBar;

    m_actionExportImage = new CommandAction(Constants::ACTION_EXPORT_IMAGE, m_toolbar);
    m_actionMultiExportImages = new CommandAction(Constants::ACTION_EXPORT_MULTI_IMAGES,
                                                   m_toolbar);
    m_actionButtonCopyDataUrl = new CommandAction(Constants::ACTION_COPY_DATA_URL, m_toolbar);
    m_shareButton = new QToolButton;
    m_shareButton->setToolTip(Tr::tr("Export"));
    m_shareButton->setPopupMode(QToolButton::InstantPopup);
    m_shareButton->setIcon(Icons::EXPORTFILE_TOOLBAR.icon());
    m_shareButton->setProperty(StyleHelper::C_NO_ARROW, true);
    auto shareMenu = new QMenu(m_shareButton);
    shareMenu->addAction(m_actionExportImage);
    shareMenu->addAction(m_actionMultiExportImages);
    shareMenu->addAction(m_actionButtonCopyDataUrl);
    m_shareButton->setMenu(shareMenu);

    m_actionBackground = new CommandAction(Constants::ACTION_BACKGROUND, m_toolbar);
    m_actionOutline = new CommandAction(Constants::ACTION_OUTLINE, m_toolbar);
    m_actionFitToScreen = new CommandAction(Constants::ACTION_FIT_TO_SCREEN, m_toolbar);
    m_actionOriginalSize = new CommandAction(Core::Constants::ZOOM_RESET, m_toolbar);
    m_actionZoomIn = new CommandAction(Core::Constants::ZOOM_IN, m_toolbar);
    m_actionZoomOut = new CommandAction(Core::Constants::ZOOM_OUT, m_toolbar);
    m_actionPlayPause = new CommandAction(Constants::ACTION_TOGGLE_ANIMATION, m_toolbar);

    m_actionBackground->setCheckable(true);
    m_actionBackground->setChecked(settings.showBackground);

    m_actionOutline->setCheckable(true);
    m_actionOutline->setChecked(settings.showOutline);

    m_actionFitToScreen->setCheckable(true);
    m_actionFitToScreen->setChecked(settings.fitToScreen);

    m_actionZoomIn->setAutoRepeat(true);

    m_actionZoomOut->setAutoRepeat(true);

    const Icon backgroundIcon({{":/utils/images/desktopdevicesmall.png", Theme::IconsBaseColor}});
    m_actionBackground->setIcon(backgroundIcon.icon());
    m_actionOutline->setIcon(Icons::BOUNDING_RECT.icon());
    m_actionZoomIn->setIcon(ActionManager::command(Core::Constants::ZOOM_IN)->action()->icon());
    m_actionZoomOut->setIcon(ActionManager::command(Core::Constants::ZOOM_OUT)->action()->icon());
    m_actionOriginalSize->setIcon(
        ActionManager::command(Core::Constants::ZOOM_RESET)->action()->icon());
    m_actionFitToScreen->setIcon(Icons::FITTOVIEW_TOOLBAR.icon());

    // icons update - try to use system theme
    updateIconByTheme(m_actionFitToScreen, QLatin1String("zoom-fit-best"));
    // a display - something is on the background
    updateIconByTheme(m_actionBackground, QLatin1String("video-display"));
    // "emblem to specify the directory where the user stores photographs"
    // (photograph has outline - piece of paper)
    updateIconByTheme(m_actionOutline, QLatin1String("emblem-photos"));

    auto setAsDefault = new QAction(Tr::tr("Set as Default"), m_toolbar);
    const auto updateSetAsDefaultToolTip = [this, setAsDefault] {
        const ImageView::Settings settings = m_imageView->settings();
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

    m_labelImageSize = new QLabel;
    m_labelInfo = new QLabel;

    auto bar = new QToolBar;
    bar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    bar->addWidget(m_shareButton);
    bar->addSeparator();
    bar->addAction(m_actionOriginalSize);
    bar->addAction(m_actionZoomIn);
    bar->addAction(m_actionZoomOut);
    bar->addAction(m_actionPlayPause);
    bar->addAction(m_actionPlayPause);
    bar->addSeparator();
    bar->addAction(m_actionBackground);
    bar->addAction(m_actionOutline);
    bar->addAction(m_actionFitToScreen);
    bar->addAction(setAsDefault);

    auto horizontalLayout = new QHBoxLayout(m_toolbar);
    horizontalLayout->setSpacing(0);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->addWidget(bar);
    horizontalLayout->addItem(
        new QSpacerItem(315, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    horizontalLayout->addWidget(new StyledSeparator);
    horizontalLayout->addWidget(m_labelImageSize);
    horizontalLayout->addWidget(new StyledSeparator);
    horizontalLayout->addWidget(m_labelInfo);

    // connections
    connect(m_actionExportImage, &QAction::triggered, m_imageView, &ImageView::exportImage);
    connect(m_actionMultiExportImages,
            &QAction::triggered,
            m_imageView,
            &ImageView::exportMultiImages);
    connect(m_actionButtonCopyDataUrl, &QAction::triggered, m_imageView, &ImageView::copyDataUrl);
    connect(m_actionZoomIn, &QAction::triggered, m_imageView, &ImageView::zoomIn);
    connect(m_actionZoomOut, &QAction::triggered, m_imageView, &ImageView::zoomOut);
    connect(m_actionFitToScreen, &QAction::triggered, m_imageView, &ImageView::setFitToScreen);
    connect(m_imageView,
            &ImageView::fitToScreenChanged,
            m_actionFitToScreen,
            &QAction::setChecked);
    connect(m_actionOriginalSize,
            &QAction::triggered,
            m_imageView,
            &ImageView::resetToOriginalSize);
    connect(m_actionBackground, &QAction::toggled, m_imageView, &ImageView::setViewBackground);
    connect(m_actionOutline, &QAction::toggled, m_imageView, &ImageView::setViewOutline);
    connect(m_actionPlayPause, &QAction::triggered, this, &ImageViewer::playToggled);
    connect(m_file.data(), &ImageViewerFile::imageSizeChanged,
            this, &ImageViewer::imageSizeUpdated);
    connect(m_file.data(), &ImageViewerFile::openFinished,
            m_imageView, &ImageView::createScene);
    connect(m_file.data(), &ImageViewerFile::openFinished,
            this, &ImageViewer::updateToolButtons);
    connect(m_file.data(), &ImageViewerFile::aboutToReload,
            m_imageView, &ImageView::reset);
    connect(m_file.data(), &ImageViewerFile::reloadFinished,
            m_imageView, &ImageView::createScene);
    connect(m_file.data(), &ImageViewerFile::movieStateChanged,
            this, &ImageViewer::updatePauseAction);
    connect(m_imageView, &ImageView::scaleFactorChanged,
            this, &ImageViewer::scaleFactorUpdate);
    connect(setAsDefault, &QAction::triggered, m_imageView, [this, updateSetAsDefaultToolTip] {
        m_imageView->writeSettings(ICore::settings());
        updateSetAsDefaultToolTip();
    });
}

ImageViewer::~ImageViewer()
{
    delete m_imageView;
    delete m_toolbar;
}

IDocument *ImageViewer::document() const
{
    return m_file.data();
}

QWidget *ImageViewer::toolBar()
{
    return m_toolbar;
}

IEditor *ImageViewer::duplicate()
{
    auto other = new ImageViewer(m_file);
    other->m_imageView->createScene();
    other->updateToolButtons();
    other->m_labelImageSize->setText(m_labelImageSize->text());

    emit editorDuplicated(other);

    return other;
}

void ImageViewer::exportImage()
{
    if (m_file->type() == ImageViewerFile::TypeSvg)
        m_actionExportImage->trigger();
}

void ImageViewer::exportMultiImages()
{
    if (m_file->type() == ImageViewerFile::TypeSvg)
        m_actionMultiExportImages->trigger();
}

void ImageViewer::copyDataUrl()
{
    m_actionButtonCopyDataUrl->trigger();
}

void ImageViewer::imageSizeUpdated(const QSize &size)
{
    QString imageSizeText;
    if (size.isValid())
        imageSizeText = QString::fromLatin1("%1x%2").arg(size.width()).arg(size.height());
    m_labelImageSize->setText(imageSizeText);
}

void ImageViewer::scaleFactorUpdate(qreal factor)
{
    const QString info = QString::number(factor * 100, 'f', 2) + QLatin1Char('%');
    m_labelInfo->setText(info);
}

void ImageViewer::switchViewBackground()
{
    m_actionBackground->trigger();
}

void ImageViewer::switchViewOutline()
{
    m_actionOutline->trigger();
}

void ImageViewer::zoomIn()
{
    m_actionZoomIn->trigger();
}

void ImageViewer::zoomOut()
{
    m_actionZoomOut->trigger();
}

void ImageViewer::resetToOriginalSize()
{
    m_actionOriginalSize->trigger();
}

void ImageViewer::fitToScreen()
{
    m_actionFitToScreen->trigger();
}

void ImageViewer::updateToolButtons()
{
    const bool isSvg = m_file->type() == ImageViewerFile::TypeSvg;
    m_actionExportImage->setEnabled(isSvg);
    m_actionMultiExportImages->setEnabled(isSvg);
    updatePauseAction();
}

void ImageViewer::togglePlay()
{
    m_actionPlayPause->trigger();
}

void ImageViewer::playToggled()
{
    QMovie *m = m_file->movie();
    if (!m)
        return;
    const QMovie::MovieState state = m_file->movie()->state();
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
    const bool isMovie = m_file->type() == ImageViewerFile::TypeMovie;
    const QMovie::MovieState state = isMovie ? m_file->movie()->state() : QMovie::NotRunning;
    CommandAction *a = m_actionPlayPause;
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

class ImageViewerFactory final : public IEditorFactory
{
public:
    ImageViewerFactory()
    {
        setId(Constants::IMAGEVIEWER_ID);
        setDisplayName(Tr::tr("Image Viewer"));
        setEditorCreator([] { return new ImageViewer; });

        const QList<QByteArray> supportedMimeTypes = QImageReader::supportedMimeTypes();
        for (const QByteArray &format : supportedMimeTypes)
            addMimeType(QString::fromLatin1(format));
    }
};

static void createAction(QObject *guard, Id id,
                         const std::function<void(ImageViewer *v)> &onTriggered,
                         const QString &title = {},
                         const QKeySequence &key = {})
{
    ActionBuilder builder(guard, id);
    builder.setText(title);
    builder.setContext(Context(Constants::IMAGEVIEWER_ID));
    if (!key.isEmpty())
        builder.setDefaultKeySequence(key);

    builder.setOnTriggered(guard, [onTriggered] {
        if (auto iv = qobject_cast<ImageViewer *>(EditorManager::currentEditor()))
            onTriggered(iv);
    });
}

void setupImageViewer(QObject *guard)
{
    static ImageViewerFactory theImageViewerFactory;

    createAction(guard, Core::Constants::ZOOM_IN, &ImageViewer::zoomIn);

    createAction(guard, Core::Constants::ZOOM_OUT, &ImageViewer::zoomOut);

    createAction(guard, Core::Constants::ZOOM_RESET, &ImageViewer::resetToOriginalSize);

    createAction(guard, Constants::ACTION_FIT_TO_SCREEN, &ImageViewer::fitToScreen,
                 Tr::tr("Fit to Screen"), Tr::tr("Ctrl+="));

    createAction( guard, Constants::ACTION_BACKGROUND, &ImageViewer::switchViewBackground,
                 Tr::tr("Switch Background"), Tr::tr("Ctrl+["));

    createAction(guard, Constants::ACTION_OUTLINE, &ImageViewer::switchViewOutline,
                 Tr::tr("Switch Outline"), Tr::tr("Ctrl+]"));

    createAction(guard, Constants::ACTION_TOGGLE_ANIMATION, &ImageViewer::togglePlay,
                 Tr::tr("Toggle Animation"));

    createAction(guard, Constants::ACTION_EXPORT_IMAGE, &ImageViewer::exportImage,
                 Tr::tr("Export Image"));

    createAction(guard, Constants::ACTION_EXPORT_MULTI_IMAGES, &ImageViewer::exportMultiImages,
                 Tr::tr("Export Multiple Images"));

    createAction(guard, Constants::ACTION_COPY_DATA_URL, &ImageViewer::copyDataUrl,
                 Tr::tr("Copy as Data URL"));
}

} // ImageViewer::Internal

#include "imageviewer.moc"
