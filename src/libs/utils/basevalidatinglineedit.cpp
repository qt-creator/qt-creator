/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "basevalidatinglineedit.h"

#include <QtCore/QDebug>

enum { debug = 0 };

namespace Core {
namespace Utils {

struct BaseValidatingLineEditPrivate {
    explicit BaseValidatingLineEditPrivate(const QWidget *w);

    const QColor m_okTextColor;
    QColor m_errorTextColor;

    BaseValidatingLineEdit::State m_state;
    QString m_errorMessage;
    QString m_initialText;
    bool m_firstChange;
};

BaseValidatingLineEditPrivate::BaseValidatingLineEditPrivate(const QWidget *w) :
    m_okTextColor(BaseValidatingLineEdit::textColor(w)),
    m_errorTextColor(Qt::red),
    m_state(BaseValidatingLineEdit::Invalid),
    m_firstChange(true)
{
}

BaseValidatingLineEdit::BaseValidatingLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_bd(new BaseValidatingLineEditPrivate(this))
{
    // Note that textChanged() is also triggered automagically by
    // QLineEdit::setText(), no need to trigger manually.
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(slotChanged(QString)));
}

BaseValidatingLineEdit::~BaseValidatingLineEdit()
{
    delete m_bd;
}

QString BaseValidatingLineEdit::initialText() const
{
    return m_bd->m_initialText;
}

void BaseValidatingLineEdit::setInitialText(const QString &t)
{
    if (m_bd->m_initialText != t) {
        m_bd->m_initialText = t;
        m_bd->m_firstChange = true;
        setText(t);
    }
}

QColor BaseValidatingLineEdit::errorColor() const
{
    return m_bd->m_errorTextColor;
}

void BaseValidatingLineEdit::setErrorColor(const  QColor &c)
{
     m_bd->m_errorTextColor = c;
}

QColor BaseValidatingLineEdit::textColor(const QWidget *w)
{
    return w->palette().color(QPalette::Active, QPalette::Text);
}

void BaseValidatingLineEdit::setTextColor(QWidget *w, const QColor &c)
{
    QPalette palette = w->palette();
    palette.setColor(QPalette::Active, QPalette::Text, c);
    w->setPalette(palette);
}

BaseValidatingLineEdit::State BaseValidatingLineEdit::state() const
{
    return m_bd->m_state;
}

bool BaseValidatingLineEdit::isValid() const
{
    return m_bd->m_state == Valid;
}

QString BaseValidatingLineEdit::errorMessage() const
{
    return m_bd->m_errorMessage;
}

void BaseValidatingLineEdit::slotChanged(const QString &t)
{
    m_bd->m_errorMessage.clear();
    // Are we displaying the initial text?
    const bool isDisplayingInitialText = !m_bd->m_initialText.isEmpty() && t == m_bd->m_initialText;
    const State newState = isDisplayingInitialText ?
                               DisplayingInitialText :
                               (validate(t, &m_bd->m_errorMessage) ? Valid : Invalid);
    setToolTip(m_bd->m_errorMessage);
    if (debug)
        qDebug() << Q_FUNC_INFO << t << "State" <<  m_bd->m_state << "->" << newState << m_bd->m_errorMessage;
    // Changed..figure out if valid changed. DisplayingInitialText is not valid,
    // but should not show error color. Also trigger on the first change.
    if (newState != m_bd->m_state || m_bd->m_firstChange) {
        const bool validHasChanged = (m_bd->m_state == Valid) != (newState == Valid);
        m_bd->m_state = newState;
        m_bd->m_firstChange = false;
        setTextColor(this, newState == Invalid ? m_bd->m_errorTextColor : m_bd->m_okTextColor);
        if (validHasChanged)
            emit validChanged();
    }
}

void BaseValidatingLineEdit::slotReturnPressed()
{
    if (isValid())
        emit validReturnPressed();
}

} // namespace Utils
} // namespace Core
