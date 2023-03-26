// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lineeditaction.h"

#include <QLineEdit>
#include <QIntValidator>

namespace QmlDesigner {

LineEditAction::LineEditAction(const QString &placeHolderText, QObject *parent) :
    QWidgetAction(parent),
    m_placeHolderText(placeHolderText)
{
}

void LineEditAction::setLineEditText(const QString &text)
{
    emit lineEditTextChange(text);
}

void LineEditAction::clearLineEditText()
{
    emit lineEditTextClear();
}

QWidget *LineEditAction::createWidget(QWidget *parent)
{
    auto lineEdit = new QLineEdit(parent);

    lineEdit->setPlaceholderText(m_placeHolderText);
    lineEdit->setFixedWidth(100);
    QFont font = lineEdit->font();
    font.setPixelSize(Theme::instance()->smallFontPixelSize());
    lineEdit->setFont(font);
    lineEdit->setValidator(new QIntValidator(0, 99999, this));

    connect(lineEdit, &QLineEdit::textEdited, this, &LineEditAction::textChanged);
    connect(this, &LineEditAction::lineEditTextClear, lineEdit, &QLineEdit::clear);
    connect(this, &LineEditAction::lineEditTextChange, lineEdit, &QLineEdit::setText);

    return lineEdit;
}


} // namespace QmlDesigner
