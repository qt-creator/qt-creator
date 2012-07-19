/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    QLineEdit *lineEdit = new QLineEdit(parent);

    lineEdit->setPlaceholderText(m_placeHolderText);
    lineEdit->setFixedWidth(80);
    lineEdit->setValidator(new QIntValidator(0, 4096, this));

    connect(lineEdit, SIGNAL(textEdited(QString)), this, SIGNAL(textChanged(QString)));
    connect(this, SIGNAL(lineEditTextClear()), lineEdit, SLOT(clear()));
    connect(this, SIGNAL(lineEditTextChange(QString)), lineEdit, SLOT(setText(QString)));

    return lineEdit;
}


} // namespace QmlDesigner
