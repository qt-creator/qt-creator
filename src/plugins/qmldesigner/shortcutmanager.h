// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QAction>

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
                         const Core::Context &qmlDesignerEditor3DContext,
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
    void duplicateSelected();
    void paste();
    void selectAll();
    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);
    void goIntoComponent();

private:
    QAction m_revertToSavedAction;
    QAction m_saveAction;
    QAction m_saveAsAction;
    QAction m_exportAsImageAction;
    QAction m_takeScreenshotAction;
    QAction m_closeCurrentEditorAction;
    QAction m_closeAllEditorsAction;
    QAction m_closeOtherEditorsAction;
    QAction m_undoAction;
    QAction m_redoAction;
    QAction m_deleteAction;
    QAction m_cutAction;
    QAction m_copyAction;
    QAction m_pasteAction;
    QAction m_duplicateAction;
    QAction m_selectAllAction;
    QAction m_escapeAction;

    bool isMatBrowserActive = false;
    bool isAssetsLibraryActive = false;
};

} // namespace QmlDesigner
