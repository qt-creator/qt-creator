/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QToolBar>
#include <QUndoGroup>

#include <QCoreApplication>
#include <coreplugin/designmode.h>
#include <coreplugin/editortoolbar.h>

using namespace Core;

namespace ScxmlEditor {
namespace Internal {

class ScxmlTextEditorFactory;
class ScxmlEditorWidget;
class ScxmlEditorStack;
class ScxmlContext;

class ScxmlEditorData : public QObject
{
    Q_OBJECT
public:
    ScxmlEditorData(QObject *parent = nullptr);
    ~ScxmlEditorData();

    void fullInit();
    IEditor *createEditor();

private:
    void updateToolBar();
    QWidget *createModeWidget();
    EditorToolBar *createMainToolBar();

    ScxmlContext *m_context = nullptr;
    Context m_contexts;
    QWidget *m_modeWidget = nullptr;
    ScxmlEditorStack *m_widgetStack = nullptr;
    DesignMode *m_designMode = nullptr;
    QToolBar *m_widgetToolBar = nullptr;
    EditorToolBar *m_mainToolBar = nullptr;
    QUndoGroup *m_undoGroup = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;

    ScxmlTextEditorFactory *m_xmlEditorFactory = nullptr;
};

} // namespace Internal
} // namespace ScxmlEditor
