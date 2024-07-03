// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "editorview.h"

#include <QPointer>

namespace Core::Internal {

class EditorArea : public SplitterOrView
{
    Q_OBJECT

public:
    EditorArea();
    ~EditorArea() override;

    IDocument *currentDocument() const;
    EditorView *currentView() const;

signals:
    void windowTitleNeedsUpdate();
    void hidden();

private:
    void focusChanged(QWidget *old, QWidget *now);
    void setCurrentView(EditorView *view);
    void updateCurrentEditor(IEditor *editor);
    void updateCloseSplitButton();
    void hideEvent(QHideEvent *) override;

    QPointer<EditorView> m_currentView;
    QPointer<IDocument> m_currentDocument;
};

} // Core::Internal
