// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "textdocument.h"
#include "texteditor.h"

#include <utils/action.h>
#include <utils/id.h>

QT_BEGIN_NAMESPACE
class QTextBrowser;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal { class EditorWithPreviewPrivate; }

class TEXTEDITOR_EXPORT EditorWithPreview : public BaseTextEditor
{
    Q_OBJECT

public:
    EditorWithPreview(const TextDocumentPtr &document,
                      TextEditorWidget *textEditorWidget,
                      QTextBrowser *previewWidget,
                      const QByteArray &idPrefix);
    ~EditorWithPreview() override;

    static Utils::Id editorId(const QByteArray &idPrefix);
    static Utils::Id textContextId(const QByteArray &idPrefix);

    void updatePreview();
    void updatePreviewNow();

    void toggleEditor();
    void togglePreview();
    void swapViews();

    virtual void increasePreviewZoom();
    virtual void decreasePreviewZoom();
    virtual void resetPreviewZoom();

    void gotoLine(int line, int column = 0, bool centerLine = true) override;
    QByteArray saveState() const override;
    void restoreState(const QByteArray &state) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

protected:
    virtual void renderPreview() = 0;

    QAction *insertEditorToolBarWidget(TextEditorWidget::Side side, QWidget *widget);

    TextDocumentPtr textDocumentPtr() const;
    TextEditorWidget *textEditorWidget() const;
    QTextBrowser *previewWidget() const;

private:
    friend class Internal::EditorWithPreviewPrivate;
    Internal::EditorWithPreviewPrivate *d;
};

class TEXTEDITOR_EXPORT EditorWithPreviewActions
{
public:
    explicit EditorWithPreviewActions(const QByteArray &idPrefix);

private:
    Utils::Action m_toggleEditorAction;
    Utils::Action m_togglePreviewAction;
    Utils::Action m_swapViewsAction;
    Utils::Action m_previewZoomInAction;
    Utils::Action m_previewZoomOutAction;
    Utils::Action m_previewZoomResetAction;
};

} // namespace TextEditor
