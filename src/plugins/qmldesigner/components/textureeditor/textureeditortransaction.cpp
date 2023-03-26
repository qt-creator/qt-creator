// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textureeditortransaction.h"

#include <QTimerEvent>

namespace QmlDesigner {

TextureEditorTransaction::TextureEditorTransaction(TextureEditorView *textureEditor)
    : QObject(textureEditor),
      m_textureEditor(textureEditor)
{
}

void TextureEditorTransaction::start()
{
    if (!m_textureEditor->model())
        return;
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
    m_rewriterTransaction = m_textureEditor->beginRewriterTransaction(QByteArrayLiteral("MaterialEditorTransaction::start"));
    m_timerId = startTimer(10000);
}

void TextureEditorTransaction::end()
{
    if (m_rewriterTransaction.isValid() &&  m_textureEditor->model()) {
        killTimer(m_timerId);
        m_rewriterTransaction.commit();
    }
}

bool TextureEditorTransaction::active() const
{
    return m_rewriterTransaction.isValid();
}

void TextureEditorTransaction::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() != m_timerId)
        return;
    killTimer(timerEvent->timerId());
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
}

} // namespace QmlDesigner
