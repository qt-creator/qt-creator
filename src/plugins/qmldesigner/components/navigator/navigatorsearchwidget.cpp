// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorsearchwidget.h"

#include <utils/stylehelper.h>
#include <theme.h>

#include <QAction>
#include <QBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QToolButton>

namespace QmlDesigner {

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    clearButton = new QToolButton(this);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const int searchIconSize = 16;
    const int clearIconSize = 12;
    const QColor iconColor(QmlDesigner::Theme::getColor(QmlDesigner::Theme::DSiconColor));

    const QIcon searchIcon
        = Utils::StyleHelper::getIconFromIconFont(fontName,
                                                  QmlDesigner::Theme::getIconUnicode(
                                                      QmlDesigner::Theme::Icon::search),
                                                  searchIconSize,
                                                  searchIconSize,
                                                  iconColor);

    const QIcon clearIcon
        = Utils::StyleHelper::getIconFromIconFont(fontName,
                                                  QmlDesigner::Theme::getIconUnicode(
                                                      QmlDesigner::Theme::Icon::closeCross),
                                                  clearIconSize,
                                                  clearIconSize,
                                                  iconColor);

    addAction(searchIcon, QLineEdit::LeadingPosition);

    clearButton->setIcon(clearIcon);
    clearButton->setIconSize(QSize(clearIconSize, clearIconSize));
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->hide();
    clearButton->setStyleSheet(Theme::replaceCssColors(
        QString("QToolButton { border: none; padding: 0px; }"
                "QToolButton:hover { background: creatorTheme.DScontrolBackgroundHover; }")));

    setClearButtonEnabled(false);

    connect(clearButton, &QToolButton::clicked, this, &QLineEdit::clear);
    connect(this, &QLineEdit::textChanged, this, &LineEdit::updateClearButton);

    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString("QLineEdit { padding-right: %1px; } ")
                      .arg(clearButton->sizeHint().width() + frameWidth + 8));

    setFixedHeight(29);
}

void LineEdit::resizeEvent(QResizeEvent *)
{
    QSize hint = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    clearButton->move(rect().right() - frameWidth - hint.width() - 3,
                      (rect().bottom() + 1 - hint.height()) / 2);
}

void LineEdit::updateClearButton(const QString& text)
{
    clearButton->setVisible(!text.isEmpty());
}

NavigatorSearchWidget::NavigatorSearchWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->setSpacing(0);
    layout->setContentsMargins(5, 5, 5, 3);
    setLayout(layout);

    m_textField = new LineEdit;
    m_textField->setPlaceholderText(tr("Search"));
    m_textField->setFrame(false);

    connect(m_textField, &QLineEdit::textChanged, this, &NavigatorSearchWidget::textChanged);

    layout->addWidget(m_textField);
}

void NavigatorSearchWidget::clear()
{
    m_textField->clear();
}

} // QmlDesigner
