// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QToolBar>
#include <QUndoGroup>

#include <QCoreApplication>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/icontext.h>

using namespace Core;

namespace ScxmlEditor {
namespace Internal {

class ScxmlTextEditorFactory;
class ScxmlEditorWidget;
class ScxmlEditorStack;

class ScxmlEditorData : public QObject
{
    Q_OBJECT
public:
    ScxmlEditorData();
    ~ScxmlEditorData() override;

    void fullInit();
    IEditor *createEditor();

private:
    void updateToolBar();
    QWidget *createModeWidget();
    EditorToolBar *createMainToolBar();

    Context m_contexts;
    QWidget *m_modeWidget = nullptr;
    ScxmlEditorStack *m_widgetStack = nullptr;
    QToolBar *m_widgetToolBar = nullptr;
    EditorToolBar *m_mainToolBar = nullptr;
    QUndoGroup *m_undoGroup = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;

    ScxmlTextEditorFactory *m_xmlEditorFactory = nullptr;
};

} // namespace Internal
} // namespace ScxmlEditor
