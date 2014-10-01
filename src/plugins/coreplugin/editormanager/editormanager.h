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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef EDITORMANAGER_H
#define EDITORMANAGER_H

#include "../core_global.h"

#include "documentmodel.h"

#include <coreplugin/id.h>
#include <coreplugin/idocument.h> // enumerations

#include <QList>
#include <QWidget>
#include <QMenu>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace Core {

class IContext;
class IEditor;
class IEditorFactory;
class IExternalEditor;
class MimeType;
class IDocument;
class IMode;
class IVersionControl;

class EditorToolBar;

enum MakeWritableResult {
    OpenedWithVersionControl,
    MadeWritable,
    SavedAs,
    Failed
};

namespace Internal {
class EditorClosingCoreListener;
class EditorView;
class MainWindow;
class OpenEditorsViewFactory;
class OpenEditorsWindow;
class SplitterOrView;
} // namespace Internal

class CORE_EXPORT EditorManagerPlaceHolder : public QWidget
{
    Q_OBJECT
public:
    explicit EditorManagerPlaceHolder(Core::IMode *mode, QWidget *parent = 0);
    ~EditorManagerPlaceHolder();
private slots:
    void currentModeChanged(Core::IMode *);
private:
    Core::IMode *m_mode;
};

class CORE_EXPORT EditorManager : public QWidget
{
    Q_OBJECT

public:
    typedef QList<IEditorFactory *> EditorFactoryList;
    typedef QList<IExternalEditor *> ExternalEditorList;

    static EditorManager *instance();

    static EditorToolBar *createToolBar(QWidget *parent = 0);

    enum OpenEditorFlag {
        NoFlags = 0,
        DoNotChangeCurrentEditor = 1,
        IgnoreNavigationHistory = 2,
        DoNotMakeVisible = 4,
        CanContainLineNumber = 8,
        OpenInOtherSplit = 16,
        NoNewSplits = 32
    };
    Q_DECLARE_FLAGS(OpenEditorFlags, OpenEditorFlag)

    static QString splitLineNumber(QString *fileName);
    static IEditor *openEditor(const QString &fileName, Id editorId = Id(),
        OpenEditorFlags flags = NoFlags, bool *newEditor = 0);
    static IEditor *openEditorAt(const QString &fileName,  int line, int column = 0,
                                 Id editorId = Id(), OpenEditorFlags flags = NoFlags,
                                 bool *newEditor = 0);
    static IEditor *openEditorWithContents(Id editorId, QString *titlePattern = 0,
                                           const QByteArray &contents = QByteArray(),
                                           OpenEditorFlags flags = NoFlags);
    static IEditor *openEditor(Internal::EditorView *view, const QString &fileName,
        Id id = Id(), OpenEditorFlags flags = NoFlags, bool *newEditor = 0);

    static bool openExternalEditor(const QString &fileName, Id editorId);

    static QStringList getOpenFileNames();
    static Id getOpenWithEditorId(const QString &fileName, bool *isExternalEditor = 0);

    static IDocument *currentDocument();
    static IEditor *currentEditor();
    static Internal::EditorView *currentEditorView();
    static QList<IEditor *> visibleEditors();

    static void activateEditor(IEditor *editor, OpenEditorFlags flags = 0);
    static void activateEditorForEntry(DocumentModel::Entry *entry, OpenEditorFlags flags = 0);
    static IEditor *activateEditorForDocument(IDocument *document, OpenEditorFlags flags = 0);
    static IEditor *activateEditorForDocument(Internal::EditorView *view, IDocument *document, OpenEditorFlags flags = 0);

    static bool closeDocuments(const QList<IDocument *> &documents, bool askAboutModifiedEditors = true);
    static void closeEditor(DocumentModel::Entry *entry);
    static void closeOtherEditors(IDocument *document);

    static Internal::EditorView *viewForEditor(IEditor *editor);

    static void addCurrentPositionToNavigationHistory(IEditor *editor = 0, const QByteArray &saveState = QByteArray());
    static void cutForwardNavigationHistory();

    static bool saveEditor(IEditor *editor);

    static bool closeEditors(const QList<IEditor *> &editorsToClose, bool askAboutModifiedEditors = true);
    static void closeEditor(IEditor *editor, bool askAboutModifiedEditors = true);

    static MakeWritableResult makeFileWritable(IDocument *document);

    static QByteArray saveState();
    static bool restoreState(const QByteArray &state);
    static bool hasSplitter();

    static void saveSettings();
    static void readSettings();

    static Internal::OpenEditorsWindow *windowPopup();
    static void showPopupOrSelectDocument();

    static void showEditorStatusBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText = QString(),
                           QObject *object = 0, const char *member = 0);

    static void hideEditorStatusBar(const QString &id);

    static EditorFactoryList editorFactories(const MimeType &mimeType, bool bestMatchOnly = true);
    static ExternalEditorList externalEditors(const MimeType &mimeType, bool bestMatchOnly = true);

    static void setReloadSetting(IDocument::ReloadSetting behavior);
    static IDocument::ReloadSetting reloadSetting();

    static void setAutoSaveEnabled(bool enabled);
    static bool autoSaveEnabled();
    static void setAutoSaveInterval(int interval);
    static int autoSaveInterval();
    static bool isAutoSaveFile(const QString &fileName);

    static QTextCodec *defaultTextCodec();

    static qint64 maxTextFileSize();

    static void setWindowTitleAddition(const QString &addition);
    static QString windowTitleAddition();

    static void setWindowTitleVcsTopic(const QString &topic);
    static QString windowTitleVcsTopic();

    static void addSaveAndCloseEditorActions(QMenu *contextMenu, DocumentModel::Entry *entry);
    static void addNativeDirAndOpenWithActions(QMenu *contextMenu, DocumentModel::Entry *entry);

signals:
    void currentEditorChanged(Core::IEditor *editor);
    void currentDocumentStateChanged();
    void editorCreated(Core::IEditor *editor, const QString &fileName);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void editorsClosed(QList<Core::IEditor *> editors);
    void findOnFileSystemRequest(const QString &path);

public slots:
    static bool closeAllEditors(bool askAboutModifiedEditors = true);
    static void closeAllEditorsExceptVisible();

    static bool saveDocument(Core::IDocument *documentParam = 0);
    static bool saveDocumentAs(Core::IDocument *documentParam = 0);
    static void revertToSaved();
    static void revertToSaved(IDocument *document);
    static void closeEditor();
    static void closeOtherEditors();
    static void doEscapeKeyFocusMoveMagic();

private slots:
    static void gotoNextDocHistory();
    static void gotoPreviousDocHistory();
    static void handleContextChange(const QList<Core::IContext *> &context);
    static void updateActions();
    static void makeCurrentEditorWritable();
    static void vcsOpenCurrentEditor();
    static void updateWindowTitle();
    void handleDocumentStateChange();
    static void autoSave();

    static void saveDocumentFromContextMenu();
    static void saveDocumentAsFromContextMenu();
    static void revertToSavedFromContextMenu();

    static void closeEditorFromContextMenu();
    static void closeOtherEditorsFromContextMenu();

    static void showInGraphicalShell();
    static void openTerminal();
    static void findInDirectory();

    static void rootDestroyed(QObject *root);
    static void setCurrentEditorFromContextChange();

    static void gotoNextSplit();

public slots:
    static void goBackInNavigationHistory();
    static void goForwardInNavigationHistory();
    static void split(Qt::Orientation orientation);
    static void split();
    static void splitSideBySide();
    static void splitNewWindow();
    static void removeCurrentSplit();
    static void removeAllSplits();
    static void gotoOtherSplit();

private:
    explicit EditorManager(QWidget *parent);
    ~EditorManager();
    static void init();

    static IEditor *createEditor(Id id = Id(), const QString &fileName = QString());
    static void addEditor(IEditor *editor);
    static void removeEditor(IEditor *editor);

    static void restoreEditorState(IEditor *editor);

    static IEditor *placeEditor(Internal::EditorView *view, IEditor *editor);
    static IEditor *duplicateEditor(IEditor *editor);
    static IEditor *activateEditor(Internal::EditorView *view, IEditor *editor, OpenEditorFlags flags = NoFlags);
    static void activateEditorForEntry(Internal::EditorView *view, DocumentModel::Entry *entry,
                                       OpenEditorFlags flags = NoFlags);
    static void activateView(Internal::EditorView *view);
    static int visibleDocumentsCount();

    static void setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory = false);
    static void setCurrentView(Internal::EditorView *view);
    static Internal::SplitterOrView *findRoot(const Internal::EditorView *view, int *rootIndex = 0);

    static void closeView(Internal::EditorView *view);
    static void emptyView(Internal::EditorView *view);
    static void splitNewWindow(Internal::EditorView *view);
    static IEditor *pickUnusedEditor(Internal::EditorView **foundView = 0);
    static void addDocumentToRecentFiles(IDocument *document);
    static void updateAutoSave();
    static void setCloseSplitEnabled(Internal::SplitterOrView *splitterOrView, bool enable);
    static void updateMakeWritableWarning();
    static void setupSaveActions(IDocument *document, QAction *saveAction, QAction *saveAsAction, QAction *revertToSavedAction);

    friend class Core::Internal::MainWindow;
    friend class Core::Internal::SplitterOrView;
    friend class Core::Internal::EditorView;
    friend class Core::EditorToolBar;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditorManager::OpenEditorFlags)

#endif // EDITORMANAGER_H
