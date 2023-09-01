// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidtr.h"
#include "splashscreencontainerwidget.h"
#include "splashscreenwidget.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/filepath.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace Utils;

namespace Android {
namespace Internal {

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

static SplashScreenWidget *addWidgetToPage(QWidget *page,
                                           const QSize &size, const QSize &screenSize,
                                           const QString &title, const QString &tooltip,
                                           TextEditor::TextEditorWidget *textEditorWidget,
                                           const QString &splashScreenPath,
                                           int scalingRatio, int maxScalingRatio,
                                           QHBoxLayout *pageLayout,
                                           QVector<SplashScreenWidget *> &widgetContainer)
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
                           QVector<SplashScreenWidget *> &widgetContainer,
                           QVector<SplashScreenWidget *> &portraitWidgetContainer,
                           QVector<SplashScreenWidget *> &landscapeWidgetContainer,
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

} // namespace Internal
} // namespace Android
