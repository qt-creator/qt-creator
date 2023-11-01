// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "documentmodel.h"
#include "editorarea.h"
#include "editormanager.h"
#include "editorview.h"
#include "ieditor.h"
#include "ieditorfactory.h"

#include "../idocument.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QAction;
class QSettings;
class QTimer;
QT_END_NAMESPACE

namespace Utils {
class QtcSettings;
}

namespace Core {

class EditorManager;

namespace Internal {

class EditorWindow;
class MainWindow;
class OpenEditorsViewFactory;
class OpenEditorsWindow;

enum MakeWritableResult { OpenedWithVersionControl, MadeWritable, SavedAs, Failed };

class EditorManagerPrivate : public QObject
{
    Q_OBJECT

    friend class Core::EditorManager;

public:
    enum class CloseFlag {
        CloseWithAsking,
        CloseWithoutAsking,
        Suspend
    };

    static EditorManagerPrivate *instance();

    static void extensionsInitialized(); // only use from MainWindow

    static EditorArea *mainEditorArea();
    static EditorView *currentEditorView();
    static QList<EditorView *> allEditorViews();
    static void setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory = false);
    static IEditor *openEditor(EditorView *view,
                               const Utils::FilePath &filePath,
                               Utils::Id editorId = {},
                               EditorManager::OpenEditorFlags flags = EditorManager::NoFlags,
                               bool *newEditor = nullptr);
    static IEditor *openEditorAt(EditorView *view,
                                 const Utils::Link &filePath,
                                 Utils::Id editorId = {},
                                 EditorManager::OpenEditorFlags flags = EditorManager::NoFlags,
                                 bool *newEditor = nullptr);
    static IEditor *openEditorWith(const Utils::FilePath &filePath, Utils::Id editorId);
    static IEditor *duplicateEditor(IEditor *editor);
    static IEditor *activateEditor(EditorView *view, IEditor *editor,
                                   EditorManager::OpenEditorFlags flags = EditorManager::NoFlags);
    static IEditor *activateEditorForDocument(EditorView *view, IDocument *document,
                                              EditorManager::OpenEditorFlags flags = {});
    static bool activateEditorForEntry(EditorView *view, DocumentModel::Entry *entry,
                                       EditorManager::OpenEditorFlags flags = EditorManager::NoFlags);
    /* closes the document if there is no other editor on the document visible */
    static void closeEditorOrDocument(IEditor *editor);
    static bool closeEditors(const QList<IEditor *> &editors, CloseFlag flag);

    static EditorView *viewForEditor(IEditor *editor);
    static void setCurrentView(EditorView *view);
    static void activateView(EditorView *view);

    static MakeWritableResult makeFileWritable(IDocument *document);
    static void doEscapeKeyFocusMoveMagic();

    static Utils::Id getOpenWithEditorId(const Utils::FilePath &fileName, bool *isExternalEditor = nullptr);

    static void saveSettings();
    static void readSettings();
    static Qt::CaseSensitivity readFileSystemSensitivity(Utils::QtcSettings *settings);
    static void writeFileSystemSensitivity(Utils::QtcSettings *settings,
                                           Qt::CaseSensitivity sensitivity);

    static EditorWindow *createEditorWindow();
    static void splitNewWindow(Internal::EditorView *view);
    static void closeView(Internal::EditorView *view);
    static const QList<IEditor *> emptyView(Internal::EditorView *view);
    static void deleteEditors(const QList<IEditor *> &editors);

    static void updateActions();

    static void updateWindowTitleForDocument(IDocument *document, QWidget *window);

    static void vcsOpenCurrentEditor();
    static void makeCurrentEditorWritable();

    static void setPlaceholderText(const QString &text);
    static QString placeholderText();

    static void updateAutoSave();

public slots:
    static bool saveDocument(Core::IDocument *document);
    static bool saveDocumentAs(Core::IDocument *document);

    static void split(Qt::Orientation orientation);
    static void removeAllSplits();
    static void gotoPreviousSplit();
    static void gotoNextSplit();

    void handleDocumentStateChange(Core::IDocument *document);
    void editorAreaDestroyed(QObject *area);

signals:
    void placeholderTextChanged(const QString &text);

private:
    static void gotoNextDocHistory();
    static void gotoPreviousDocHistory();

    static void gotoLastEditLocation();

    static void autoSave();
    static void handleContextChange(const QList<Core::IContext *> &context);

    static void copyFilePathFromContextMenu();
    void copyLocationFromContextMenu();
    static void copyFileNameFromContextMenu();
    static void saveDocumentFromContextMenu();
    static void saveDocumentAsFromContextMenu();
    static void revertToSavedFromContextMenu();
    static void closeEditorFromContextMenu();
    static void closeOtherDocumentsFromContextMenu();

    static void closeAllEditorsExceptVisible();
    static void revertToSaved(IDocument *document);
    static void autoSuspendDocuments();

    static void openTerminal();
    static void findInDirectory();

    static void togglePinned();

    static void removeCurrentSplit();

    static void setCurrentEditorFromContextChange();

    static OpenEditorsWindow *windowPopup();
    static void showPopupOrSelectDocument();

    static EditorFactories findFactories(Utils::Id editorId, const Utils::FilePath &filePath);
    static IEditor *createEditor(IEditorFactory *factory, const Utils::FilePath &filePath);
    static void addEditor(IEditor *editor);
    static void removeEditor(IEditor *editor, bool removeSusependedEntry);
    static IEditor *placeEditor(EditorView *view, IEditor *editor);
    static void restoreEditorState(IEditor *editor);
    static int visibleDocumentsCount();
    static EditorArea *findEditorArea(const EditorView *view, int *areaIndex = nullptr);
    static IEditor *pickUnusedEditor(Internal::EditorView **foundView = nullptr);
    static void addDocumentToRecentFiles(IDocument *document);
    static void updateMakeWritableWarning();
    static void setupSaveActions(IDocument *document, QAction *saveAction,
                                 QAction *saveAsAction, QAction *revertToSavedAction);
    static void updateWindowTitle();
    static bool skipOpeningBigTextFile(const Utils::FilePath &filePath);

private:
    explicit EditorManagerPrivate(QObject *parent);
    ~EditorManagerPrivate() override;
    void init();

    EditLocation m_globalLastEditLocation;
    QList<EditLocation> m_globalHistory;
    QList<EditorArea *> m_editorAreas;
    QPointer<IEditor> m_currentEditor;
    QPointer<IEditor> m_scheduledCurrentEditor;
    QPointer<EditorView> m_currentView;
    QTimer *m_autoSaveTimer = nullptr;

    // actions
    QAction *m_revertToSavedAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeCurrentEditorAction;
    QAction *m_closeAllEditorsAction;
    QAction *m_closeOtherDocumentsAction;
    QAction *m_closeAllEditorsExceptVisibleAction;
    QAction *m_gotoNextDocHistoryAction;
    QAction *m_gotoPreviousDocHistoryAction;
    QAction *m_goBackAction;
    QAction *m_goForwardAction;
    QAction *m_gotoLastEditAction;
    QAction *m_splitAction;
    QAction *m_splitSideBySideAction;
    QAction *m_splitNewWindowAction;
    QAction *m_removeCurrentSplitAction;
    QAction *m_removeAllSplitsAction;
    QAction *m_gotoPreviousSplitAction;
    QAction *m_gotoNextSplitAction;

    QAction *m_copyFilePathContextAction;
    QAction *m_copyLocationContextAction; // Copy path and line number.
    QAction *m_copyFileNameContextAction;
    QAction *m_saveCurrentEditorContextAction;
    QAction *m_saveAsCurrentEditorContextAction;
    QAction *m_revertToSavedCurrentEditorContextAction;

    QAction *m_closeCurrentEditorContextAction;
    QAction *m_closeAllEditorsContextAction;
    QAction *m_closeOtherDocumentsContextAction;
    QAction *m_closeAllEditorsExceptVisibleContextAction;
    QAction *m_openGraphicalShellAction;
    QAction *m_openGraphicalShellContextAction;
    QAction *m_showInFileSystemViewAction;
    QAction *m_showInFileSystemViewContextAction;
    QAction *m_openTerminalAction;
    QAction *m_findInDirectoryAction;
    QAction *m_filePropertiesAction = nullptr;
    QAction *m_pinAction = nullptr;
    DocumentModel::Entry *m_contextMenuEntry = nullptr;
    IEditor *m_contextMenuEditor = nullptr;

    OpenEditorsWindow *m_windowPopup = nullptr;

    QMap<QString, QVariant> m_editorStates;
    OpenEditorsViewFactory *m_openEditorsFactory = nullptr;

    EditorManager::WindowTitleHandler m_titleAdditionHandler;
    EditorManager::WindowTitleHandler m_sessionTitleHandler;
    EditorManager::WindowTitleHandler m_titleVcsTopicHandler;

    QString m_placeholderText;
    QList<std::function<bool(IEditor *)>> m_closeEditorListeners;
};

} // Internal
} // Core
