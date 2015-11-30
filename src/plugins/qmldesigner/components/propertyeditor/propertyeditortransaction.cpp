/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "propertyeditortransaction.h"
#include <QTimerEvent>

#include <QDebug>

namespace QmlDesigner {

PropertyEditorTransaction::PropertyEditorTransaction(QmlDesigner::PropertyEditorView *propertyEditor) : QObject(propertyEditor), m_propertyEditor(propertyEditor), m_timerId(-1)
{
}

void PropertyEditorTransaction::start()
{
    if (!m_propertyEditor->model())
        return;
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
    m_rewriterTransaction = m_propertyEditor->beginRewriterTransaction(QByteArrayLiteral("PropertyEditorTransaction::start"));
    m_timerId = startTimer(4000);
}

void PropertyEditorTransaction::end()
{
    if (m_rewriterTransaction.isValid() &&  m_propertyEditor->model()) {
        killTimer(m_timerId);
        m_rewriterTransaction.commit();
    }
}

void PropertyEditorTransaction::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() != m_timerId)
        return;
    killTimer(timerEvent->timerId());
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
}

} //QmlDesigner

