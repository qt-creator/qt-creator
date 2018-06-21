/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "rewritertransaction.h"
#include <abstractview.h>
#include <rewritingexception.h>
#include <rewriterview.h>

#include <utils/qtcassert.h>

#ifndef QMLDESIGNER_TEST
#include <designdocument.h>
#include <qmldesignerplugin.h>
#endif

#include <QDebug>

namespace QmlDesigner {



QList<QByteArray> RewriterTransaction::m_identifierList;
bool RewriterTransaction::m_activeIdentifier = qEnvironmentVariableIsSet("QML_DESIGNER_TRACE_REWRITER_TRANSACTION");

RewriterTransaction::RewriterTransaction() : m_valid(false)
{
}

RewriterTransaction::RewriterTransaction(AbstractView *_view, const QByteArray &identifier)
    : m_view(_view),
      m_identifier(identifier),
      m_valid(true)
{
    Q_ASSERT(view());

    static int identifierNumber = 0;
    m_identifierNumber = identifierNumber++;

    if (m_activeIdentifier) {
        qDebug() << "Begin RewriterTransaction:" << m_identifier << m_identifierNumber;
        m_identifierList.append(m_identifier + QByteArrayLiteral("-") + QByteArray::number(m_identifierNumber));
    }

    view()->emitRewriterBeginTransaction();
}

RewriterTransaction::~RewriterTransaction()
{
    try {
        commit();
    } catch (const RewritingException &e) {
        QTC_ASSERT(false, ;);
        e.showException();
    }
}

bool RewriterTransaction::isValid() const
{
    return m_valid;
}

void RewriterTransaction::ignoreSemanticChecks()
{
    m_ignoreSemanticChecks = true;
}

void RewriterTransaction::commit()
{
    if (m_valid) {
        m_valid = false;

        RewriterView *rewriterView = view()->rewriterView();

        QTC_ASSERT(rewriterView, qWarning() << Q_FUNC_INFO << "No rewriter attached");

        bool oldSemanticChecks = false;
        if (rewriterView) {
            oldSemanticChecks = rewriterView->checkSemanticErrors();
            if (m_ignoreSemanticChecks)
                rewriterView->setCheckSemanticErrors(false);
        }

        view()->emitRewriterEndTransaction();

        if (rewriterView)
            view()->rewriterView()->setCheckSemanticErrors(oldSemanticChecks);

        if (m_activeIdentifier) {
            qDebug() << "Commit RewriterTransaction:" << m_identifier << m_identifierNumber;
            bool success = m_identifierList.removeOne(m_identifier + QByteArrayLiteral("-") + QByteArray::number(m_identifierNumber));
            Q_UNUSED(success);
            Q_ASSERT(success);
        }
    }
}

void RewriterTransaction::rollback()
{
    // TODO: should be implemented with a function in the rewriter
    if (m_valid) {
        m_valid = false;
        view()->emitRewriterEndTransaction();

#ifndef QMLDESIGNER_TEST
        QmlDesignerPlugin::instance()->currentDesignDocument()->undo();
#endif
        if (m_activeIdentifier) {
            qDebug() << "Rollback RewriterTransaction:" << m_identifier << m_identifierNumber;
            m_identifierList.removeOne(m_identifier + QByteArrayLiteral("-") + QByteArray::number(m_identifierNumber));
       }
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
        m_identifier = other.m_identifier;
        m_identifierNumber = other.m_identifierNumber;
        other.m_valid = false;
    }
}

RewriterTransaction& RewriterTransaction::operator=(const RewriterTransaction &other)
{
    if (!m_valid && (&other != this)) {
        m_valid = other.m_valid;
        m_view = other.m_view;
        m_identifier = other.m_identifier;
        m_identifierNumber = other.m_identifierNumber;
        other.m_valid = false;
    }

    return *this;
}

} //QmlDesigner

