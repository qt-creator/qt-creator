// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorstatusbar.h"

#include <utils/theme/theme.h>

namespace QmlDesigner {

TextEditorStatusBar::TextEditorStatusBar(QWidget *parent) : QToolBar(parent), m_label(new QLabel(this))
{
    QWidget *spacer = new QWidget(this);
    spacer->setMinimumWidth(50);
    addWidget(spacer);
    addWidget(m_label);

    /* We have to set another .css, since the central widget has already a style sheet */
    m_label->setStyleSheet(QString("QLabel { color :%1 }").arg(Utils::creatorTheme()->color(Utils::Theme::TextColorError).name()));
}

void TextEditorStatusBar::clearText()
{
    m_label->clear();
}

void TextEditorStatusBar::setText(const QString &text)
{
    m_label->setText(text);
}

} // namespace QmlDesigner

