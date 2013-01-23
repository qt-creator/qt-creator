#ifndef QMLDESIGNER_SHORTCUTMANAGER_H
#define QMLDESIGNER_SHORTCUTMANAGER_H

#include <QObject>
#include <QAction>
#include <utils/parameteraction.h>


namespace Core {
    class IEditor;
}

namespace QmlDesigner {

class DesignDocument;

class ShortCutManager : public QObject
{
    Q_OBJECT

public:
    ShortCutManager();

    void registerActions();

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
