/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "navigatorsearchwidget.h"

#include <utils/stylehelper.h>
#include <theme.h>

#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace QmlDesigner {

NavigatorSearchWidget::NavigatorSearchWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QBoxLayout(QBoxLayout::LeftToRight);
    setLayout(layout);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const int iconSize = 15;
    const QColor iconColor(QmlDesigner::Theme::getColor(QmlDesigner::Theme::IconsBaseColor));
    const QIcon searchIcon = Utils::StyleHelper::getIconFromIconFont(
                fontName, QmlDesigner::Theme::getIconUnicode(QmlDesigner::Theme::Icon::search),
                iconSize, iconSize, iconColor);

    m_textField = new QLineEdit;
    m_textField->setPlaceholderText(tr("Filter"));
    m_textField->setFrame(false);
    m_textField->setClearButtonEnabled(true);
    m_textField->addAction(searchIcon, QLineEdit::LeadingPosition);

    connect(m_textField, &QLineEdit::textChanged, this, &NavigatorSearchWidget::textChanged);

    layout->addWidget(m_textField);
}

void NavigatorSearchWidget::clear()
{
    m_textField->clear();
}

} // QmlDesigner
