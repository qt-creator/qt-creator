// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace QmlJSEditor {
class SemanticHighlighter;
}

namespace Core {
class IContext;
}

namespace EffectComposer {

class EffectCodeEditorWidget : public QmlJSEditor::QmlJSEditorWidget
{
    Q_OBJECT

public:
    EffectCodeEditorWidget();
    ~EffectCodeEditorWidget() override;

    void unregisterAutoCompletion();
    void setEditorTextWithIndentation(const QString &text);

    std::unique_ptr<TextEditor::AssistInterface> createAssistInterface(
        TextEditor::AssistKind assistKind, TextEditor::AssistReason assistReason) const override;

    void setUniformsCallback(const std::function<QStringList()> &callback);

signals:
    void returnKeyClicked();

public:
    Core::IContext *m_context = nullptr;
    QAction *m_completionAction = nullptr;
    bool m_isMultiline = true;

private:
    QStringList getUniforms() const;

    std::function<QStringList()> m_getUniforms;
};

class EffectDocument : public QmlJSEditor::QmlJSEditorDocument
{
public:
    EffectDocument();
    ~EffectDocument();

protected:
    void applyFontSettings() final;
    void triggerPendingUpdates() final;

private:
    QmlJSEditor::SemanticHighlighter *m_semanticHighlighter = nullptr;
};

class EffectCodeEditorFactory : public TextEditor::TextEditorFactory
{
public:
    EffectCodeEditorFactory();

    static void decorateEditor(TextEditor::TextEditorWidget *editor);
};

} // namespace EffectComposer
