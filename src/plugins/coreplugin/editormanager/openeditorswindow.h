// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "editorview.h"

#include <QFrame>
#include <QList>

namespace Core::Internal {

class OpenEditorsView;

class OpenEditorsWindow final : public QFrame
{
public:
    explicit OpenEditorsWindow(QWidget *parent = nullptr);

    void setEditors(const QList<EditLocation> &globalHistory, EditorView *view);

    void selectNextEditor();
    void selectPreviousEditor();
    void selectAndHide();

    QSize sizeHint() const final;
    void setVisible(bool visible) final;

private:
    bool eventFilter(QObject *src, QEvent *e) final;
    void focusInEvent(QFocusEvent *) final;

    OpenEditorsView *m_editorView;
};

} // Core::Internal
