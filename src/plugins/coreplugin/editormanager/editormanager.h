/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef EDITORMANAGER_H
#define EDITORMANAGER_H

#include "../core_global.h"

#include <coreplugin/id.h>
#include <coreplugin/idocument.h> // enumerations

#include <QList>
#include <QWidget>
#include <QMenu>

QT_BEGIN_NAMESPACE
class QModelIndex;
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

struct EditorManagerPrivate;
class OpenEditorsModel;

namespace Internal {
class OpenEditorsWindow;
class EditorView;
class SplitterOrView;

class EditorClosingCoreListener;
class OpenEditorsViewFactory;


} // namespace Internal

class CORE_EXPORT EditorManagerPlaceHolder : public QWidget
{
    Q_OBJECT
public:
    explicit EditorManagerPlaceHolder(Core::IMode *mode, QWidget *parent = 0);
    ~EditorManagerPlaceHolder();
    static EditorManagerPlaceHolder* current();
private slots:
    void currentModeChanged(Core::IMode *);
private:
    Core::IMode *m_mode;
    static EditorManagerPlaceHolder* m_current;
};

class CORE_EXPORT EditorManager : public QWidget
{
    Q_OBJECT

public:
    typedef QList<IEditorFactory *> EditorFactoryList;
    typedef QList<IExternalEditor *> ExternalEditorList;

    explicit EditorManager(QWidget *parent);
    virtual ~EditorManager();
    void init();
    static EditorManager *instance();

    static EditorToolBar *createToolBar(QWidget *parent = 0);

    enum OpenEditorFlag {
        NoActivate = 1,
        IgnoreNavigationHistory = 2,
        ModeSwitch = 4,
        CanContainLineNumber = 8
    };
    Q_DECLARE_FLAGS(OpenEditorFlags, OpenEditorFlag)

    static QString splitLineNumber(QString *fileName);
    static IEditor *openEditor(const QString &fileName, const Id &editorId = Id(),
        OpenEditorFlags flags = 0, bool *newEditor = 0);
    static IEditor *openEditorWithContents(const Id &editorId,
        QString *titlePattern = 0, const QString &contents = QString());

    static bool openExternalEditor(const QString &fileName, const Id &editorId);

    QStringList getOpenFileNames() const;
    Id getOpenWithEditorId(const QString &fileName, bool *isExternalEditor = 0) const;

    bool hasEditor(const QString &fileName) const;
    QList<IEditor *> editorsForFileName(const QString &filename) const;
    QList<IEditor *> editorsForDocument(IDocument *document) const;

    static IEditor *currentEditor();
    QList<IEditor *> visibleEditors() const;
    QList<IEditor*> openedEditors() const;

    static void activateEditor(IEditor *editor, OpenEditorFlags flags = 0);
    void activateEditorForIndex(const QModelIndex &index, OpenEditorFlags = 0);
    IEditor *activateEditorForDocument(Internal::EditorView *view, IDocument *document, OpenEditorFlags flags = 0);

    OpenEditorsModel *openedEditorsModel() const;
    void closeEditor(const QModelIndex &index);
    void closeOtherEditors(IEditor *editor);

    QList<IEditor*> editorsForDocuments(QList<IDocument *> documents) const;
    void addCurrentPositionToNavigationHistory(IEditor *editor = 0, const QByteArray &saveState = QByteArray());
    void cutForwardNavigationHistory();

    bool saveEditor(IEditor *editor);

    bool closeEditors(const QList<IEditor *> &editorsToClose, bool askAboutModifiedEditors = true);

    MakeWritableResult makeFileWritable(IDocument *document);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    bool hasSplitter() const;

    void saveSettings();
    void readSettings();

    Internal::OpenEditorsWindow *windowPopup() const;
    void showPopupOrSelectDocument() const;

    void showEditorStatusBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText = QString(),
                           QObject *object = 0, const char *member = 0);

    void hideEditorStatusBar(const QString &id);

    static EditorFactoryList editorFactories(const MimeType &mimeType, bool bestMatchOnly = true);
    static ExternalEditorList externalEditors(const MimeType &mimeType, bool bestMatchOnly = true);

    void setReloadSetting(IDocument::ReloadSetting behavior);
    IDocument::ReloadSetting reloadSetting() const;

    void setAutoSaveEnabled(bool enabled);
    bool autoSaveEnabled() const;
    void setAutoSaveInterval(int interval);
    int autoSaveInterval() const;

    QTextCodec *defaultTextCodec() const;

    static qint64 maxTextFileSize();

    void setWindowTitleAddition(const QString &addition);
    QString windowTitleAddition() const;

    void addSaveAndCloseEditorActions(QMenu *contextMenu, const QModelIndex &editorIndex);
    void addNativeDirActions(QMenu *contextMenu, const QModelIndex &editorIndex);

signals:
    void currentEditorChanged(Core::IEditor *editor);
    void currentEditorStateChanged(Core::IEditor *editor);
    void editorCreated(Core::IEditor *editor, const QString &fileName);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void editorsClosed(QList<Core::IEditor *> editors);

public slots:
    bool closeAllEditors(bool askAboutModifiedEditors = true);

    bool saveDocument(Core::IDocument *documentParam = 0);
    bool saveDocumentAs(Core::IDocument *documentParam = 0);
    void revertToSaved();
    void revertToSaved(Core::IEditor *editor);
    void closeEditor();
    void closeOtherEditors();

private slots:
    void gotoNextDocHistory();
    void gotoPreviousDocHistory();
    void handleContextChange(Core::IContext *context);
    void updateActions();
    void makeCurrentEditorWritable();
    void vcsOpenCurrentEditor();
    void updateWindowTitle();
    void handleEditorStateChange();
    void updateVariable(const QByteArray &variable);
    void autoSave();

    void saveDocumentFromContextMenu();
    void saveDocumentAsFromContextMenu();
    void revertToSavedFromContextMenu();

    void closeEditorFromContextMenu();
    void closeOtherEditorsFromContextMenu();

    void showInGraphicalShell();
    void openTerminal();

public slots:
    void goBackInNavigationHistory();
    void goForwardInNavigationHistory();
    void split(Qt::Orientation orientation);
    void split();
    void splitSideBySide();
    void removeCurrentSplit();
    void removeAllSplits();
    void gotoOtherSplit();

private:
    QList<IDocument *> documentsForEditors(QList<IEditor *> editors) const;
    static IEditor *createEditor(const Id &id = Id(), const QString &fileName = QString());
    void addEditor(IEditor *editor, bool isDuplicate = false);
    void removeEditor(IEditor *editor);

    void restoreEditorState(IEditor *editor);

    IEditor *placeEditor(Internal::EditorView *view, IEditor *editor);
    IEditor *duplicateEditor(IEditor *editor);
    void setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory = false);
    void setCurrentView(Internal::SplitterOrView *view);
    IEditor *activateEditor(Internal::EditorView *view, IEditor *editor, OpenEditorFlags flags = 0);
    void activateEditorForIndex(Internal::EditorView *view, const QModelIndex &index, OpenEditorFlags = 0);
    IEditor *openEditor(Internal::EditorView *view, const QString &fileName,
        const Id &id = Id(), OpenEditorFlags flags = 0, bool *newEditor = 0);
    Internal::SplitterOrView *currentSplitterOrView() const;
    Internal::SplitterOrView *topSplitterOrView() const;

    void closeEditor(IEditor *editor);
    void closeDuplicate(IEditor *editor);
    void closeView(Internal::EditorView *view);
    void emptyView(Internal::EditorView *view);
    Internal::EditorView *currentEditorView() const;
    IEditor *pickUnusedEditor() const;
    void addDocumentToRecentFiles(IDocument *document);
    void switchToPreferedMode();
    void updateAutoSave();
    void setCloseSplitEnabled(Internal::SplitterOrView *splitterOrView, bool enable);
    void updateMakeWritableWarning();
    QString fileNameForEditor(IEditor *editor);
    void setupSaveActions(IEditor *editor, QAction *saveAction, QAction *saveAsAction, QAction *revertToSavedAction);

    EditorManagerPrivate *d;

    friend class Core::Internal::SplitterOrView;
    friend class Core::Internal::EditorView;
    friend class Core::EditorToolBar;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditorManager::OpenEditorFlags)

#endif // EDITORMANAGER_H
