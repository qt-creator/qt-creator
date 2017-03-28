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

#include <QObject>
#include <QAction>
#include <utils/parameteraction.h>

namespace Core {
    class IEditor;
    class Context;
}

namespace QmlDesigner {

class DesignDocument;

class ShortCutManager : public QObject
{
    Q_OBJECT

public:
    ShortCutManager();

    void registerActions(const Core::Context &qmlDesignerMainContext,
                         const Core::Context &qmlDesignerFormEditorContext,
                         const Core::Context &qmlDesignerNavigatorContext);

    void connectUndoActions(DesignDocument *designDocument);
    void disconnectUndoActions(DesignDocument *designDocument);
    void updateUndoActions(DesignDocument *designDocument);
    DesignDocument *currentDesignDocument() const;

    void updateActions(Core::IEditor* editor);

private:
    void undo();
    void redo();
    void deleteSelected();
    void cutSelected();
    void copySelected();
    void paste();
    void selectAll();
    void toggleLeftSidebar();
    void toggleRightSidebar();
    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);
    void goIntoComponent();

private:
    QAction m_revertToSavedAction;
    QAction m_saveAction;
    QAction m_saveAsAction;
    QAction m_exportAsImageAction;
    QAction m_closeCurrentEditorAction;
    QAction m_closeAllEditorsAction;
    QAction m_closeOtherEditorsAction;
    QAction m_undoAction;
    QAction m_redoAction;
    Utils::ParameterAction m_deleteAction;
    Utils::ParameterAction m_cutAction;
    Utils::ParameterAction m_copyAction;
    Utils::ParameterAction m_pasteAction;
    Utils::ParameterAction m_selectAllAction;
    QAction m_collapseExpandStatesAction;
    QAction m_restoreDefaultViewAction;
    QAction m_toggleLeftSidebarAction;
    QAction m_toggleRightSidebarAction;
    QAction m_goIntoComponentAction;
    QAction m_switchTextFormAction;
    QAction m_escapeAction;
};

} // namespace QmlDesigner
