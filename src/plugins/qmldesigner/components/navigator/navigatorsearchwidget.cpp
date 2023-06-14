// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorsearchwidget.h"

#include <utils/stylehelper.h>
#include <theme.h>

#include <QAction>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QStyle>
#include <QToolButton>

namespace QmlDesigner {

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    clearButton = new QToolButton(this);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const int clearIconSize = 10;
    const QColor iconColor(QmlDesigner::Theme::getColor(QmlDesigner::Theme::DStextColor));

    const QIcon searchIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName,
        QmlDesigner::Theme::getIconUnicode(QmlDesigner::Theme::Icon::search_small),
        10,
        16,
        iconColor);

    const QIcon clearIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName,
        QmlDesigner::Theme::getIconUnicode(QmlDesigner::Theme::Icon::close_small),
        clearIconSize,
        clearIconSize,
        iconColor);

    addAction(searchIcon, QLineEdit::LeadingPosition);

    clearButton->setIcon(clearIcon);
    clearButton->setIconSize(QSize(clearIconSize, clearIconSize));
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->hide();
    clearButton->setStyleSheet(
        Theme::replaceCssColors(QString("QToolButton { border: none; padding: 0px; }"
                                        "QToolButton:hover {}")));

    setClearButtonEnabled(false);

    connect(clearButton, &QToolButton::clicked, this, &QLineEdit::clear);
    connect(this, &QLineEdit::textChanged, this, &LineEdit::updateClearButton);

    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(Theme::replaceCssColors(
        QString("QLineEdit { padding-right: %1px; border-radius: 4;"
                "color: creatorTheme.DStextColor;"
                "border-color: creatorTheme.DScontrolOutline_topToolbarIdle;"
                "background: creatorTheme.DStoolbarBackground; }"
                "QLineEdit:hover {"
                "color: creatorTheme.DStextColor;"
                "border-color: creatorTheme.DScontrolOutline_topToolbarHover;"
                "background: creatorTheme.DScontrolBackground_toolbarHover; }"
                "QLineEdit:focus {"
                "color: creatorTheme.DStextColor;"
                "border-color: creatorTheme.DSinteraction;"
                "background: creatorTheme.DStoolbarBackground; }")
            .arg(clearButton->sizeHint().width() + frameWidth + 8)));

    setFixedHeight(29);
}

void LineEdit::resizeEvent([[maybe_unused]] QResizeEvent *event)
{
    QSize hint = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    clearButton->move(rect().right() - frameWidth - hint.width() - 3,
                      (rect().bottom() + 1 - hint.height()) / 2);
}

void LineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
        clear();
        event->accept();
        return;
    }
    QLineEdit::keyPressEvent(event);
}

void LineEdit::paintEvent(QPaintEvent *event)
{
    if (text().isEmpty()) {
        QPalette p(palette());
        p.setColor(QPalette::Active,
                   QPalette::PlaceholderText,
                   Utils::creatorTheme()->color(Utils::Theme::DSplaceholderTextColor));
        p.setColor(QPalette::Inactive,
                   QPalette::PlaceholderText,
                   Utils::creatorTheme()->color(Utils::Theme::DSplaceholderTextColor));
        setPalette(p);
    }
    QLineEdit::paintEvent(event);
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
    layout->setContentsMargins(10, 6, 10, 6);
    setLayout(layout);

    m_textField = new LineEdit;
    m_textField->setPlaceholderText(tr("Search"));
    m_textField->setFrame(false);

    connect(m_textField, &QLineEdit::textChanged, this, &NavigatorSearchWidget::textChanged);

    layout->addWidget(m_textField);

    setFixedHeight(Theme::toolbarSize());
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, Theme::getColor(Utils::Theme::DStoolbarBackground));
    setAutoFillBackground(true);
    setPalette(pal);
}

void NavigatorSearchWidget::clear()
{
    m_textField->clear();
}

} // QmlDesigner
