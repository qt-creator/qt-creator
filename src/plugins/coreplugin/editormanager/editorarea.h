// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QPointer>
#include <QWidget>

namespace Core {

class IDocument;
class IEditor;

namespace Internal {

class EditorView;
class SplitterOrView;

class EditorArea : public QWidget
{
    Q_OBJECT

public:
    EditorArea();
    ~EditorArea() override;

    IDocument *currentDocument() const;
    EditorView *currentView() const;
    EditorView *findFirstView() const;
    EditorView *findLastView() const;

    bool hasSplits() const;
    EditorView *unsplit(EditorView *view);
    void unsplitAll(EditorView *viewToKeep);

    QByteArray saveState() const;
    void restoreState(const QByteArray &s);

signals:
    void windowTitleNeedsUpdate();
    void hidden();
    void splitStateChanged();

private:
    void focusChanged(QWidget *old, QWidget *now);
    void setCurrentView(EditorView *view);
    void updateCurrentEditor(IEditor *editor);
    void updateCloseSplitButton();
    void hideEvent(QHideEvent *) override;

    SplitterOrView *m_splitterOrView = nullptr;
    QPointer<EditorView> m_currentView;
    QPointer<IDocument> m_currentDocument;
};

} // namespace Internal
} // namespace Core
