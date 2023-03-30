// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rewritertransaction.h"
#include "textureeditorview.h"

namespace QmlDesigner {

class TextureEditorTransaction : public QObject
{
    Q_OBJECT

public:
    TextureEditorTransaction(TextureEditorView *textureEditor);

    Q_INVOKABLE void start();
    Q_INVOKABLE void end();

    Q_INVOKABLE bool active() const;

protected:
     void timerEvent(QTimerEvent *event) override;

private:
    TextureEditorView *m_textureEditor = nullptr;
    RewriterTransaction m_rewriterTransaction;
    int m_timerId = -1;
};

} // namespace QmlDesigner
