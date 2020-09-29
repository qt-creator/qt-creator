/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "splashiconcontainerwidget.h"
#include "androidmanifesteditoriconwidget.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QFileInfo>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace Android {
namespace Internal {

namespace {
const QString extraExtraExtraHighDpiImagePath = QLatin1String("/res/drawable-xxxhdpi/");
const QString extraExtraHighDpiImagePath = QLatin1String("/res/drawable-xxhdpi/");
const QString extraHighDpiImagePath = QLatin1String("/res/drawable-xhdpi/");
const QString highDpiImagePath = QLatin1String("/res/drawable-hdpi/");
const QString mediumDpiImagePath = QLatin1String("/res/drawable-mdpi/");
const QString lowDpiImagePath = QLatin1String("/res/drawable-ldpi/");
const QString extraExtraExtraHighDpiPortraitImagePath = QLatin1String("/res/drawable-port-xxxhdpi/");
const QString extraExtraHighDpiPortraitImagePath = QLatin1String("/res/drawable-port-xxhdpi/");
const QString extraHighDpiPortraitImagePath = QLatin1String("/res/drawable-port-xhdpi/");
const QString highDpiPortraitImagePath = QLatin1String("/res/drawable-port-hdpi/");
const QString mediumDpiPortraitImagePath = QLatin1String("/res/drawable-port-mdpi/");
const QString lowDpiPortraitImagePath = QLatin1String("/res/drawable-port-ldpi/");
const QString extraExtraExtraHighDpiLandscapeImagePath = QLatin1String("/res/drawable-land-xxxhdpi/");
const QString extraExtraHighDpiLandscapeImagePath = QLatin1String("/res/drawable-land-xxhdpi/");
const QString extraHighDpiLandscapeImagePath = QLatin1String("/res/drawable-land-xhdpi/");
const QString highDpiLandscapeImagePath = QLatin1String("/res/drawable-land-hdpi/");
const QString mediumDpiLandscapeImagePath = QLatin1String("/res/drawable-land-mdpi/");
const QString lowDpiLandscapeImagePath = QLatin1String("/res/drawable-land-ldpi/");
const QString imageSuffix = QLatin1String(".png");
const QSize lowDpiImageSize{200, 320};
const QSize mediumDpiImageSize{320, 480};
const QSize highDpiImageSize{480, 800};
const QSize extraHighDpiImageSize{720, 1280};
const QSize extraExtraHighDpiImageSize{960, 1600};
const QSize extraExtraExtraHighDpiImageSize{1280, 1920};
const QSize displaySize{48, 72};
const QSize landscapeDisplaySize{72, 48};
QString manifestDir(TextEditor::TextEditorWidget *textEditorWidget)
{
    // Get the manifest file's directory from its filepath.
    return textEditorWidget->textDocument()->filePath().toFileInfo().absolutePath();
}
}

static AndroidManifestEditorIconWidget * addButtonToPage(QWidget *page,
                                                         const QSize &splashSize, const QSize &splashDisplaySize,
                                                         const QString &title, const QString &tooltip,
                                                         TextEditor::TextEditorWidget *textEditorWidget,
                                                         const QString &splashPath,
                                                         const QString &splashFileName,
                                                         QHBoxLayout *iconLayout,
                                                         QVector<AndroidManifestEditorIconWidget *> &buttonContainer,
                                                         bool scaledToOriginal,
                                                         bool scaledWithoutStretching)
{
    auto splashButton = new AndroidManifestEditorIconWidget(page,
                                                            splashSize,
                                                            splashDisplaySize,
                                                            title,
                                                            tooltip,
                                                            textEditorWidget,
                                                            splashPath, splashFileName);
    splashButton->setScaledToOriginalAspectRatio(scaledToOriginal);
    splashButton->setScaledWithoutStretching(scaledWithoutStretching);
    iconLayout->addWidget(splashButton);
    buttonContainer.push_back(splashButton);
    return splashButton;
}

static QWidget *createPage(TextEditor::TextEditorWidget *textEditorWidget,
                           QVector<AndroidManifestEditorIconWidget *> &buttonContainer,
                           QVector<AndroidManifestEditorIconWidget *> &portraitButtonContainer,
                           QVector<AndroidManifestEditorIconWidget *> &landscapeButtonContainer,
                           const QSize &splashSize,
                           const QSize &portraitSplashSize,
                           const QSize &landscapeSplashSize,
                           const QString &splashSizeTooltip,
                           const QString &portraitSplashSizeTooltip,
                           const QString &landscapeSplashSizeTooltip,
                           const QString &splashPath,
                           const QString &splashFileName,
                           const QString &portraitSplashPath,
                           const QString &portraitSplashFileName,
                           const QString &landscapeSplashPath,
                           const QString &landscapeSplashFileName)
{
    QWidget *page = new QWidget();
    auto iconLayout = new QHBoxLayout(page);
    auto splashButton= addButtonToPage(page, splashSize, landscapeDisplaySize,
                                       SplashIconContainerWidget::tr("Splash screen"),
                                       SplashIconContainerWidget::tr("Select splash screen image")
                                       + splashSizeTooltip,
                                       textEditorWidget,
                                       splashPath, splashFileName + imageSuffix,
                                       iconLayout,
                                       buttonContainer,
                                       true, false);

    auto portraitButton = addButtonToPage(page, portraitSplashSize, displaySize,
                                          SplashIconContainerWidget::tr("Portrait splash screen"),
                                          SplashIconContainerWidget::tr("Select portrait splash screen image")
                                          + portraitSplashSizeTooltip,
                                          textEditorWidget,
                                          portraitSplashPath, portraitSplashFileName + imageSuffix,
                                          iconLayout,
                                          portraitButtonContainer,
                                          false, true);

    auto landscapeButton = addButtonToPage(page, landscapeSplashSize, landscapeDisplaySize,
                                           SplashIconContainerWidget::tr("Landscape splash screen"),
                                           SplashIconContainerWidget::tr("Select landscape splash screen image")
                                           + landscapeSplashSizeTooltip,
                                           textEditorWidget,
                                           landscapeSplashPath, landscapeSplashFileName + imageSuffix,
                                           iconLayout,
                                           landscapeButtonContainer,
                                           false, true);

    auto clearButton = new QToolButton(page);
    clearButton->setText(SplashIconContainerWidget::tr("Clear all"));
    iconLayout->addWidget(clearButton);
    iconLayout->setAlignment(clearButton, Qt::AlignVCenter);
    SplashIconContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       splashButton, &AndroidManifestEditorIconWidget::clearIcon);
    SplashIconContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       portraitButton, &AndroidManifestEditorIconWidget::clearIcon);
    SplashIconContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       landscapeButton, &AndroidManifestEditorIconWidget::clearIcon);
    return page;
}


SplashIconContainerWidget::SplashIconContainerWidget(
        QWidget *parent,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QWidget(parent), m_textEditorWidget(textEditorWidget)
{
    auto layout = new QVBoxLayout(this);
    m_stickyCheck = new QCheckBox(tr("Sticky splash screen"), this);
    m_stickyCheck->setToolTip(tr("A non-sticky splash screen is hidden automatically when an activity is drawn.\n"
                                 "To hide a sticky splash screen, invoke QtAndroid::hideSplashScreen()."));
    QTabWidget *tab = new QTabWidget(this);
    auto sizeToStr = [](const QSize &size) { return QString(" (%1x%2)").arg(size.width()).arg(size.height()); };

    auto xxxHdpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               extraExtraExtraHighDpiImageSize,
                               extraExtraExtraHighDpiImageSize,
                               extraExtraExtraHighDpiImageSize.transposed(),
                               sizeToStr(extraExtraExtraHighDpiImageSize), sizeToStr(extraExtraExtraHighDpiImageSize), sizeToStr(extraExtraExtraHighDpiImageSize.transposed()),
                               extraExtraExtraHighDpiImagePath, m_imageFileName,
                               extraExtraExtraHighDpiImagePath, m_portraitImageFileName,
                               extraExtraExtraHighDpiImagePath, m_landscapeImageFileName);
    tab->addTab(xxxHdpiPage, tr("XXXHDPI splash screen"));
    auto xxHdpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               extraExtraHighDpiImageSize,
                               extraExtraHighDpiImageSize,
                               extraExtraHighDpiImageSize.transposed(),
                               sizeToStr(extraExtraHighDpiImageSize), sizeToStr(extraExtraHighDpiImageSize), sizeToStr(extraExtraHighDpiImageSize.transposed()),
                               extraExtraHighDpiImagePath, m_imageFileName,
                               extraExtraHighDpiImagePath, m_portraitImageFileName,
                               extraExtraHighDpiImagePath, m_landscapeImageFileName);
    tab->addTab(xxHdpiPage, tr("XXHDPI splash screen"));
    auto xHdpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               extraHighDpiImageSize,
                               extraHighDpiImageSize,
                               extraHighDpiImageSize.transposed(),
                               sizeToStr(extraHighDpiImageSize), sizeToStr(extraHighDpiImageSize), sizeToStr(extraHighDpiImageSize.transposed()),
                               extraHighDpiImagePath, m_imageFileName,
                               extraHighDpiImagePath, m_portraitImageFileName,
                               extraHighDpiImagePath, m_landscapeImageFileName);
    tab->addTab(xHdpiPage, tr("XHDPI splash screen"));
    auto hdpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               highDpiImageSize,
                               highDpiImageSize,
                               highDpiImageSize.transposed(),
                               sizeToStr(highDpiImageSize), sizeToStr(highDpiImageSize), sizeToStr(highDpiImageSize.transposed()),
                               highDpiImagePath, m_imageFileName,
                               highDpiImagePath, m_portraitImageFileName,
                               highDpiImagePath, m_landscapeImageFileName);
    tab->addTab(hdpiPage, tr("HDPI splash screen"));
    auto mdpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               mediumDpiImageSize,
                               mediumDpiImageSize,
                               mediumDpiImageSize.transposed(),
                               sizeToStr(mediumDpiImageSize), sizeToStr(mediumDpiImageSize), sizeToStr(mediumDpiImageSize.transposed()),
                               mediumDpiImagePath, m_imageFileName,
                               mediumDpiImagePath, m_portraitImageFileName,
                               mediumDpiImagePath, m_landscapeImageFileName);
    tab->addTab(mdpiPage, tr("MDPI splash screen"));
    auto ldpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               lowDpiImageSize,
                               lowDpiImageSize,
                               lowDpiImageSize.transposed(),
                               sizeToStr(lowDpiImageSize), sizeToStr(lowDpiImageSize), sizeToStr(lowDpiImageSize.transposed()),
                               lowDpiImagePath, m_imageFileName,
                               lowDpiImagePath, m_portraitImageFileName,
                               lowDpiImagePath, m_landscapeImageFileName);
    tab->addTab(ldpiPage, tr("LDPI splash screen"));
    layout->addWidget(m_stickyCheck);
    layout->setAlignment(m_stickyCheck, Qt::AlignLeft);
    layout->addWidget(tab);
    for (auto &&imageButton : m_imageButtons) {
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::splashScreensModified);
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::imageSelected);
    }
    for (auto &&imageButton : m_portraitImageButtons) {
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::splashScreensModified);
        connect(imageButton, &AndroidManifestEditorIconWidget::iconRemoved,
                this, &SplashIconContainerWidget::splashScreensModified);
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::imageSelected);
    }
    for (auto &&imageButton : m_landscapeImageButtons) {
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::splashScreensModified);
        connect(imageButton, &AndroidManifestEditorIconWidget::iconRemoved,
                this, &SplashIconContainerWidget::splashScreensModified);
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::imageSelected);
    }
    connect(m_stickyCheck, &QCheckBox::stateChanged, [this](int state) {
        bool old = m_splashScreenSticky;
        m_splashScreenSticky = (state == Qt::Checked);
        if (old != m_splashScreenSticky)
            splashScreensModified();
    });
}

static void maybeChangeImagePath(AndroidManifestEditorIconWidget *widget,
                                 TextEditor::TextEditorWidget *m_textEditorWidget,
                                 const QString &extraExtraExtraHighDpiAlternativePath,
                                 const QString &extraExtraHighDpiAlternativePath,
                                 const QString &extraHighDpiAlternativePath,
                                 const QString &highDpiAlternativePath,
                                 const QString &mediumDpiAlternativePath,
                                 const QString &lowDpiAlternativePath)
{
    QString currentPath = widget->targetIconPath();
    QString alternativePath = currentPath;
    if (currentPath == extraExtraExtraHighDpiImagePath)
        alternativePath = extraExtraExtraHighDpiAlternativePath;
    else if (currentPath == extraExtraHighDpiImagePath)
        alternativePath = extraExtraHighDpiAlternativePath;
    else if (currentPath == extraHighDpiImagePath)
        alternativePath = extraHighDpiAlternativePath;
    else if (currentPath == highDpiImagePath)
        alternativePath = highDpiAlternativePath;
    else if (currentPath == mediumDpiImagePath)
        alternativePath = mediumDpiAlternativePath;
    else if (currentPath == lowDpiImagePath)
        alternativePath = lowDpiAlternativePath;
    QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + alternativePath + widget->targetIconFileName();
    QFileInfo image(targetPath);
    if (image.exists())
        widget->setTargetIconPath(alternativePath);
}

void SplashIconContainerWidget::loadImages()
{
    for (auto &&imageButton : m_imageButtons) {
        imageButton->setTargetIconFileName(m_imageFileName + imageSuffix);
        imageButton->loadIcon();
    }
    for (auto &&imageButton : m_portraitImageButtons) {
        imageButton->setTargetIconFileName(m_portraitImageFileName + imageSuffix);
        maybeChangeImagePath(imageButton, m_textEditorWidget,
                             extraExtraExtraHighDpiPortraitImagePath, extraExtraHighDpiPortraitImagePath, extraHighDpiPortraitImagePath,
                             highDpiPortraitImagePath, mediumDpiPortraitImagePath, lowDpiPortraitImagePath);
        imageButton->loadIcon();
    }
    for (auto &&imageButton : m_landscapeImageButtons) {
        imageButton->setTargetIconFileName(m_landscapeImageFileName + imageSuffix);
        maybeChangeImagePath(imageButton, m_textEditorWidget,
                             extraExtraExtraHighDpiLandscapeImagePath, extraExtraHighDpiLandscapeImagePath, extraHighDpiLandscapeImagePath,
                             highDpiLandscapeImagePath, mediumDpiLandscapeImagePath, lowDpiLandscapeImagePath);
        imageButton->loadIcon();
    }
}

bool SplashIconContainerWidget::hasImages() const
{
    for (auto &&iconButton : m_imageButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

bool SplashIconContainerWidget::hasPortraitImages() const
{
    for (auto &&iconButton : m_portraitImageButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

bool SplashIconContainerWidget::hasLandscapeImages() const
{
    for (auto &&iconButton : m_landscapeImageButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

bool SplashIconContainerWidget::isSticky() const
{
    return m_splashScreenSticky;
}

void SplashIconContainerWidget::setSticky(bool sticky)
{
    m_splashScreenSticky = sticky;
    m_stickyCheck->setCheckState(m_splashScreenSticky ? Qt::Checked : Qt::Unchecked);
}

QString SplashIconContainerWidget::imageFileName() const
{
    return m_imageFileName;
}

QString SplashIconContainerWidget::landscapeImageFileName() const
{
    return m_landscapeImageFileName;
}

QString SplashIconContainerWidget::portraitImageFileName() const
{
    return m_portraitImageFileName;
}

void SplashIconContainerWidget::setImageFileName(const QString &name)
{
    m_imageFileName = name;
}

void SplashIconContainerWidget::setLandscapeImageFileName(const QString &name)
{
    m_landscapeImageFileName = name;
}

void SplashIconContainerWidget::setPortraitImageFileName(const QString &name)
{
    m_portraitImageFileName = name;
}

static void reflectImage(const QString &path,
                         AndroidManifestEditorIconWidget *iconWidget,
                         const QVector<AndroidManifestEditorIconWidget *> &source,
                         QVector<AndroidManifestEditorIconWidget *> *firstTarget,
                         QVector<AndroidManifestEditorIconWidget *> *secondTarget = nullptr,
                         Qt::Orientation *imageOrientation = nullptr)
{
    for (int i = 0; i < source.size(); ++i)
        if (source[i] == iconWidget) {
            if (firstTarget && !(*firstTarget)[i]->hasIcon()
                    && (!imageOrientation || *imageOrientation == Qt::Horizontal))
                (*firstTarget)[i]->setIconFromPath(path);
            if (secondTarget && !(*secondTarget)[i]->hasIcon()
                    && (!imageOrientation || *imageOrientation == Qt::Vertical))
                (*secondTarget)[i]->setIconFromPath(path);
            break;
        }

}

void SplashIconContainerWidget::imageSelected(const QString &path, AndroidManifestEditorIconWidget *iconWidget)
{
    QImage original(path);
    Qt::Orientation orientation = Qt::Horizontal;
    if (!original.isNull()) {
        if (original.width() > original.height()) {
            orientation = Qt::Horizontal;
        } else {
            orientation = Qt::Vertical;
        }
    }
    reflectImage(path, iconWidget,
                 m_imageButtons, &m_portraitImageButtons, &m_landscapeImageButtons, &orientation);
    reflectImage(path, iconWidget, m_portraitImageButtons, &m_landscapeImageButtons);
    reflectImage(path, iconWidget, m_landscapeImageButtons, &m_portraitImageButtons);
}

} // namespace Internal
} // namespace Android
