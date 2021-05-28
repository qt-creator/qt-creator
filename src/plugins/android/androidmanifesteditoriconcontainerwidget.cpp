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

#include "androidmanifesteditoriconcontainerwidget.h"
#include "androidmanifesteditoriconwidget.h"

#include <utils/utilsicons.h>

#include <QFrame>
#include <QHBoxLayout>

namespace Android {
namespace Internal {

namespace {
const char extraExtraExtraHighDpiIconPath[] = "/res/drawable-xxxhdpi/";
const char extraExtraHighDpiIconPath[] = "/res/drawable-xxhdpi/";
const char extraHighDpiIconPath[] = "/res/drawable-xhdpi/";
const char highDpiIconPath[] = "/res/drawable-hdpi/";
const char mediumDpiIconPath[] = "/res/drawable-mdpi/";
const char lowDpiIconPath[] = "/res/drawable-ldpi/";
const char imageSuffix[] = ".png";
const QSize lowDpiIconSize{32, 32};
const QSize mediumDpiIconSize{48, 48};
const QSize highDpiIconSize{72, 72};
const QSize extraHighDpiIconSize{96, 96};
const QSize extraExtraHighDpiIconSize{144, 144};
const QSize extraExtraExtraHighDpiIconSize{192, 192};
}

AndroidManifestEditorIconContainerWidget::AndroidManifestEditorIconContainerWidget(
        QWidget *parent,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QWidget(parent)
{
    auto iconLayout = new QHBoxLayout(this);
    auto masterIconButton = new AndroidManifestEditorIconWidget(this,
                                                                lowDpiIconSize,
                                                                lowDpiIconSize,
                                                                tr("Master icon"), tr("Select master icon."));
    masterIconButton->setIcon(QIcon::fromTheme(QLatin1String("document-open"), Utils::Icons::OPENFILE.icon()));
    iconLayout->addWidget(masterIconButton);
    iconLayout->addStretch(1);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    iconLayout->addWidget(line);
    iconLayout->addStretch(1);

    QString iconFileName = m_iconFileName + imageSuffix;

    auto lIconButton = new AndroidManifestEditorIconWidget(this,
                        lowDpiIconSize,
                        lowDpiIconSize,
                        tr("LDPI icon"),
                        tr("Select an icon suitable for low-density (ldpi) screens (~120dpi)."),
                        textEditorWidget,
                        lowDpiIconPath,
                        iconFileName);
    iconLayout->addWidget(lIconButton);
    m_iconButtons.push_back(lIconButton);
    iconLayout->addStretch(1);

    auto mIconButton = new AndroidManifestEditorIconWidget(this,
                        mediumDpiIconSize,
                        mediumDpiIconSize,
                        tr("MDPI icon"),
                        tr("Select an icon for medium-density (mdpi) screens (~160dpi)."),
                        textEditorWidget,
                        mediumDpiIconPath,
                        iconFileName);
    iconLayout->addWidget(mIconButton);
    m_iconButtons.push_back(mIconButton);
    iconLayout->addStretch(1);

    auto hIconButton =  new AndroidManifestEditorIconWidget(this,
                         highDpiIconSize,
                         highDpiIconSize,
                         tr("HDPI icon"),
                         tr("Select an icon for high-density (hdpi) screens (~240dpi)."),
                         textEditorWidget,
                         highDpiIconPath,
                         iconFileName);
    iconLayout->addWidget(hIconButton);
    m_iconButtons.push_back(hIconButton);
    iconLayout->addStretch(1);

    auto xhIconButton =  new AndroidManifestEditorIconWidget(this,
                         extraHighDpiIconSize,
                         extraHighDpiIconSize,
                         tr("XHDPI icon"),
                         tr("Select an icon for extra-high-density (xhdpi) screens (~320dpi)."),
                         textEditorWidget,
                         extraHighDpiIconPath,
                         iconFileName);
    iconLayout->addWidget(xhIconButton);
    m_iconButtons.push_back(xhIconButton);
    iconLayout->addStretch(1);

    auto xxhIconButton =  new AndroidManifestEditorIconWidget(this,
                     extraExtraHighDpiIconSize,
                     extraExtraHighDpiIconSize,
                     tr("XXHDPI icon"),
                     tr("Select an icon for extra-extra-high-density (xxhdpi) screens (~480dpi)."),
                     textEditorWidget,
                     extraExtraHighDpiIconPath,
                     iconFileName);
    iconLayout->addWidget(xxhIconButton);
    m_iconButtons.push_back(xxhIconButton);
    iconLayout->addStretch(1);

    auto xxxhIconButton =  new AndroidManifestEditorIconWidget(this,
             extraExtraExtraHighDpiIconSize,
             extraExtraExtraHighDpiIconSize,
             tr("XXXHDPI icon"),
             tr("Select an icon for extra-extra-extra-high-density (xxxhdpi) screens (~640dpi)."),
             textEditorWidget,
             extraExtraExtraHighDpiIconPath,
             iconFileName);
    iconLayout->addWidget(xxxhIconButton);
    m_iconButtons.push_back(xxxhIconButton);
    iconLayout->addStretch(3);

    auto handleIconModification = [this] {
        bool iconsMaybeChanged = hasIcons();
        if (m_hasIcons != iconsMaybeChanged)
            emit iconsModified();
        m_hasIcons = iconsMaybeChanged;
    };
    for (auto &&iconButton : m_iconButtons) {
        connect(masterIconButton, &AndroidManifestEditorIconWidget::iconSelected,
                iconButton, &AndroidManifestEditorIconWidget::setIconFromPath);
        connect(iconButton, &AndroidManifestEditorIconWidget::iconRemoved,
                this, handleIconModification);
        connect(iconButton, &AndroidManifestEditorIconWidget::iconSelected,
                this, handleIconModification);
    }
    connect(masterIconButton, &AndroidManifestEditorIconWidget::iconSelected,
            this, handleIconModification);
}

void AndroidManifestEditorIconContainerWidget::setIconFileName(const QString &name)
{
    m_iconFileName = name;
}

QString AndroidManifestEditorIconContainerWidget::iconFileName() const
{
    return m_iconFileName;
}

void AndroidManifestEditorIconContainerWidget::loadIcons()
{
    for (auto &&iconButton : m_iconButtons) {
        iconButton->setTargetIconFileName(m_iconFileName + imageSuffix);
        iconButton->loadIcon();
    }
    m_hasIcons = hasIcons();
}

bool AndroidManifestEditorIconContainerWidget::hasIcons() const
{
    for (auto &&iconButton : m_iconButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

} // namespace Internal
} // namespace Android
