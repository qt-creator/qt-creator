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

#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace Android {
namespace Internal {

namespace {
const QString highDpiImagePath = "/res/drawable-hdpi/logo.png";
const QString mediumDpiImagePath = "/res/drawable-mdpi/logo.png";
const QString lowDpiImagePath = "/res/drawable-ldpi/logo.png";
const QString highDpiLandscapeImagePath = "/res/drawable-hdpi/logo_landscape.png";
const QString mediumDpiLandscapeImagePath = "/res/drawable-mdpi/logo_landscape.png";
const QString lowDpiLandscapeImagePath = "/res/drawable-ldpi/logo_landscape.png";
const QString highDpiPortraitImagePath = "/res/drawable-hdpi/logo_portrait.png";
const QString mediumDpiPortraitImagePath = "/res/drawable-mdpi/logo_portrait.png";
const QString lowDpiPortraitImagePath = "/res/drawable-ldpi/logo_portrait.png";
const QSize lowDpiImageSize{200, 320};
const QSize mediumDpiImageSize{320, 480};
const QSize highDpiImageSize{480, 720};
const QSize lowDpiLandscapeImageSize{320, 200};
const QSize mediumDpiLandscapeImageSize{480, 320};
const QSize highDpiLandscapeImageSize{720, 480};
const QSize displaySize{48, 72};
const QSize landscapeDisplaySize{72, 48};
}

static QWidget *createPage(TextEditor::TextEditorWidget *textEditorWidget,
                           QVector<AndroidManifestEditorIconWidget *> &buttonContainer,
                           QVector<AndroidManifestEditorIconWidget *> &portraitButtonContainer,
                           QVector<AndroidManifestEditorIconWidget *> &landscapeButtonContainer,
                           const QSize &splashSize,
                           const QSize &portraitSplashSize,
                           const QSize &landscapeSplashSize,
                           const QString &splashPath,
                           const QString &portraitSplashPath,
                           const QString &landscapeSplashPath)
{
    QWidget *page = new QWidget();
    auto iconLayout = new QHBoxLayout(page);
    auto splashButton = new AndroidManifestEditorIconWidget(page,
                                                            splashSize,
                                                            landscapeDisplaySize,
                                                            SplashIconContainerWidget::tr("Splash screen"),
                                                            SplashIconContainerWidget::tr("Select splash screen image"),
                                                            textEditorWidget, splashPath);
    splashButton->setScaled(false);
    iconLayout->addWidget(splashButton);
    buttonContainer.push_back(splashButton);
    iconLayout->addStretch(1);

    auto portraitButton = new AndroidManifestEditorIconWidget(page,
                                                              portraitSplashSize,
                                                              displaySize,
                                                              SplashIconContainerWidget::tr("Portrait splash screen"),
                                                              SplashIconContainerWidget::tr("Select portrait splash screen image"),
                                                              textEditorWidget, portraitSplashPath);
    iconLayout->addWidget(portraitButton);
    portraitButtonContainer.push_back(portraitButton);
    iconLayout->addStretch(1);

    auto landscapeButton = new AndroidManifestEditorIconWidget(page,
                                                               landscapeSplashSize,
                                                               landscapeDisplaySize,
                                                               SplashIconContainerWidget::tr("Landscape splash screen"),
                                                               SplashIconContainerWidget::tr("Select landscape splash screen image"),
                                                               textEditorWidget, landscapeSplashPath);
    iconLayout->addWidget(landscapeButton);
    landscapeButtonContainer.push_back(landscapeButton);
    iconLayout->addStretch(1);

    iconLayout->addStretch(6);

    return page;
}


SplashIconContainerWidget::SplashIconContainerWidget(
        QWidget *parent,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    m_stickyCheck = new QCheckBox(tr("Sticky splash screen"), this);
    m_stickyCheck->setToolTip(tr("A non-sticky splash screen is hidden automatically when an activity is drawn.\n"
                                 "To hide a sticky splash screen, invoke QtAndroid::hideSplashScreen()."));
    QTabWidget *tab = new QTabWidget(this);

    auto hdpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               highDpiImageSize,
                               highDpiImageSize,
                               highDpiLandscapeImageSize,
                               highDpiImagePath, highDpiPortraitImagePath, highDpiLandscapeImagePath);
    tab->addTab(hdpiPage, tr("High DPI splash screen"));
    auto mdpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               mediumDpiImageSize,
                               mediumDpiImageSize,
                               mediumDpiLandscapeImageSize,
                               mediumDpiImagePath, mediumDpiPortraitImagePath, mediumDpiLandscapeImagePath);
    tab->addTab(mdpiPage, tr("Medium DPI splash screen"));
    auto ldpiPage = createPage(textEditorWidget,
                               m_imageButtons, m_portraitImageButtons, m_landscapeImageButtons,
                               lowDpiImageSize,
                               lowDpiImageSize,
                               lowDpiLandscapeImageSize,
                               lowDpiImagePath, lowDpiPortraitImagePath, lowDpiLandscapeImagePath);
    tab->addTab(ldpiPage, tr("Low DPI splash screen"));
    layout->addWidget(m_stickyCheck);
    layout->setAlignment(m_stickyCheck, Qt::AlignLeft);
    layout->addWidget(tab);
    for (auto &&imageButton : m_imageButtons)
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::splashScreensModified);
    for (auto &&imageButton : m_portraitImageButtons) {
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::splashScreensModified);
        connect(imageButton, &AndroidManifestEditorIconWidget::iconRemoved,
                this, &SplashIconContainerWidget::splashScreensModified);
    }
    for (auto &&imageButton : m_landscapeImageButtons) {
        connect(imageButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, &SplashIconContainerWidget::splashScreensModified);
        connect(imageButton, &AndroidManifestEditorIconWidget::iconRemoved,
                this, &SplashIconContainerWidget::splashScreensModified);
    }
    connect(m_stickyCheck, &QCheckBox::stateChanged, [this](int state) {
        bool old = m_splashScreenSticky;
        m_splashScreenSticky = (state == Qt::Checked);
        if (old != m_splashScreenSticky)
            splashScreensModified();
    });
}

void SplashIconContainerWidget::loadImages()
{
    for (auto &&imageButton : m_imageButtons)
        imageButton->loadIcon();
    for (auto &&imageButton : m_portraitImageButtons)
        imageButton->loadIcon();
    for (auto &&imageButton : m_landscapeImageButtons)
        imageButton->loadIcon();
}

bool SplashIconContainerWidget::hasImages()
{
    for (auto &&iconButton : m_imageButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

bool SplashIconContainerWidget::hasPortraitImages()
{
    for (auto &&iconButton : m_portraitImageButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

bool SplashIconContainerWidget::hasLandscapeImages()
{
    for (auto &&iconButton : m_landscapeImageButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

bool SplashIconContainerWidget::isSticky()
{
    return m_splashScreenSticky;
}

void SplashIconContainerWidget::setSticky(bool sticky)
{
    m_splashScreenSticky = sticky;
    m_stickyCheck->setCheckState(m_splashScreenSticky ? Qt::Checked : Qt::Unchecked);
}

} // namespace Internal
} // namespace Android
