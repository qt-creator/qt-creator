// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "materialeditortransaction.h"

#include <QTimerEvent>

#include <QDebug>

namespace QmlDesigner {

MaterialEditorTransaction::MaterialEditorTransaction(QmlDesigner::MaterialEditorView *materialEditor)
    : QObject(materialEditor),
      m_materialEditor(materialEditor)
{
}

void MaterialEditorTransaction::start()
{
    if (!m_materialEditor->model())
        return;
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
    m_rewriterTransaction = m_materialEditor->beginRewriterTransaction(QByteArrayLiteral("MaterialEditorTransaction::start"));
    m_timerId = startTimer(10000);
}

void MaterialEditorTransaction::end()
{
    if (m_rewriterTransaction.isValid() &&  m_materialEditor->model()) {
        killTimer(m_timerId);
        m_rewriterTransaction.commit();
    }
}

bool MaterialEditorTransaction::active() const
{
    return m_rewriterTransaction.isValid();
}

void MaterialEditorTransaction::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() != m_timerId)
        return;
    killTimer(timerEvent->timerId());
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
}

} // namespace QmlDesigner

