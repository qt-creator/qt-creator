/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef EDITORMANAGER_H
#define EDITORMANAGER_H

#include "../core_global.h"

#include <coreplugin/id.h>
#include <coreplugin/ifile.h> // enumerations

#include <QtCore/QList>
#include <QtGui/QWidget>
#include <QtGui/QMenu>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Core {

class IContext;
class ICore;
class IEditor;
class IEditorFactory;
class IExternalEditor;
class MimeType;
class IFile;
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

    explicit EditorManager(ICore *core, QWidget *parent);
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

    IEditor *openEditor(const QString &fileName, const Id &editorId = Id(),
        OpenEditorFlags flags = 0, bool *newEditor = 0);
    IEditor *openEditorWithContents(const Id &editorId,
        QString *titlePattern = 0, const QString &contents = QString());

    bool openExternalEditor(const QString &fileName, const Id &editorId);

    QStringList getOpenFileNames() const;
    Id getOpenWithEditorId(const QString &fileName, bool *isExternalEditor = 0) const;

    bool hasEditor(const QString &fileName) const;
    QList<IEditor *> editorsForFileName(const QString &filename) const;
    QList<IEditor *> editorsForFile(IFile *file) const;

    IEditor *currentEditor() const;
    QList<IEditor *> visibleEditors() const;
    QList<IEditor*> openedEditors() const;

    void activateEditor(IEditor *editor, OpenEditorFlags flags = 0);
    void activateEditorForIndex(const QModelIndex &index, OpenEditorFlags = 0);
    IEditor *activateEditorForFile(Internal::EditorView *view, IFile *file, OpenEditorFlags flags = 0);

    OpenEditorsModel *openedEditorsModel() const;
    void closeEditor(const QModelIndex &index);
    void closeOtherEditors(IEditor *editor);

    QList<IEditor*> editorsForFiles(QList<IFile *> files) const;
    void addCurrentPositionToNavigationHistory(IEditor *editor = 0, const QByteArray &saveState = QByteArray());
    void cutForwardNavigationHistory();

    bool saveEditor(IEditor *editor);

    bool closeEditors(const QList<IEditor *> &editorsToClose, bool askAboutModifiedEditors = true);

    MakeWritableResult makeFileWritable(IFile *file);

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

    EditorFactoryList editorFactories(const MimeType &mimeType, bool bestMatchOnly = true) const;
    ExternalEditorList externalEditors(const MimeType &mimeType, bool bestMatchOnly = true) const;

    void setReloadSetting(IFile::ReloadSetting behavior);
    IFile::ReloadSetting reloadSetting() const;

    void setAutoSaveEnabled(bool enabled);
    bool autoSaveEnabled() const;
    void setAutoSaveInterval(int interval);
    int autoSaveInterval() const;

    QTextCodec *defaultTextCodec() const;

    static qint64 maxTextFileSize();

    void setWindowTitleAddition(const QString &addition);
    QString windowTitleAddition() const;

    void addCloseEditorActions(QMenu *contextMenu, const QModelIndex &editorIndex);
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

    bool saveFile(Core::IFile *file = 0);
    bool saveFileAs(Core::IFile *file = 0);
    void revertToSaved();
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
    void updateVariable(const QString &variable);
    void autoSave();

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
    QList<IFile *> filesForEditors(QList<IEditor *> editors) const;
    IEditor *createEditor(const Id &id = Id(), const QString &fileName = QString());
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

    void closeEditor(IEditor *editor);
    void closeDuplicate(IEditor *editor);
    void closeView(Internal::EditorView *view);
    void emptyView(Internal::EditorView *view);
    Internal::EditorView *currentEditorView() const;
    IEditor *pickUnusedEditor() const;
    void addFileToRecentFiles(IFile *file);
    void switchToPreferedMode();
    void updateAutoSave();

    EditorManagerPrivate *d;

    friend class Core::Internal::SplitterOrView;
    friend class Core::Internal::EditorView;
    friend class Core::EditorToolBar;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditorManager::OpenEditorFlags)

#endif // EDITORMANAGER_H
