// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
