/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/
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
