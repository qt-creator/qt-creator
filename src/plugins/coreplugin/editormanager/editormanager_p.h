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

class Command;
class EditorManager;

namespace Internal {

class EditorWindow;
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
        Suspend, // tab is kept, and document model entry is suspended if editor was the last one
        SuspendRemoveTab // tab is removed, but document model entry is suspended if editor was the last one
    };

    enum class RemoveEditorFlag { None, EnsureNewEditor };

    static EditorManagerPrivate *instance();

    static void extensionsInitialized(); // only use from MainWindow

    static EditorArea *mainEditorArea();
    static EditorView *currentEditorView();
    static QList<EditorView *> allEditorViews();
    static bool hasMoreThanOneview();
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
    static IEditor *openEditorWith(
        const Utils::FilePath &filePath, Utils::Id editorId, EditorView *mainView = nullptr);
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
    static void tabClosed(DocumentModel::Entry *entry);
    static QSet<DocumentModel::Entry *> entriesToCloseForTabbedViews(
        const QSet<EditorView *> &viewsToClose);
    static void removeEditorsFromViews(
        const QList<std::pair<EditorView *, IEditor *>> &editorsPerView,
        EditorView::RemovalOption option,
        RemoveEditorFlag flag);

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
    static Utils::FilePaths openFilesForState(const QByteArray &state, int max);

    static EditorWindow *createEditorWindow();
    static void addEditorArea(EditorArea *area);
    static void splitNewWindow(Internal::EditorView *view);
    static void closeView(Internal::EditorView *view);
    static const QList<IEditor *> emptyView(Internal::EditorView *view);
    static void setShowingTabs(bool visible);
    static void deleteEditors(const QList<IEditor *> &editors);

    static void updateActions();

    static void updateWindowTitleForDocument(IDocument *document, QWidget *window);

    static void vcsOpenCurrentEditor();
    static void makeCurrentEditorWritable();

    static void setPlaceholderText(const QString &text);
    static QString placeholderText();

    static void updateAutoSave();

    static void handleFileRenamed(
        const Utils::FilePath &originalFilePath,
        const Utils::FilePath &newFilePath,
        Utils::Id originalType = {});

    static void addClosedDocumentToCloseHistory(IEditor *editor);

    static void addContextMenuActions(
        QMenu *contextMenu,
        DocumentModel::Entry *entry,
        IEditor *editor,
        EditorView *view = nullptr);

public slots:
    static bool saveDocument(Core::IDocument *document);
    static bool saveDocumentAs(Core::IDocument *document);

    static void split(Qt::Orientation orientation);
    static void removeAllSplits();
    static void gotoPreviousSplit();
    static void gotoNextSplit();

    static void reopenLastClosedDocument();

    void handleDocumentStateChange(Core::IDocument *document);
    void editorAreaDestroyed(QObject *area);

signals:
    void placeholderTextChanged(const QString &text);
    void currentViewChanged();
    void viewCountChanged();

private:
    static void gotoNextDocHistory();
    static void gotoPreviousDocHistory();

    static void gotoNextTab();
    static void gotoPreviousTab();

    static void gotoLastEditLocation();

    static void autoSave();
    static void handleContextChange(const QList<Core::IContext *> &context);

    static void closeAllEditorsExceptVisible();
    static void revertToSaved(IDocument *document);
    static void autoSuspendDocuments();

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
    static EditorArea *findEditorArea(const EditorView *view, int *areaIndex = nullptr);
    static IEditor *pickUnusedEditor(Internal::EditorView **foundView = nullptr);
    static void addDocumentToRecentFiles(IDocument *document);
    static void updateMakeWritableWarning();
    static void setupSaveActions(IDocument *document, QAction *saveAction,
                                 QAction *saveAsAction, QAction *revertToSavedAction);
    static void updateWindowTitle();
    static bool skipOpeningBigTextFile(const Utils::FilePath &filePath);

    static void addCopyFilePathActions(
        QMenu *contextMenu, const Utils::FilePath &filePath, IEditor *editor = nullptr);
    static void addSaveAndCloseEditorActions(
        QMenu *contextMenu,
        DocumentModel::Entry *entry,
        IEditor *editor,
        EditorView *view = nullptr);
    static void addNativeDirAndOpenWithActions(
        QMenu *contextMenu, const Utils::FilePath &filePath, EditorView *view = nullptr);
    static void populateOpenWithMenu(
        QMenu *menu, const Utils::FilePath &filePath, EditorView *view = nullptr);

private:
    explicit EditorManagerPrivate(QObject *parent);
    ~EditorManagerPrivate() override;
    void init();

    EditLocation m_globalLastEditLocation;
    QList<EditLocation> m_globalHistory;
    QList<EditorArea *> m_editorAreas;
    QPointer<IEditor> m_currentEditor;
    QPointer<IEditor> m_scheduledCurrentEditor;
    QList<QPointer<EditorView>> m_currentView;
    QTimer *m_autoSaveTimer = nullptr;

    // actions
    QAction *m_revertToSavedAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAsAction = nullptr;
    QAction *m_closeCurrentEditorAction = nullptr;
    QAction *m_closeAllEditorsAction = nullptr;
    QAction *m_closeOtherDocumentsAction = nullptr;
    QAction *m_closeAllEditorsExceptVisibleAction = nullptr;
    QAction *m_gotoNextDocHistoryAction = nullptr;
    QAction *m_gotoPreviousDocHistoryAction = nullptr;
    QAction *m_gotoNextTabAction = nullptr;
    QAction *m_gotoPreviousTabAction = nullptr;
    Command *m_gotoNextTabCommand = nullptr;
    Command *m_gotoPreviousTabCommand = nullptr;
    QAction *m_goBackAction = nullptr;
    QAction *m_goForwardAction = nullptr;
    QAction *m_nextDocAction = nullptr;
    QAction *m_prevDocAction = nullptr;
    QAction *m_reopenLastClosedDocumenAction = nullptr;
    QAction *m_gotoLastEditAction = nullptr;
    QAction *m_splitAction = nullptr;
    QAction *m_splitSideBySideAction = nullptr;
    QAction *m_splitNewWindowAction = nullptr;
    QAction *m_removeCurrentSplitAction = nullptr;
    QAction *m_removeAllSplitsAction = nullptr;
    QAction *m_gotoPreviousSplitAction = nullptr;
    QAction *m_gotoNextSplitAction = nullptr;

    QAction *m_openGraphicalShellAction = nullptr;
    QAction *m_showInFileSystemViewAction = nullptr;

    OpenEditorsWindow *m_windowPopup = nullptr;

    QMap<QString, QVariant> m_editorStates;

    EditorManager::WindowTitleHandler m_titleAdditionHandler;
    EditorManager::WindowTitleHandler m_sessionTitleHandler;
    EditorManager::WindowTitleHandler m_titleVcsTopicHandler;

    QString m_placeholderText;
    QList<std::function<bool(IEditor *)>> m_closeEditorListeners;
};

} // Internal
} // Core
