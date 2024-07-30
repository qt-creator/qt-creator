// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "splashscreencontainerwidget.h"

#include "androidtr.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/fileutils.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QPainter>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace Utils;

namespace Android::Internal {

const char extraExtraExtraHighDpiImagePath[] = "/res/drawable-xxxhdpi/";
const char extraExtraHighDpiImagePath[] = "/res/drawable-xxhdpi/";
const char extraHighDpiImagePath[] = "/res/drawable-xhdpi/";
const char highDpiImagePath[] = "/res/drawable-hdpi/";
const char mediumDpiImagePath[] = "/res/drawable-mdpi/";
const char lowDpiImagePath[] = "/res/drawable-ldpi/";
const char splashscreenName[] = "splashscreen";
const char splashscreenPortraitName[] = "splashscreen_port";
const char splashscreenLandscapeName[] = "splashscreen_land";
const char splashscreenFileName[] = "logo";
const char splashscreenPortraitFileName[] = "logo_port";
const char splashscreenLandscapeFileName[] = "logo_land";
const char imageSuffix[] = ".png";
const QString fileDialogImageFiles = Tr::tr("Images (*.png *.jpg *.jpeg)"); // TODO: Implement a centralized images filter string
const QSize lowDpiImageSize{200, 320};
const QSize mediumDpiImageSize{320, 480};
const QSize highDpiImageSize{480, 800};
const QSize extraHighDpiImageSize{720, 1280};
const QSize extraExtraHighDpiImageSize{960, 1600};
const QSize extraExtraExtraHighDpiImageSize{1280, 1920};
const QSize displaySize{48, 72};
const QSize landscapeDisplaySize{72, 48};
// https://developer.android.com/training/multiscreen/screendensities#TaskProvideAltBmp
const int extraExtraExtraHighDpiScalingRatio = 16;
const int extraExtraHighDpiScalingRatio = 12;
const int extraHighDpiScalingRatio = 8;
const int highDpiScalingRatio = 6;
const int mediumDpiScalingRatio = 4;
const int lowDpiScalingRatio = 3;

static FilePath manifestDir(TextEditor::TextEditorWidget *textEditorWidget)
{
    // Get the manifest file's directory from its filepath.
    return textEditorWidget->textDocument()->filePath().absolutePath();
}

static Q_LOGGING_CATEGORY(androidManifestEditorLog, "qtc.android.splashScreenWidget", QtWarningMsg);

class SplashScreenWidget : public QWidget
{
    Q_OBJECT

    class SplashScreenButton : public QToolButton
    {
    public:
        explicit SplashScreenButton(SplashScreenWidget *parent)
            : QToolButton(parent),
              m_parentWidget(parent)
        {}

    private:
        void paintEvent(QPaintEvent *event) override
        {
            Q_UNUSED(event);
            QPainter painter(this);
            painter.setPen(QPen(Qt::gray, 1));
            painter.setBrush(QBrush(m_parentWidget->m_backgroundColor));
            painter.drawRect(0, 0, width()-1, height()-1);
            if (!m_parentWidget->m_image.isNull())
                painter.drawImage(m_parentWidget->m_imagePosition, m_parentWidget->m_image);
        }

        SplashScreenWidget *m_parentWidget;
    };

public:
    explicit SplashScreenWidget(QWidget *parent) : QWidget(parent) {}
    SplashScreenWidget(QWidget *parent,
                       const QSize &size,
                       const QSize &screenSize,
                       const QString &title,
                       const QString &tooltip,
                       const QString &imagePath,
                       int scalingRatio, int maxScalingRatio,
                       TextEditor::TextEditorWidget *textEditorWidget = nullptr);

    bool hasImage() const;
    void clearImage();
    void setBackgroundColor(const QColor &backgroundColor);
    void showImageFullScreen(bool fullScreen);
    void setImageFromPath(const FilePath &imagePath, bool resize = true);
    void setImageFileName(const QString &imageFileName);
    void loadImage();

signals:
    void imageChanged();

private:
    void selectImage();
    void removeImage();
    void setScaleWarningLabelVisible(bool visible);

private:
    TextEditor::TextEditorWidget *m_textEditorWidget = nullptr;
    QLabel *m_scaleWarningLabel = nullptr;
    SplashScreenButton *m_splashScreenButton = nullptr;
    int m_scalingRatio, m_maxScalingRatio;
    QColor m_backgroundColor = Qt::white;
    QImage m_image;
    QPoint m_imagePosition;
    QString m_imagePath;
    QString m_imageFileName;
    QString m_imageSelectionText;
    QSize m_screenSize;
    bool m_showImageFullScreen = false;
};

SplashScreenWidget::SplashScreenWidget(
        QWidget *parent,
        const QSize &size, const QSize &screenSize,
        const QString &title, const QString &tooltip,
        const QString &imagePath,
        int scalingRatio, int maxScalingRatio,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QWidget(parent),
      m_textEditorWidget(textEditorWidget),
      m_scalingRatio(scalingRatio),
      m_maxScalingRatio(maxScalingRatio),
      m_imagePath(imagePath),
      m_screenSize(screenSize)
{
    auto splashLayout = new QVBoxLayout(this);
    auto splashTitle = new QLabel(title, this);
    auto splashButtonLayout = new QGridLayout();
    m_splashScreenButton = new SplashScreenButton(this);
    m_splashScreenButton->setMinimumSize(size);
    m_splashScreenButton->setMaximumSize(size);
    m_splashScreenButton->setToolTip(tooltip);
    QSize clearAndWarningSize(16, 16);
    QToolButton *clearButton = nullptr;
    if (textEditorWidget) {
        clearButton = new QToolButton(this);
        clearButton->setMinimumSize(clearAndWarningSize);
        clearButton->setMaximumSize(clearAndWarningSize);
        clearButton->setIcon(Utils::Icons::CLOSE_FOREGROUND.icon());
    }
    if (textEditorWidget) {
        m_scaleWarningLabel = new QLabel(this);
        m_scaleWarningLabel->setMinimumSize(clearAndWarningSize);
        m_scaleWarningLabel->setMaximumSize(clearAndWarningSize);
        m_scaleWarningLabel->setPixmap(Utils::Icons::WARNING.icon().pixmap(clearAndWarningSize));
        m_scaleWarningLabel->setToolTip(Tr::tr("Icon scaled up."));
        m_scaleWarningLabel->setVisible(false);
    }
    auto label = new QLabel(Tr::tr("Click to select..."), parent);
    splashLayout->addWidget(splashTitle);
    splashLayout->setAlignment(splashTitle, Qt::AlignHCenter);
    splashButtonLayout->setColumnMinimumWidth(0, 16);
    splashButtonLayout->addWidget(m_splashScreenButton, 0, 1, 1, 3);
    splashButtonLayout->setAlignment(m_splashScreenButton, Qt::AlignVCenter);
    if (textEditorWidget) {
        splashButtonLayout->addWidget(clearButton, 0, 4, 1, 1);
        splashButtonLayout->setAlignment(clearButton, Qt::AlignTop);
    }
    if (textEditorWidget) {
        splashButtonLayout->addWidget(m_scaleWarningLabel, 0, 0, 1, 1);
        splashButtonLayout->setAlignment(m_scaleWarningLabel, Qt::AlignTop);
    }
    splashLayout->addLayout(splashButtonLayout);
    splashLayout->setAlignment(splashButtonLayout, Qt::AlignHCenter);
    splashLayout->addWidget(label);
    splashLayout->setAlignment(label, Qt::AlignHCenter);
    this->setLayout(splashLayout);
    connect(m_splashScreenButton, &QAbstractButton::clicked,
            this, &SplashScreenWidget::selectImage);
    if (clearButton)
        connect(clearButton, &QAbstractButton::clicked,
                this, &SplashScreenWidget::removeImage);
    m_imageSelectionText = tooltip;
}

void SplashScreenWidget::setBackgroundColor(const QColor &backgroundColor)
{
    m_backgroundColor = backgroundColor;
    m_splashScreenButton->update();
    emit imageChanged();
}

void SplashScreenWidget::showImageFullScreen(bool fullScreen)
{
    m_showImageFullScreen = fullScreen;
    loadImage();
}

void SplashScreenWidget::setImageFromPath(const FilePath &imagePath, bool resize)
{
    if (!m_textEditorWidget)
        return;
    const FilePath baseDir = manifestDir(m_textEditorWidget);
    const FilePath targetPath = baseDir / m_imagePath / m_imageFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image target path is empty, cannot set image.";
        return;
    }
    QImage image = QImage(imagePath.toString());
    if (image.isNull()) {
        qCDebug(androidManifestEditorLog) << "Image '" << imagePath << "' not found or invalid format.";
        return;
    }
    if (!targetPath.absolutePath().ensureWritableDir()) {
        qCDebug(androidManifestEditorLog) << "Cannot create image target path.";
        return;
    }
    if (resize == true && m_scalingRatio < m_maxScalingRatio) {
        const QSize size = QSize((float(image.width()) / float(m_maxScalingRatio)) * float(m_scalingRatio),
                                 (float(image.height()) / float(m_maxScalingRatio)) * float(m_scalingRatio));
        image = image.scaled(size);
    }
    QFile file(targetPath.toString());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        image.save(&file, "PNG");
        file.close();
        loadImage();
        emit imageChanged();
    }
    else {
        qCDebug(androidManifestEditorLog).noquote()
                << "Cannot save image." << targetPath.toUserOutput();
    }
}

void SplashScreenWidget::selectImage()
{
    const FilePath file = FileUtils::getOpenFilePath(this, m_imageSelectionText,
                                                     FileUtils::homePath(),
                                                     QStringLiteral("%1 (*.png *.jpg *.jpeg)")
                                                     .arg(Tr::tr("Images")));
    if (file.isEmpty())
        return;
    setImageFromPath(file, false);
    emit imageChanged();
}

void SplashScreenWidget::removeImage()
{
    const FilePath baseDir = manifestDir(m_textEditorWidget);
    const FilePath targetPath = baseDir / m_imagePath / m_imageFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image target path empty, cannot remove image.";
        return;
    }
    targetPath.removeFile();
    m_image = QImage();
    m_splashScreenButton->update();
    setScaleWarningLabelVisible(false);
}

void SplashScreenWidget::clearImage()
{
    removeImage();
    emit imageChanged();
}

void SplashScreenWidget::loadImage()
{
    if (m_imageFileName.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image name not set, cannot load image.";
        return;
    }
    const FilePath baseDir = manifestDir(m_textEditorWidget);
    const FilePath targetPath = baseDir / m_imagePath / m_imageFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image target path empty, cannot load image.";
        return;
    }
    QImage image = QImage(targetPath.toString());
    if (image.isNull()) {
        qCDebug(androidManifestEditorLog).noquote()
                << "Cannot load image." << targetPath.toUserOutput();
        return;
    }
    if (m_showImageFullScreen) {
        m_image = image.scaled(m_splashScreenButton->size());
        m_imagePosition = QPoint(0,0);
    }
    else {
        QSize scaledSize = QSize((m_splashScreenButton->width() * image.width()) / m_screenSize.width(),
                                 (m_splashScreenButton->height() * image.height()) / m_screenSize.height());
        m_image = image.scaled(scaledSize, Qt::KeepAspectRatio);
        m_imagePosition = QPoint((m_splashScreenButton->width() - m_image.width()) / 2,
                                 (m_splashScreenButton->height() - m_image.height()) / 2);
    }
    m_splashScreenButton->update();
}

bool SplashScreenWidget::hasImage() const
{
    return !m_image.isNull();
}

void SplashScreenWidget::setScaleWarningLabelVisible(bool visible)
{
    if (m_scaleWarningLabel)
        m_scaleWarningLabel->setVisible(visible);
}

void SplashScreenWidget::setImageFileName(const QString &imageFileName)
{
    m_imageFileName = imageFileName;
}

static SplashScreenWidget *addWidgetToPage(QWidget *page,
                                           const QSize &size, const QSize &screenSize,
                                           const QString &title, const QString &tooltip,
                                           TextEditor::TextEditorWidget *textEditorWidget,
                                           const QString &splashScreenPath,
                                           int scalingRatio, int maxScalingRatio,
                                           QHBoxLayout *pageLayout,
                                           QList<SplashScreenWidget *> &widgetContainer)
{
    auto splashScreenWidget = new SplashScreenWidget(page,
                                                     size,
                                                     screenSize,
                                                     title,
                                                     tooltip,
                                                     splashScreenPath,
                                                     scalingRatio,
                                                     maxScalingRatio,
                                                     textEditorWidget);
    pageLayout->addWidget(splashScreenWidget);
    widgetContainer.push_back(splashScreenWidget);
    return splashScreenWidget;
}

static QWidget *createPage(TextEditor::TextEditorWidget *textEditorWidget,
                           QList<SplashScreenWidget *> &widgetContainer,
                           QList<SplashScreenWidget *> &portraitWidgetContainer,
                           QList<SplashScreenWidget *> &landscapeWidgetContainer,
                           int scalingRatio,
                           const QSize &size,
                           const QSize &portraitSize,
                           const QSize &landscapeSize,
                           const QString &path)
{
    auto sizeToStr = [](const QSize &size) { return QString(" (%1x%2)").arg(size.width()).arg(size.height()); };
    QWidget *page = new QWidget();
    auto pageLayout = new QHBoxLayout(page);
    auto genericWidget= addWidgetToPage(page,
                                        displaySize, size,
                                        Tr::tr("Splash screen"),
                                        Tr::tr("Select splash screen image")
                                        + sizeToStr(size),
                                        textEditorWidget,
                                        path,
                                        scalingRatio, extraExtraExtraHighDpiScalingRatio,
                                        pageLayout,
                                        widgetContainer);

    auto portraitWidget = addWidgetToPage(page,
                                          displaySize, portraitSize,
                                          Tr::tr("Portrait splash screen"),
                                          Tr::tr("Select portrait splash screen image")
                                          + sizeToStr(portraitSize),
                                          textEditorWidget,
                                          path,
                                          scalingRatio, extraExtraExtraHighDpiScalingRatio,
                                          pageLayout,
                                          portraitWidgetContainer);

    auto landscapeWidget = addWidgetToPage(page,
                                           landscapeDisplaySize, landscapeSize,
                                           Tr::tr("Landscape splash screen"),
                                           Tr::tr("Select landscape splash screen image")
                                           + sizeToStr(landscapeSize),
                                           textEditorWidget,
                                           path,
                                           scalingRatio, extraExtraExtraHighDpiScalingRatio,
                                           pageLayout,
                                           landscapeWidgetContainer);

    auto clearButton = new QToolButton(page);
    clearButton->setText(Tr::tr("Clear All"));
    pageLayout->addWidget(clearButton);
    pageLayout->setAlignment(clearButton, Qt::AlignVCenter);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       genericWidget, &SplashScreenWidget::clearImage);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       portraitWidget, &SplashScreenWidget::clearImage);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       landscapeWidget, &SplashScreenWidget::clearImage);
    return page;
}


SplashScreenContainerWidget::SplashScreenContainerWidget(
        QWidget *parent,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QStackedWidget(parent),
      m_textEditorWidget(textEditorWidget)
{
    auto noSplashscreenWidget = new QWidget(this);
    auto splashscreenWidget = new QWidget(this);
    auto layout = new QHBoxLayout(this);
    auto settingsLayout = new QVBoxLayout(this);
    auto noSplashscreenLayout = new QVBoxLayout(this);
    auto formLayout = new QFormLayout(this);
    QTabWidget *tab = new QTabWidget(this);

    m_stickyCheck = new QCheckBox(this);
    m_stickyCheck->setToolTip(Tr::tr("A non-sticky splash screen is hidden automatically when an activity is drawn.\n"
                                     "To hide a sticky splash screen, invoke QtAndroid::hideSplashScreen()."));
    formLayout->addRow(Tr::tr("Sticky splash screen:"), m_stickyCheck);

    m_imageShowMode = new QComboBox(this);
    formLayout->addRow(Tr::tr("Image show mode:"), m_imageShowMode);
    const QList<QStringList> imageShowModeMethodsMap = {
        {"center", "Place the object in the center of the screen in both the vertical and horizontal axis,\n"
                   "not changing its size."},
        {"fill", "Grow the horizontal and vertical size of the image if needed so it completely fills its screen."}};
    for (int i = 0; i < imageShowModeMethodsMap.size(); ++i) {
        m_imageShowMode->addItem(imageShowModeMethodsMap.at(i).first());
        m_imageShowMode->setItemData(i, imageShowModeMethodsMap.at(i).at(1), Qt::ToolTipRole);
    }

    m_backgroundColor = new QToolButton(this);
    m_backgroundColor->setToolTip(Tr::tr("Background color of the splash screen."));
    formLayout->addRow(Tr::tr("Background color:"), m_backgroundColor);

    m_masterImage = new QToolButton(this);
    m_masterImage->setToolTip(Tr::tr("Select master image to use."));
    m_masterImage->setIcon(Icon::fromTheme("document-open"));
    formLayout->addRow(Tr::tr("Master image:"), m_masterImage);

    m_portraitMasterImage = new QToolButton(this);
    m_portraitMasterImage->setToolTip(Tr::tr("Select portrait master image to use."));
    m_portraitMasterImage->setIcon(Icon::fromTheme("document-open"));
    formLayout->addRow(Tr::tr("Portrait master image:"), m_portraitMasterImage);

    m_landscapeMasterImage = new QToolButton(this);
    m_landscapeMasterImage->setToolTip(Tr::tr("Select landscape master image to use."));
    m_landscapeMasterImage->setIcon(Icon::fromTheme("document-open"));
    formLayout->addRow(Tr::tr("Landscape master image:"), m_landscapeMasterImage);

    auto clearAllButton = new QToolButton(this);
    clearAllButton->setText(Tr::tr("Clear All"));

    auto ldpiPage = createPage(textEditorWidget,
                               m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                               lowDpiScalingRatio,
                               lowDpiImageSize,
                               lowDpiImageSize,
                               lowDpiImageSize.transposed(),
                               lowDpiImagePath);
    tab->addTab(ldpiPage, Tr::tr("LDPI"));
    auto mdpiPage = createPage(textEditorWidget,
                               m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                               mediumDpiScalingRatio,
                               mediumDpiImageSize,
                               mediumDpiImageSize,
                               mediumDpiImageSize.transposed(),
                               mediumDpiImagePath);
    tab->addTab(mdpiPage, Tr::tr("MDPI"));
    auto hdpiPage = createPage(textEditorWidget,
                               m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                               highDpiScalingRatio,
                               highDpiImageSize,
                               highDpiImageSize,
                               highDpiImageSize.transposed(),
                               highDpiImagePath);
    tab->addTab(hdpiPage, Tr::tr("HDPI"));
    auto xHdpiPage = createPage(textEditorWidget,
                                m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                                extraHighDpiScalingRatio,
                                extraHighDpiImageSize,
                                extraHighDpiImageSize,
                                extraHighDpiImageSize.transposed(),
                                extraHighDpiImagePath);
    tab->addTab(xHdpiPage, Tr::tr("XHDPI"));
    auto xxHdpiPage = createPage(textEditorWidget,
                                 m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                                 extraExtraHighDpiScalingRatio,
                                 extraExtraHighDpiImageSize,
                                 extraExtraHighDpiImageSize,
                                 extraExtraHighDpiImageSize.transposed(),
                                 extraExtraHighDpiImagePath);
    tab->addTab(xxHdpiPage, Tr::tr("XXHDPI"));
    auto xxxHdpiPage = createPage(textEditorWidget,
                                  m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                                  extraExtraExtraHighDpiScalingRatio,
                                  extraExtraExtraHighDpiImageSize,
                                  extraExtraExtraHighDpiImageSize,
                                  extraExtraExtraHighDpiImageSize.transposed(),
                                  extraExtraExtraHighDpiImagePath);
    tab->addTab(xxxHdpiPage, Tr::tr("XXXHDPI"));
    formLayout->setContentsMargins(10, 10, 10, 10);
    formLayout->setSpacing(10);
    settingsLayout->addLayout(formLayout);
    settingsLayout->addWidget(clearAllButton);
    settingsLayout->setAlignment(clearAllButton, Qt::AlignHCenter);
    layout->addLayout(settingsLayout);
    layout->addWidget(tab);
    splashscreenWidget->setLayout(layout);
    addWidget(splashscreenWidget);
    setBackgroundColor(Qt::white);

    auto warningLabel = new QLabel(this);
    warningLabel->setAlignment(Qt::AlignHCenter);
    warningLabel->setText(Tr::tr("An image is used for the splashscreen. Qt Creator manages\n"
                                 "splashscreen by using a different method which requires changing\n"
                                 "the manifest file by overriding your settings. Allow override?"));
    m_convertSplashscreen = new QToolButton(this);
    m_convertSplashscreen->setText(Tr::tr("Convert"));
    noSplashscreenLayout->addStretch();
    noSplashscreenLayout->addWidget(warningLabel);
    noSplashscreenLayout->addWidget(m_convertSplashscreen);
    noSplashscreenLayout->addStretch();
    noSplashscreenLayout->setSpacing(10);
    noSplashscreenLayout->setAlignment(warningLabel, Qt::AlignHCenter);
    noSplashscreenLayout->setAlignment(m_convertSplashscreen, Qt::AlignHCenter);
    noSplashscreenWidget->setLayout(noSplashscreenLayout);
    addWidget(noSplashscreenWidget);

    const auto splashFileName = QString(splashscreenFileName).append(imageSuffix);
    const auto portraitSplashFileName = QString(splashscreenPortraitFileName).append(imageSuffix);
    const auto landscapeSplashFileName = QString(splashscreenLandscapeFileName).append(imageSuffix);

    for (auto &&imageWidget : m_imageWidgets)
        imageWidget->setImageFileName(splashFileName);
    for (auto &&imageWidget : m_portraitImageWidgets)
        imageWidget->setImageFileName(portraitSplashFileName);
    for (auto &&imageWidget : m_landscapeImageWidgets)
        imageWidget->setImageFileName(landscapeSplashFileName);

    for (auto &&imageWidget : m_imageWidgets) {
        connect(imageWidget, &SplashScreenWidget::imageChanged, this, [this] {
            createSplashscreenThemes();
            emit splashScreensModified();
        });
    }
    for (auto &&imageWidget : m_portraitImageWidgets) {
        connect(imageWidget, &SplashScreenWidget::imageChanged, this, [this] {
            createSplashscreenThemes();
            emit splashScreensModified();
        });
    }
    for (auto &&imageWidget : m_landscapeImageWidgets) {
        connect(imageWidget, &SplashScreenWidget::imageChanged, this, [this] {
            createSplashscreenThemes();
            emit splashScreensModified();
        });
    }
    connect(m_stickyCheck, &QCheckBox::stateChanged, this, [this](int state) {
        bool old = m_splashScreenSticky;
        m_splashScreenSticky = (state == Qt::Checked);
        if (old != m_splashScreenSticky)
            emit splashScreensModified();
    });
    connect(m_backgroundColor, &QToolButton::clicked, this, [this] {
        const QColor color = QColorDialog::getColor(m_splashScreenBackgroundColor,
                                                    this,
                                                    Tr::tr("Select background color"));
        if (color.isValid()) {
            setBackgroundColor(color);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_masterImage, &QToolButton::clicked, this, [this] {
        const FilePath file = FileUtils::getOpenFilePath(this, Tr::tr("Select master image"),
                                                         FileUtils::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_imageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_portraitMasterImage, &QToolButton::clicked, this, [this] {
        const FilePath file = FileUtils::getOpenFilePath(this, Tr::tr("Select portrait master image"),
                                                         FileUtils::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_portraitImageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_landscapeMasterImage, &QToolButton::clicked, this, [this] {
        const FilePath file = FileUtils::getOpenFilePath(this, Tr::tr("Select landscape master image"),
                                                         FileUtils::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_landscapeImageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_imageShowMode, &QComboBox::currentTextChanged, this, [this](const QString &mode) {
        setImageShowMode(mode);
        createSplashscreenThemes();
        emit splashScreensModified();
    });
    connect(clearAllButton, &QToolButton::clicked, this, [this] {
        clearAll();
        emit splashScreensModified();
    });
    connect(m_convertSplashscreen, &QToolButton::clicked, this, [this] {
        clearAll();
        setCurrentIndex(0);
        emit splashScreensModified();
    });
}

void SplashScreenContainerWidget::loadImages()
{
    if (isSplashscreenEnabled()) {
        for (auto &&imageWidget : m_imageWidgets) {
            imageWidget->loadImage();
        }
        loadSplashscreenDrawParams(splashscreenName);
        for (auto &&imageWidget : m_portraitImageWidgets) {
            imageWidget->loadImage();
        }
        loadSplashscreenDrawParams(splashscreenPortraitName);
        for (auto &&imageWidget : m_landscapeImageWidgets) {
            imageWidget->loadImage();
        }
        loadSplashscreenDrawParams(splashscreenLandscapeName);
    }
}

void SplashScreenContainerWidget::loadSplashscreenDrawParams(const QString &name)
{
    const FilePath filePath = manifestDir(m_textEditorWidget).pathAppended("res/drawable/" + name + ".xml");
    QFile file(filePath.toString());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QXmlStreamReader reader(&file);
        reader.setNamespaceProcessing(false);
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.hasError()) {
                // This should not happen
                return;
            } else {
                if (reader.name() == QLatin1String("solid")) {
                    const QXmlStreamAttributes attributes = reader.attributes();
                    const auto color = attributes.value(QLatin1String("android:color"));
                    if (!color.isEmpty())
                        setBackgroundColor(QColor(color));
                }
                else if (reader.name() == QLatin1String("bitmap")) {
                    const QXmlStreamAttributes attributes = reader.attributes();
                    const auto showMode = attributes.value(QLatin1String("android:gravity"));
                    if (!showMode.isEmpty())
                        setImageShowMode(showMode.toString());
                }
            }
        }
    }
}

void SplashScreenContainerWidget::clearAll()
{
    for (auto &&imageWidget : m_imageWidgets) {
        imageWidget->clearImage();
    }
    for (auto &&imageWidget : m_portraitImageWidgets) {
        imageWidget->clearImage();
    }
    for (auto &&imageWidget : m_landscapeImageWidgets) {
        imageWidget->clearImage();
    }
    setBackgroundColor(Qt::white);
    createSplashscreenThemes();
}

bool SplashScreenContainerWidget::hasImages() const
{
    for (auto &&imageWidget : m_imageWidgets) {
        if (imageWidget->hasImage())
            return true;
    }
    return false;
}

bool SplashScreenContainerWidget::hasPortraitImages() const
{
    for (auto &&imageWidget : m_portraitImageWidgets) {
        if (imageWidget->hasImage())
            return true;
    }
    return false;
}

bool SplashScreenContainerWidget::hasLandscapeImages() const
{
    for (auto &&imageWidget : m_landscapeImageWidgets) {
        if (imageWidget->hasImage())
            return true;
    }
    return false;
}

bool SplashScreenContainerWidget::isSticky() const
{
    return m_splashScreenSticky;
}

void SplashScreenContainerWidget::setSticky(bool sticky)
{
    m_splashScreenSticky = sticky;
    m_stickyCheck->setCheckState(m_splashScreenSticky ? Qt::Checked : Qt::Unchecked);
}

void SplashScreenContainerWidget::setBackgroundColor(const QColor &color)
{
    if (color != m_splashScreenBackgroundColor) {
        m_splashScreenBackgroundColor = color;
        m_backgroundColor->setStyleSheet(QString("QToolButton {background-color: %1; border: 1px solid gray;}").arg(color.name()));

        for (auto &&imageWidget : m_imageWidgets)
            imageWidget->setBackgroundColor(color);
        for (auto &&imageWidget : m_portraitImageWidgets)
            imageWidget->setBackgroundColor(color);
        for (auto &&imageWidget : m_landscapeImageWidgets)
            imageWidget->setBackgroundColor(color);
    }
}

void SplashScreenContainerWidget::setImageShowMode(const QString &mode)
{
    bool imageFullScreen;

    if (mode == "center")
        imageFullScreen = false;
    else if (mode == "fill")
        imageFullScreen = true;
    else
        return;

    for (auto &&imageWidget : m_imageWidgets)
        imageWidget->showImageFullScreen(imageFullScreen);
    for (auto &&imageWidget : m_portraitImageWidgets)
        imageWidget->showImageFullScreen(imageFullScreen);
    for (auto &&imageWidget : m_landscapeImageWidgets)
        imageWidget->showImageFullScreen(imageFullScreen);

    m_imageShowMode->setCurrentText(mode);
}

void SplashScreenContainerWidget::createSplashscreenThemes()
{
    const QString baseDir = manifestDir(m_textEditorWidget).toString();
    const QStringList splashscreenThemeFiles = {"/res/values/splashscreentheme.xml",
                                                "/res/values-port/splashscreentheme.xml",
                                                "/res/values-land/splashscreentheme.xml"};
    const QStringList splashscreenDrawableFiles = {QString("/res/drawable/%1.xml").arg(splashscreenName),
                                                   QString("/res/drawable/%1.xml").arg(splashscreenPortraitName),
                                                   QString("/res/drawable/%1.xml").arg(splashscreenLandscapeName)};
    QStringList splashscreens[3];

    if (hasImages())
        splashscreens[0] << splashscreenName << splashscreenFileName;
    if (hasPortraitImages())
        splashscreens[1] << splashscreenPortraitName << splashscreenPortraitFileName;
    if (hasLandscapeImages())
        splashscreens[2] << splashscreenLandscapeName << splashscreenLandscapeFileName;

    for (int i = 0; i < 3; i++) {
        if (!splashscreens[i].isEmpty()) {
            QDir dir;
            QFile themeFile(baseDir + splashscreenThemeFiles[i]);
            dir.mkpath(QFileInfo(themeFile).absolutePath());
            if (themeFile.open(QFile::WriteOnly | QFile::Truncate)) {
                QXmlStreamWriter stream(&themeFile);
                stream.setAutoFormatting(true);
                stream.writeStartDocument();
                stream.writeStartElement("resources");
                stream.writeStartElement("style");
                stream.writeAttribute("name", "splashScreenTheme");
                stream.writeStartElement("item");
                stream.writeAttribute("name", "android:windowBackground");
                stream.writeCharacters(QLatin1String("@drawable/%1").arg(splashscreens[i].at(0)));
                stream.writeEndElement(); // item
                stream.writeEndElement(); // style
                stream.writeEndElement(); // resources
                stream.writeEndDocument();
                themeFile.close();
            }
            QFile drawableFile(baseDir + splashscreenDrawableFiles[i]);
            dir.mkpath(QFileInfo(drawableFile).absolutePath());
            if (drawableFile.open(QFile::WriteOnly | QFile::Truncate)) {
                QXmlStreamWriter stream(&drawableFile);
                stream.setAutoFormatting(true);
                stream.writeStartDocument();
                stream.writeStartElement("layer-list");
                stream.writeAttribute("xmlns:android", "http://schemas.android.com/apk/res/android");
                stream.writeStartElement("item");
                stream.writeStartElement("shape");
                stream.writeAttribute("android:shape", "rectangle");
                stream.writeEmptyElement("solid");
                stream.writeAttribute("android:color", m_splashScreenBackgroundColor.name());
                stream.writeEndElement(); // shape
                stream.writeEndElement(); // item
                stream.writeStartElement("item");
                stream.writeEmptyElement("bitmap");
                stream.writeAttribute("android:src", QLatin1String("@drawable/%1").arg(splashscreens[i].at(1)));
                stream.writeAttribute("android:gravity", m_imageShowMode->currentText());
                stream.writeEndElement(); // item
                stream.writeEndElement(); // layer-list
                stream.writeEndDocument();
                drawableFile.close();
            }
        }
        else {
            QFile::remove(baseDir + splashscreenThemeFiles[i]);
            QFile::remove(baseDir + splashscreenDrawableFiles[i]);
        }
    }
}

void SplashScreenContainerWidget::checkSplashscreenImage(const QString &name)
{
    if (isSplashscreenEnabled()) {
        const FilePath baseDir = manifestDir(m_textEditorWidget);
        const QString paths[] = {extraExtraExtraHighDpiImagePath,
                                 extraExtraHighDpiImagePath,
                                 extraHighDpiImagePath,
                                 highDpiImagePath,
                                 mediumDpiImagePath,
                                 lowDpiImagePath};

        for (const QString &path : paths) {
            const FilePath filePath = baseDir.pathAppended(path + name);
            if (filePath.stringAppended(".png").exists() || filePath.stringAppended(".jpg").exists()
                    || filePath.stringAppended(".jpeg").exists()) {
                setCurrentIndex(1);
                break;
            }
        }
    }
}

bool SplashScreenContainerWidget::isSplashscreenEnabled()
{
    return (currentIndex() == 0);
}

QString SplashScreenContainerWidget::imageName() const
{
    return splashscreenName;
}

QString SplashScreenContainerWidget::portraitImageName() const
{
    return splashscreenPortraitName;
}

QString SplashScreenContainerWidget::landscapeImageName() const
{
    return splashscreenLandscapeName;
}

} // namespace Android::Internal

#include "splashscreencontainerwidget.moc"
