// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidmanifesteditoriconcontainerwidget.h"
#include "androidmanifesteditoriconwidget.h"
#include "androidtr.h"

#include <utils/utilsicons.h>

#include <QFrame>
#include <QHBoxLayout>

using namespace Utils;

namespace Android {
namespace Internal {

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

AndroidManifestEditorIconContainerWidget::AndroidManifestEditorIconContainerWidget(
        QWidget *parent,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QWidget(parent)
{
    auto iconLayout = new QHBoxLayout(this);
    auto masterIconButton = new AndroidManifestEditorIconWidget(this,
                                                                lowDpiIconSize,
                                                                lowDpiIconSize,
                                                                Tr::tr("Master icon"),
                                                                Tr::tr("Select master icon."));
    masterIconButton->setIcon(Icon::fromTheme("document-open"));
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
                        Tr::tr("LDPI icon"),
                        Tr::tr("Select an icon suitable for low-density (ldpi) screens (~120dpi)."),
                        textEditorWidget,
                        lowDpiIconPath,
                        iconFileName);
    iconLayout->addWidget(lIconButton);
    m_iconButtons.push_back(lIconButton);
    iconLayout->addStretch(1);

    auto mIconButton = new AndroidManifestEditorIconWidget(this,
                        mediumDpiIconSize,
                        mediumDpiIconSize,
                        Tr::tr("MDPI icon"),
                        Tr::tr("Select an icon for medium-density (mdpi) screens (~160dpi)."),
                        textEditorWidget,
                        mediumDpiIconPath,
                        iconFileName);
    iconLayout->addWidget(mIconButton);
    m_iconButtons.push_back(mIconButton);
    iconLayout->addStretch(1);

    auto hIconButton =  new AndroidManifestEditorIconWidget(this,
                         highDpiIconSize,
                         highDpiIconSize,
                         Tr::tr("HDPI icon"),
                         Tr::tr("Select an icon for high-density (hdpi) screens (~240dpi)."),
                         textEditorWidget,
                         highDpiIconPath,
                         iconFileName);
    iconLayout->addWidget(hIconButton);
    m_iconButtons.push_back(hIconButton);
    iconLayout->addStretch(1);

    auto xhIconButton =  new AndroidManifestEditorIconWidget(this,
                         extraHighDpiIconSize,
                         extraHighDpiIconSize,
                         Tr::tr("XHDPI icon"),
                         Tr::tr("Select an icon for extra-high-density (xhdpi) screens (~320dpi)."),
                         textEditorWidget,
                         extraHighDpiIconPath,
                         iconFileName);
    iconLayout->addWidget(xhIconButton);
    m_iconButtons.push_back(xhIconButton);
    iconLayout->addStretch(1);

    auto xxhIconButton =  new AndroidManifestEditorIconWidget(this,
                     extraExtraHighDpiIconSize,
                     extraExtraHighDpiIconSize,
                     Tr::tr("XXHDPI icon"),
                     Tr::tr("Select an icon for extra-extra-high-density (xxhdpi) screens (~480dpi)."),
                     textEditorWidget,
                     extraExtraHighDpiIconPath,
                     iconFileName);
    iconLayout->addWidget(xxhIconButton);
    m_iconButtons.push_back(xxhIconButton);
    iconLayout->addStretch(1);

    auto xxxhIconButton =  new AndroidManifestEditorIconWidget(this,
             extraExtraExtraHighDpiIconSize,
             extraExtraExtraHighDpiIconSize,
             Tr::tr("XXXHDPI icon"),
             Tr::tr("Select an icon for extra-extra-extra-high-density (xxxhdpi) screens (~640dpi)."),
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
