// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "splashscreencontainerwidget.h"
#include "androidtoolmenu.h"
#include "androidtr.h"
#include "androidmanifestutils.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/fileutils.h>
#include <utils/utilsicons.h>

#include <projectexplorer/projectmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDomDocument>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QPainter>
#include <QTabWidget>
#include <QToolButton>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDirIterator>

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

// https://developer.android.com/training/multiscreen/screendensities#TaskProvideAltBmp
const int extraExtraExtraHighDpiScalingRatio = 16;
const int extraExtraHighDpiScalingRatio = 12;
const int extraHighDpiScalingRatio = 8;
const int highDpiScalingRatio = 6;
const int mediumDpiScalingRatio = 4;
const int lowDpiScalingRatio = 3;

using ProjectExplorer::Project;
using ProjectExplorer::ProjectManager;

static Q_LOGGING_CATEGORY(androidManifestEditorLog, "qtc.android.splashScreenWidget", QtWarningMsg);

class SplashScreenWidget : public QWidget
{
    Q_OBJECT

    class SplashScreenButton : public QToolButton
    {
    public:
        explicit SplashScreenButton(SplashScreenWidget *parent)
            : QToolButton(parent), m_parentWidget(parent) {}

    private:
        void paintEvent(QPaintEvent *event) override
        {
            Q_UNUSED(event)
            QPainter painter(this);
            painter.setPen(QPen(Qt::darkGray, 3));
            painter.setBrush(QBrush(m_parentWidget->m_backgroundColor));
            painter.drawRect(0, 0, width()-1, height()-1);
            if (!m_parentWidget->m_image.isNull())
                painter.drawImage(m_parentWidget->m_imagePosition, m_parentWidget->m_image);
        }

        SplashScreenWidget *m_parentWidget;
    };

public:
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
    FilePath getManifestDirectory();

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

FilePath SplashScreenWidget::getManifestDirectory()
{
    const QWidget *parentWidget = this->parentWidget();

    while (parentWidget) {
        const auto *container = qobject_cast<const SplashScreenContainerWidget *>(parentWidget);
        if (container)
            return container->manifestDirectory();
        parentWidget = parentWidget->parentWidget();
    }
       return FilePath();
}

SplashScreenWidget::SplashScreenWidget(QWidget *parent, const QSize &size, const QSize &screenSize,
                                       const QString &title, const QString &tooltip,
                                       const QString &imagePath, int scalingRatio, int maxScalingRatio,
                                       TextEditor::TextEditorWidget *textEditorWidget):
    QWidget(parent), m_textEditorWidget(textEditorWidget),
    m_scalingRatio(scalingRatio), m_maxScalingRatio(maxScalingRatio),
    m_imagePath(imagePath), m_screenSize(screenSize)
{
    auto splashLayout = new QVBoxLayout(this);
    splashLayout->addStretch(1);
    auto splashTitle = new QLabel(title, this);
    auto splashButtonLayout = new QGridLayout();
    splashButtonLayout->setHorizontalSpacing(0);
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
    splashButtonLayout->addWidget(m_splashScreenButton, 0, 0, 1, 1);
    if (textEditorWidget) {
        splashButtonLayout->addWidget(clearButton, 0, 0, 1, 1);
        splashButtonLayout->setAlignment(clearButton, Qt::AlignTop | Qt::AlignRight);
    }
    if (textEditorWidget) {
        splashButtonLayout->addWidget(m_scaleWarningLabel, 0, 0, 1, 1);
        splashButtonLayout->setAlignment(m_scaleWarningLabel, Qt::AlignTop);
    }
    splashLayout->addLayout(splashButtonLayout);
    splashLayout->setAlignment(splashButtonLayout, Qt::AlignHCenter);
    splashLayout->addWidget(splashTitle);
    splashLayout->setAlignment(splashTitle, Qt::AlignCenter);
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
    QImage image = QImage(imagePath.toFSPathString());
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
    QFile file(targetPath.toFSPathString());
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
    const FilePath file = FileUtils::getOpenFilePath(m_imageSelectionText,
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
    const FilePath baseDir = getManifestDirectory();
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
    const FilePath baseDir = getManifestDirectory();
    const FilePath targetPath = baseDir / m_imagePath / m_imageFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image target path empty, cannot load image.";
        return;
    }
    QImage image = QImage(targetPath.toFSPathString());
    if (image.isNull()) {
        qCDebug(androidManifestEditorLog).noquote() << "Cannot load image." << targetPath.toUserOutput();
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

static QSize preferredSplashViewSize()
{
    const QScreen  *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    const QSize screenSize = screen->availableSize();
    const int baseHeight = screenSize.height() / 4;
    const int baseWidth = baseHeight * 2 / 3; // ~2:3 portrait aspect ratio

    return QSize(baseWidth, baseHeight);
}

static QWidget *createPage(TextEditor::TextEditorWidget *textEditorWidget,
                           QList<SplashScreenWidget *> &widgetContainer,
                           QList<SplashScreenWidget *> &portraitWidgetContainer,
                           QList<SplashScreenWidget *> &landscapeWidgetContainer,
                           int scalingRatio, const QSize &size, const QSize &portraitSize,
                           const QSize &landscapeSize, const QString &path, const QString &tabTitle)
{
    auto sizeToStr = [](const QSize &size) { return QString(" (%1x%2)").arg(size.width()).arg(size.height()); };
    QWidget *page = new QWidget();
    auto mainPageLayout = new QVBoxLayout(page);

    auto widgetsGrid = new QGridLayout();

    const QSize displaySize = preferredSplashViewSize();
    const QSize landscapeDisplaySize = displaySize.transposed();

    widgetsGrid->setRowStretch(0, 1);
    widgetsGrid->setColumnStretch(0, 1);

    auto genericWidget = new SplashScreenWidget(page, displaySize, size,
                        Tr::tr("Master"), Tr::tr("Select splash screen image") + sizeToStr(size),
                        path, scalingRatio, extraExtraExtraHighDpiScalingRatio, textEditorWidget);
    widgetContainer.push_back(genericWidget);
    widgetsGrid->addWidget(genericWidget, 1, 1, Qt::AlignBottom);

    widgetsGrid->setColumnStretch(2, 1);

    auto portraitWidget = new SplashScreenWidget(page, displaySize, portraitSize,
                         Tr::tr("Portrait"),
                         Tr::tr("Select portrait splash screen image") + sizeToStr(portraitSize),
                         path, scalingRatio, extraExtraExtraHighDpiScalingRatio, textEditorWidget);
    portraitWidgetContainer.push_back(portraitWidget);
    widgetsGrid->addWidget(portraitWidget, 1, 3, Qt::AlignBottom);

    widgetsGrid->setColumnStretch(4, 1);

    auto landscapeWidget = new SplashScreenWidget(page, landscapeDisplaySize, landscapeSize,
                          Tr::tr("Landscape"),
                          Tr::tr("Select landscape splash screen image") + sizeToStr(landscapeSize),
                          path, scalingRatio, extraExtraExtraHighDpiScalingRatio, textEditorWidget);
    landscapeWidgetContainer.push_back(landscapeWidget);
    widgetsGrid->addWidget(landscapeWidget, 1, 5, Qt::AlignBottom);

    widgetsGrid->setColumnStretch(6, 1);

    auto clearButton = new QToolButton(page);
    clearButton->setText(Tr::tr("Clear %1").arg(tabTitle));
    widgetsGrid->addWidget(clearButton, 3, 3, Qt::AlignHCenter | Qt::AlignTop);

    mainPageLayout->addLayout(widgetsGrid);

    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       genericWidget, &SplashScreenWidget::clearImage);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       portraitWidget, &SplashScreenWidget::clearImage);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       landscapeWidget, &SplashScreenWidget::clearImage);
    return page;
}


SplashScreenContainerWidget::SplashScreenContainerWidget(QWidget *parent)
    : QStackedWidget(parent), m_textEditorWidget(nullptr)
{}

bool SplashScreenContainerWidget::initialize(TextEditor::TextEditorWidget *textEditorWidget)
{
    QTC_ASSERT(textEditorWidget, return false);
    m_textEditorWidget = textEditorWidget;
    m_manifestDirectory = manifestDir(textEditorWidget, false);

    const QList<QStringList> imageShowModeMethodsMap = {
        {"center", Tr::tr("Place the object in the center of the screen in both the vertical and horizontal axis,\n"
                          "not changing its size.")},
        {"fill", Tr::tr("Grow the horizontal and vertical size of the image if needed so it completely fills its screen.")}
    };

    m_stickyCheck = new QCheckBox(this);
    m_stickyCheck->setToolTip(Tr::tr("A non-sticky splash screen is hidden automatically when an activity is drawn.\n"
                                     "To hide a sticky splash screen, invoke QtAndroid::hideSplashScreen()."));

    m_imageShowMode = new QComboBox(this);
    m_imageShowMode->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    for (int i = 0; i < imageShowModeMethodsMap.size(); ++i) {
        m_imageShowMode->addItem(imageShowModeMethodsMap.at(i).first());
        m_imageShowMode->setItemData(i, imageShowModeMethodsMap.at(i).at(1), Qt::ToolTipRole);
    }

    m_masterImage = new QToolButton(this);
    m_masterImage->setText(Tr::tr("Select master image"));
    m_masterImage->setToolTip(Tr::tr("Select master image to use."));
    m_masterImage->setIcon(Utils::Icon::fromTheme("document-open"));
    m_masterImage->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);


    m_portraitMasterImage = new QToolButton(this);
    m_portraitMasterImage->setText(Tr::tr("Select portrait image"));
    m_portraitMasterImage->setToolTip(Tr::tr("Select portrait image to use."));
    m_portraitMasterImage->setIcon(Utils::Icon::fromTheme("document-open"));
    m_portraitMasterImage->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_landscapeMasterImage = new QToolButton(this);
    m_landscapeMasterImage->setText(Tr::tr("Select landscape image"));
    m_landscapeMasterImage->setToolTip(Tr::tr("Select landscape image to use."));
    m_landscapeMasterImage->setIcon(Utils::Icon::fromTheme("document-open"));
    m_landscapeMasterImage->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_backgroundColor = new QToolButton(this);

    auto *clearAllButton = new QToolButton(this);
    clearAllButton->setText(Tr::tr("Clear All"));

    auto *warningLabel = new QLabel(this);
    warningLabel->setAlignment(Qt::AlignHCenter);
    warningLabel->setWordWrap(true);

    auto *containerWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(containerWidget);

    QTabWidget *tab = new QTabWidget(this);
    tab->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    tab->addTab(createPage(textEditorWidget, m_imageWidgets, m_portraitImageWidgets,
                           m_landscapeImageWidgets, lowDpiScalingRatio, lowDpiImageSize,
                           lowDpiImageSize, lowDpiImageSize.transposed(), lowDpiImagePath, Tr::tr("LDPI")), Tr::tr("LDPI"));
    tab->addTab(createPage(textEditorWidget, m_imageWidgets, m_portraitImageWidgets,
                           m_landscapeImageWidgets, mediumDpiScalingRatio, mediumDpiImageSize,
                           mediumDpiImageSize, mediumDpiImageSize.transposed(), mediumDpiImagePath, Tr::tr("MDPI")), Tr::tr("MDPI"));
    tab->addTab(createPage(textEditorWidget, m_imageWidgets, m_portraitImageWidgets,
                           m_landscapeImageWidgets, highDpiScalingRatio, highDpiImageSize,
                           highDpiImageSize, highDpiImageSize.transposed(), highDpiImagePath, Tr::tr("HDPI")), Tr::tr("HDPI"));
    tab->addTab(createPage(textEditorWidget, m_imageWidgets, m_portraitImageWidgets,
                           m_landscapeImageWidgets, extraHighDpiScalingRatio, extraHighDpiImageSize,
                           extraHighDpiImageSize, extraHighDpiImageSize.transposed(), extraHighDpiImagePath,
                           Tr::tr("XHDPI")), Tr::tr("XHDPI"));
    tab->addTab(createPage(textEditorWidget, m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                           extraExtraHighDpiScalingRatio, extraExtraHighDpiImageSize, extraExtraHighDpiImageSize,
                           extraExtraHighDpiImageSize.transposed(), extraExtraHighDpiImagePath, Tr::tr("XXHDPI")), Tr::tr("XXHDPI"));
    tab->addTab(createPage(textEditorWidget, m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                           extraExtraExtraHighDpiScalingRatio, extraExtraExtraHighDpiImageSize,
                           extraExtraExtraHighDpiImageSize, extraExtraExtraHighDpiImageSize.transposed(),
                           extraExtraExtraHighDpiImagePath, Tr::tr("XXXDPI")), Tr::tr("XXXDPI"));

    auto *gridLayout = new QGridLayout();
    gridLayout->addWidget(m_masterImage, 0, 0);
    gridLayout->addWidget(m_portraitMasterImage, 0, 1);
    gridLayout->addWidget(m_landscapeMasterImage, 0, 2);
    gridLayout->addWidget(clearAllButton, 0, 3, Qt::AlignLeft);

    auto *settingsLayout = new QFormLayout();
    settingsLayout->setLabelAlignment(Qt::AlignLeft);
    m_imageShowMode->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    settingsLayout->addRow(Tr::tr("Image show mode:"), m_imageShowMode);
    settingsLayout->addRow(Tr::tr("Background Colour:"), m_backgroundColor);
    settingsLayout->addRow(Tr::tr("Sticky splash screen:"), m_stickyCheck);

    mainLayout->addWidget(tab);
    mainLayout->addLayout(gridLayout);
    mainLayout->addLayout(settingsLayout);
    containerWidget->setLayout(mainLayout);
    addWidget(containerWidget);

    setBackgroundColor(Qt::white);

    const auto splashFileName = QString(splashscreenFileName).append(imageSuffix);
    const auto portraitSplashFileName = QString(splashscreenPortraitFileName).append(imageSuffix);
    const auto landscapeSplashFileName = QString(splashscreenLandscapeFileName).append(imageSuffix);

    auto processWidgets = [this](QList<SplashScreenWidget*> &widgetList, const QString &fileName) {
        for (auto &&imageWidget : widgetList) {
            imageWidget->setImageFileName(fileName);
            connect(imageWidget, &SplashScreenWidget::imageChanged, this, [this] {
                createSplashscreenThemes();
                emit splashScreensModified();
            });
        }
    };

    processWidgets(m_imageWidgets, splashFileName);
    processWidgets(m_portraitImageWidgets, portraitSplashFileName);
    processWidgets(m_landscapeImageWidgets, landscapeSplashFileName);

    connect(m_stickyCheck, &QCheckBox::stateChanged, this, [this](int state) {
        bool old = m_splashScreenSticky;
        m_splashScreenSticky = (state == Qt::Checked);
        if (old != m_splashScreenSticky) {
            const FilePath manifestFile = manifestDirectory().pathAppended(QLatin1String("AndroidManifest.xml"));
            if (manifestFile.exists()) {
                const QString stickyValue = m_splashScreenSticky ? QLatin1String("true") : QString();
                updateManifestActivityMetaData(manifestFile,
                                               QLatin1String("android.app.splash_screen_sticky"),
                                               stickyValue);
            }
            emit splashScreensModified();
        }
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
        const Utils::FilePath file = FileUtils::getOpenFilePath(Tr::tr("Select master image"),
                                                                FileUtils::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_imageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_portraitMasterImage, &QToolButton::clicked, this, [this] {
        const Utils::FilePath file = FileUtils::getOpenFilePath(Tr::tr("Select portrait master image"),
                                                                FileUtils::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_portraitImageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_landscapeMasterImage, &QToolButton::clicked, this, [this] {
        const Utils::FilePath file = FileUtils::getOpenFilePath(Tr::tr("Select landscape master image"),
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
    if (m_convertSplashscreen) {
        connect(m_convertSplashscreen, &QToolButton::clicked, this, [this] {
            clearAll();
            setCurrentIndex(0);
            emit splashScreensModified();
        });
    }

    loadImages();
    return true;
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

        const FilePath manifestFile = manifestDirectory().pathAppended(QLatin1String("AndroidManifest.xml"));
        if (manifestFile.exists()) {
            auto stickyResult = readManifestActivityMetaData(manifestFile,
                                                             QLatin1String("android.app.splash_screen_sticky"));
            if (stickyResult) {
                const bool sticky = (stickyResult.value() == QLatin1String("true"));
                m_splashScreenSticky = sticky;
                if (m_stickyCheck)
                    m_stickyCheck->setChecked(sticky);
            }
        }
    }
}

void SplashScreenContainerWidget::loadSplashscreenDrawParams(const QString &name)
{
    const FilePath filePath = manifestDirectory().pathAppended("res/drawable/" + name + ".xml");
    QFile file(filePath.toFSPathString());
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
    const QStringList splashscreenThemeFiles = {"res/values/splashscreentheme.xml",
                                                "res/values-port/splashscreentheme.xml",
                                                "res/values-land/splashscreentheme.xml"};
    const QStringList splashscreenDrawableFiles = {QString("res/drawable/%1.xml").arg(splashscreenName),
                                                   QString("res/drawable/%1.xml").arg(splashscreenPortraitName),
                                                   QString("res/drawable/%1.xml").arg(splashscreenLandscapeName)};
    QStringList splashscreens[3];

    if (hasImages())
        splashscreens[0] << splashscreenName << splashscreenFileName;
    if (hasPortraitImages())
        splashscreens[1] << splashscreenPortraitName << splashscreenPortraitFileName;
    if (hasLandscapeImages())
        splashscreens[2] << splashscreenLandscapeName << splashscreenLandscapeFileName;

    const FilePath currentManifestDir = manifestDirectory();
    for (int i = 0; i < 3; i++) {
        const FilePath splashscreenThemeFile = currentManifestDir.pathAppended(splashscreenThemeFiles[i]);
        const FilePath splashscreenDrawableFile = currentManifestDir.pathAppended(splashscreenDrawableFiles[i]);
        if (!splashscreens[i].isEmpty()) {
            QDir dir;
            QFile themeFile(splashscreenThemeFile.toFSPathString());
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
            QFile drawableFile(splashscreenDrawableFile.toFSPathString());
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
            splashscreenThemeFile.removeFile();
            splashscreenDrawableFile.removeFile();
        }
    }

    FilePath manifestFile = currentManifestDir.pathAppended(QLatin1String("AndroidManifest.xml"));
    if (manifestFile.exists()) {
        bool hasSplashScreens = !splashscreens[0].isEmpty()
                                || !splashscreens[1].isEmpty()
                                || !splashscreens[2].isEmpty();
        const QString themeValue = hasSplashScreens ? QLatin1String("@style/splashScreenTheme")
                                                     : QString();
        auto result = updateManifestApplicationAttribute(manifestFile,
                                                         QLatin1String("android:theme"),
                                                         themeValue);
        if (!result)
            qCDebug(androidManifestEditorLog) << "Failed to update manifest theme:" << result.error();

        const QString drawableValue = hasSplashScreens ? QLatin1String("@drawable/") + QLatin1String(splashscreenName)
                                                       : QString();
        auto drawableResult = updateManifestActivityMetaData(manifestFile,
                                                             QLatin1String("android.app.splash_screen_drawable"),
                                                             drawableValue);
        if (!drawableResult)
            qCDebug(androidManifestEditorLog) << "Failed to update manifest splash_screen_drawable:" << drawableResult.error();
    }
}

void SplashScreenContainerWidget::checkSplashscreenImage(const QString &name)
{
    if (isSplashscreenEnabled()) {
        const QString paths[] = {extraExtraExtraHighDpiImagePath,
                                 extraExtraHighDpiImagePath,
                                 extraHighDpiImagePath,
                                 highDpiImagePath,
                                 mediumDpiImagePath,
                                 lowDpiImagePath};

        const FilePath currentManifestDir = manifestDirectory();
        for (const QString &path : paths) {
            const FilePath filePath = currentManifestDir.pathAppended(path + name);
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

FilePath SplashScreenContainerWidget::manifestDirectory() const
{
    if (m_manifestDirectory.isEmpty() && m_textEditorWidget)
        return manifestDir(m_textEditorWidget, false);
    return m_manifestDirectory;
}

} // namespace Android::Internal

#include "splashscreencontainerwidget.moc"
