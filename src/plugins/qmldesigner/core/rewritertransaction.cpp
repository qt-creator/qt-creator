/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "rewritertransaction.h"
#include <abstractview.h>

namespace QmlDesigner {


RewriterTransaction::RewriterTransaction() : m_valid(false)
{
}

RewriterTransaction::RewriterTransaction(AbstractView *_view) : m_view(_view), m_valid(true)
{
    Q_ASSERT(view());
    view()->emitCustomNotification("__start rewriter transaction__");
}

RewriterTransaction::~RewriterTransaction()
{
    commit();
}

bool RewriterTransaction::isValid() const
{
    return m_valid;
}

void RewriterTransaction::commit()
{
    if (m_valid) {
        m_valid = false;
        view()->emitCustomNotification("__end rewriter transaction__");
    }
}

AbstractView *RewriterTransaction::view()
{
    return m_view.data();
}

RewriterTransaction::RewriterTransaction(const RewriterTransaction &other)
        : m_valid(false)
{
    if (&other != this) {
        m_valid = other.m_valid;
        m_view = other.m_view;
        other.m_valid = false;
    }
}

RewriterTransaction& RewriterTransaction::operator=(const RewriterTransaction &other)
{
    if (!m_valid && (&other != this)) {
        m_valid = other.m_valid;
        m_view = other.m_view;
        other.m_valid = false;
    }

    return *this;
}

} //QmlDesigner

