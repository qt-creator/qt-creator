/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLDESIGNER_SHORTCUTMANAGER_H
#define QMLDESIGNER_SHORTCUTMANAGER_H

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

public slots:
    void updateActions(Core::IEditor* editor);

private slots:
    void undo();
    void redo();
    void deleteSelected();
    void cutSelected();
    void copySelected();
    void paste();
    void selectAll();
    void toggleSidebars();
    void toggleLeftSidebar();
    void toggleRightSidebar();
    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);
    void goIntoComponent();

private:
    QAction m_revertToSavedAction;
    QAction m_saveAction;
    QAction m_saveAsAction;
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
    QAction m_hideSidebarsAction;
    QAction m_restoreDefaultViewAction;
    QAction m_toggleLeftSidebarAction;
    QAction m_toggleRightSidebarAction;
    QAction m_goIntoComponentAction;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_SHORTCUTMANAGER_H
