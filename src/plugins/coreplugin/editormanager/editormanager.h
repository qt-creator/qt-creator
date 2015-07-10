/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef EDITORMANAGER_H
#define EDITORMANAGER_H

#include "../core_global.h"
#include "../idocument.h"

#include "documentmodel.h"

#include <QList>
#include <QWidget>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Utils { class MimeType; }

namespace Core {

class IContext;
class IEditor;
class IEditorFactory;
class IExternalEditor;
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
class EditorManagerPrivate;
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
    explicit EditorManagerPlaceHolder(IMode *mode, QWidget *parent = 0);
    ~EditorManagerPlaceHolder();

private:
    void currentModeChanged(IMode *);

    IMode *m_mode;
};

class CORE_EXPORT EditorManager : public QObject
{
    Q_OBJECT

public:
    typedef QList<IEditorFactory *> EditorFactoryList;
    typedef QList<IExternalEditor *> ExternalEditorList;
    typedef std::function<QString (const QString &)> WindowTitleHandler;

    static EditorManager *instance();

    enum OpenEditorFlag {
        NoFlags = 0,
        DoNotChangeCurrentEditor = 1,
        IgnoreNavigationHistory = 2,
        DoNotMakeVisible = 4,
        CanContainLineAndColumnNumber = 8,
        OpenInOtherSplit = 16,
        DoNotSwitchToDesignMode = 32
    };
    Q_DECLARE_FLAGS(OpenEditorFlags, OpenEditorFlag)

    static QString splitLineAndColumnNumber(QString *fileName);
    static IEditor *openEditor(const QString &fileName, Id editorId = Id(),
        OpenEditorFlags flags = NoFlags, bool *newEditor = 0);
    static IEditor *openEditorAt(const QString &fileName,  int line, int column = 0,
                                 Id editorId = Id(), OpenEditorFlags flags = NoFlags,
                                 bool *newEditor = 0);
    static IEditor *openEditorWithContents(Id editorId, QString *titlePattern = 0,
                                           const QByteArray &contents = QByteArray(),
                                           const QString &uniqueId = QString(),
                                           OpenEditorFlags flags = NoFlags);

    static bool openExternalEditor(const QString &fileName, Id editorId);

    static QStringList getOpenFileNames();

    static IDocument *currentDocument();
    static IEditor *currentEditor();
    static QList<IEditor *> visibleEditors();

    static void activateEditor(IEditor *editor, OpenEditorFlags flags = 0);
    static void activateEditorForEntry(DocumentModel::Entry *entry, OpenEditorFlags flags = 0);
    static IEditor *activateEditorForDocument(IDocument *document, OpenEditorFlags flags = 0);

    static bool closeDocument(IDocument *document, bool askAboutModifiedEditors = true);
    static bool closeDocuments(const QList<IDocument *> &documents, bool askAboutModifiedEditors = true);
    static void closeDocument(DocumentModel::Entry *entry);
    static void closeOtherDocuments(IDocument *document);

    static void addCurrentPositionToNavigationHistory(const QByteArray &saveState = QByteArray());
    static void cutForwardNavigationHistory();

    static bool saveDocument(IDocument *document);

    static bool closeEditors(const QList<IEditor *> &editorsToClose, bool askAboutModifiedEditors = true);
    static void closeEditor(IEditor *editor, bool askAboutModifiedEditors = true);

    static QByteArray saveState();
    static bool restoreState(const QByteArray &state);
    static bool hasSplitter();

    static void showEditorStatusBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText = QString(),
                           QObject *object = 0, const char *member = 0);
    static void hideEditorStatusBar(const QString &id);

    static EditorFactoryList editorFactories(const Utils::MimeType &mimeType, bool bestMatchOnly = true);
    static ExternalEditorList externalEditors(const Utils::MimeType &mimeType, bool bestMatchOnly = true);

    static bool isAutoSaveFile(const QString &fileName);

    static QTextCodec *defaultTextCodec();

    static qint64 maxTextFileSize();

    static void setWindowTitleAdditionHandler(WindowTitleHandler handler);
    static void setWindowTitleVcsTopicHandler(WindowTitleHandler handler);

    static void addSaveAndCloseEditorActions(QMenu *contextMenu, DocumentModel::Entry *entry,
                                             IEditor *editor = 0);
    static void addNativeDirAndOpenWithActions(QMenu *contextMenu, DocumentModel::Entry *entry);
    static void populateOpenWithMenu(QMenu *menu, const QString &fileName);

public: // for tests
    static IDocument::ReloadSetting reloadSetting();
    static void setReloadSetting(IDocument::ReloadSetting behavior);

signals:
    void currentEditorChanged(Core::IEditor *editor);
    void currentDocumentStateChanged();
    void editorCreated(Core::IEditor *editor, const QString &fileName);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void editorsClosed(QList<Core::IEditor *> editors);
    void findOnFileSystemRequest(const QString &path);

public slots:
    static void saveDocument();
    static void saveDocumentAs();
    static void revertToSaved();
    static bool closeAllEditors(bool askAboutModifiedEditors = true);
    static void slotCloseCurrentEditorOrDocument();
    static void closeOtherDocuments();
    static void splitSideBySide();
    static void gotoOtherSplit();
    static void goBackInNavigationHistory();
    static void goForwardInNavigationHistory();
    static void updateWindowTitles();

private:
    explicit EditorManager(QObject *parent);
    ~EditorManager();

    friend class Core::Internal::MainWindow;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditorManager::OpenEditorFlags)

#endif // EDITORMANAGER_H
