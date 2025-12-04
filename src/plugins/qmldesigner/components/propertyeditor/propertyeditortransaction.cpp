// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditortransaction.h"

#include "propertyeditortracing.h"

#include <QTimerEvent>

#include <QDebug>

namespace QmlDesigner {

using QmlDesigner::PropertyEditorTracing::category;

PropertyEditorTransaction::PropertyEditorTransaction(QmlDesigner::PropertyEditorView *propertyEditor) : QObject(propertyEditor), m_propertyEditor(propertyEditor), m_timerId(-1)
{
    NanotraceHR::Tracer tracer{"property editor transaction constructor", category()};
}

void PropertyEditorTransaction::start()
{
    NanotraceHR::Tracer tracer{"property editor transaction start", category()};

    if (!m_propertyEditor->model())
        return;
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
    m_rewriterTransaction = m_propertyEditor->beginRewriterTransaction(QByteArrayLiteral("PropertyEditorTransaction::start"));
    m_timerId = startTimer(10000);
}

void PropertyEditorTransaction::end()
{
    NanotraceHR::Tracer tracer{"property editor transaction end", category()};

    if (m_rewriterTransaction.isValid() && m_propertyEditor->model()) {
        killTimer(m_timerId);
        m_rewriterTransaction.commit();
    }
}

bool PropertyEditorTransaction::active() const
{
    NanotraceHR::Tracer tracer{"property editor transaction active", category()};

    return m_rewriterTransaction.isValid();
}

void PropertyEditorTransaction::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() != m_timerId)
        return;

    NanotraceHR::Tracer tracer{"property editor transaction timer event", category()};

    killTimer(timerEvent->timerId());
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
}

} //QmlDesigner

