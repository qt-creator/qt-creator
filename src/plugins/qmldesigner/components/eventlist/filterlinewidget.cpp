/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "filterlinewidget.h"

#include <theme.h>
#include <utils/fancylineedit.h>
#include <utils/stylehelper.h>

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>

namespace QmlDesigner {

FilterLineWidget::FilterLineWidget(QWidget *parent)
    : QWidget(parent)
    , m_edit(new Utils::FancyLineEdit())
{
    const QString unicode = Theme::getIconUnicode(Theme::Icon::search);
    const QString fontName = "qtds_propertyIconFont.ttf";
    QIcon icon = Utils::StyleHelper::getIconFromIconFont(fontName, unicode, 28, 28);

    auto *label = new QLabel;
    label->setPixmap(icon.pixmap(QSize(18, 18)));
    label->setAlignment(Qt::AlignCenter);

    m_edit->setPlaceholderText(QObject::tr("<Filter>", "Library search input hint text"));
    m_edit->setDragEnabled(false);
    m_edit->setMinimumWidth(75);
    m_edit->setTextMargins(0, 0, 20, 0);
    m_edit->setFiltering(true);

    auto *box = new QHBoxLayout;
    box->addWidget(label);
    box->addWidget(m_edit);
    setLayout(box);

    connect(m_edit, &Utils::FancyLineEdit::filterChanged, this, &FilterLineWidget::filterChanged);
}

void FilterLineWidget::clear()
{
    m_edit->clear();
}

} // namespace QmlDesigner.
