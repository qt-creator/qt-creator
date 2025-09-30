// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewritertransaction.h"
#include "rewritertracing.h"
#include "rewriterview.h"
#include "rewritingexception.h"

#include <externaldependenciesinterface.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QDebug>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using RewriterTracing::category;

QList<QByteArray> RewriterTransaction::m_identifierList;
bool RewriterTransaction::m_activeIdentifier = Utils::qtcEnvironmentVariableIsSet(
    "QML_DESIGNER_TRACE_REWRITER_TRANSACTION");

RewriterTransaction::RewriterTransaction() : m_valid(false)
{
    NanotraceHR::Tracer tracer{"rewriter view transaction default constructor", category()};
}

RewriterTransaction::RewriterTransaction(AbstractView *_view, const QByteArray &identifier)
    : m_view(_view),
      m_identifier(identifier),
      m_valid(true)
{
    NanotraceHR::Tracer tracer{"rewriter view transaction constructor",
                               category(),
                               keyValue("identifier", identifier)};

    static int identifierNumber = 0;
    m_identifierNumber = identifierNumber++;

    if (m_activeIdentifier) {
        qDebug() << "Begin RewriterTransaction:" << m_identifier << m_identifierNumber;
        m_identifierList.append(m_identifier + QByteArrayLiteral("-") + QByteArray::number(m_identifierNumber));
    }

    beginTransaction();
}

RewriterTransaction::~RewriterTransaction()
{
    NanotraceHR::Tracer tracer{"rewriter view transaction destructor",
                               category(),
                               keyValue("identifier", m_identifier),
                               keyValue("number", m_identifierNumber),
                               keyValue("valid", m_valid)};
    try {
        commit();
    } catch (const RewritingException &e) {
        tracer.tick("catch exception");
        QTC_CHECK(false);
        e.showException();
    }
}

bool RewriterTransaction::isValid() const
{
    NanotraceHR::Tracer tracer{"rewriter view transaction isValid", category()};

    return m_valid;
}

void RewriterTransaction::ignoreSemanticChecks()
{
    NanotraceHR::Tracer tracer{"rewriter view transaction ignoreSemanticChecks", category()};

    m_ignoreSemanticChecks = true;
}

void RewriterTransaction::beginTransaction()
{
    NanotraceHR::Tracer tracer{"rewriter view transaction beginTransaction", category()};

    if (m_view && m_view->isAttached())
        m_view->model()->emitRewriterBeginTransaction();
}

void RewriterTransaction::endTransaction()
{
    NanotraceHR::Tracer tracer{"rewriter view transaction endTransaction", category()};

    if (m_view && m_view->isAttached())
        m_view->model()->emitRewriterEndTransaction();
}

void RewriterTransaction::commit()
{
    NanotraceHR::Tracer tracer{"rewriter view transaction commit",
                               category(),
                               keyValue("identifier", m_identifier),
                               keyValue("number", m_identifierNumber),
                               keyValue("valid", m_valid)};
    if (m_valid) {
        m_valid = false;

        RewriterView *rewriterView = m_view->rewriterView();

        QTC_ASSERT(rewriterView, qWarning() << Q_FUNC_INFO << "No rewriter attached");

        bool oldSemanticChecks = false;
        if (rewriterView) {
            oldSemanticChecks = rewriterView->checkSemanticErrors();
            if (m_ignoreSemanticChecks)
                rewriterView->setCheckSemanticErrors(false);
        }

        endTransaction();

        if (rewriterView)
            m_view->rewriterView()->setCheckSemanticErrors(oldSemanticChecks);

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
    NanotraceHR::Tracer tracer{"rewriter view transaction rollback",
                               category(),
                               keyValue("identifier", m_identifier),
                               keyValue("number", m_identifierNumber),
                               keyValue("valid", m_valid)};

    // TODO: should be implemented with a function in the rewriter
    if (m_valid) {
        m_valid = false;
        endTransaction();

        m_view->externalDependencies().undoOnCurrentDesignDocument();
        if (m_activeIdentifier) {
            qDebug() << "Rollback RewriterTransaction:" << m_identifier << m_identifierNumber;
            m_identifierList.removeOne(m_identifier + QByteArrayLiteral("-") + QByteArray::number(m_identifierNumber));
       }
    }
}

RewriterTransaction::RewriterTransaction(const RewriterTransaction &other)
        : m_valid(false)
{
    NanotraceHR::Tracer tracer{"rewriter view transaction copy constructor", category()};

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
    NanotraceHR::Tracer tracer{"rewriter view transaction assignment operator", category()};

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

