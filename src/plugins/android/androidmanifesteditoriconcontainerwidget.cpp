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
const QString highDpiIconPath = "/res/drawable-hdpi/icon.png";
const QString mediumDpiIconPath = "/res/drawable-mdpi/icon.png";
const QString lowDpiIconPath = "/res/drawable-ldpi/icon.png";
const QSize lowDpiIconSize{32, 32};
const QSize mediumDpiIconSize{48, 48};
const QSize highDpiIconSize{72, 72};
}

AndroidManifestEditorIconContainerWidget::AndroidManifestEditorIconContainerWidget(
        QWidget *parent,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QWidget(parent)
{
    auto iconLayout = new QHBoxLayout(this);
    auto masterIconButton = new AndroidManifestEditorIconWidget(this,
                                                                lowDpiIconSize,
                                                                tr("Master icon"), tr("Select master icon"));
    masterIconButton->setIcon(QIcon::fromTheme(QLatin1String("document-open"), Utils::Icons::OPENFILE.icon()));
    iconLayout->addWidget(masterIconButton);
    iconLayout->addStretch(1);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    iconLayout->addWidget(line);
    iconLayout->addStretch(1);

    auto lIconButton = new AndroidManifestEditorIconWidget(this,
                                                        lowDpiIconSize,
                                                        tr("Low DPI icon"), tr("Select low DPI icon"),
                                                        textEditorWidget, lowDpiIconPath);
    iconLayout->addWidget(lIconButton);
    m_iconButtons.push_back(lIconButton);
    iconLayout->addStretch(1);

    auto mIconButton = new AndroidManifestEditorIconWidget(this,
                                                        mediumDpiIconSize,
                                                        tr("Medium DPI icon"), tr("Select medium DPI icon"),
                                                        textEditorWidget, mediumDpiIconPath);
    iconLayout->addWidget(mIconButton);
    m_iconButtons.push_back(mIconButton);
    iconLayout->addStretch(1);

    auto hIconButton =  new AndroidManifestEditorIconWidget(this,
                                                         highDpiIconSize,
                                                         tr("High DPI icon"), tr("Select high DPI icon"),
                                                         textEditorWidget, highDpiIconPath);
    iconLayout->addWidget(hIconButton);
    m_iconButtons.push_back(hIconButton);
    iconLayout->addStretch(6);

    for (auto &&iconButton : m_iconButtons) {
        connect(masterIconButton, &AndroidManifestEditorIconWidget::iconSelected,
                iconButton, &AndroidManifestEditorIconWidget::setIconFromPath);
    }
}

void AndroidManifestEditorIconContainerWidget::loadIcons()
{
    for (auto &&iconButton : m_iconButtons)
        iconButton->loadIcon();
}

bool AndroidManifestEditorIconContainerWidget::hasIcons()
{
    for (auto &&iconButton : m_iconButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

} // namespace Internal
} // namespace Android
