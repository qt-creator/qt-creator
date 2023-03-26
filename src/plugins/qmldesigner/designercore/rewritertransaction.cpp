// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewritertransaction.h"
#include <abstractview.h>
#include <externaldependenciesinterface.h>
#include <rewriterview.h>
#include <rewritingexception.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QDebug>

namespace QmlDesigner {

QList<QByteArray> RewriterTransaction::m_identifierList;
bool RewriterTransaction::m_activeIdentifier = Utils::qtcEnvironmentVariableIsSet(
    "QML_DESIGNER_TRACE_REWRITER_TRANSACTION");

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
            [[maybe_unused]] bool success = m_identifierList.removeOne(
                m_identifier + QByteArrayLiteral("-") + QByteArray::number(m_identifierNumber));
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

        view()->externalDependencies().undoOnCurrentDesignDocument();
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

