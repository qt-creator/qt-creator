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

#include "../core_global.h"
#include "../idocument.h"

#include "documentmodel.h"
#include "ieditor.h"

#include "utils/textfileformat.h"

#include <QList>
#include <QWidget>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Utils { class MimeType; }

namespace Core {

class IDocument;
class SearchResultItem;

enum MakeWritableResult {
    OpenedWithVersionControl,
    MadeWritable,
    SavedAs,
    Failed
};

namespace Internal {
class EditorManagerPrivate;
class MainWindow;
} // namespace Internal

class CORE_EXPORT EditorManagerPlaceHolder final : public QWidget
{
    Q_OBJECT
public:
    explicit EditorManagerPlaceHolder(QWidget *parent = nullptr);
    ~EditorManagerPlaceHolder() final;

protected:
    void showEvent(QShowEvent *event) override;
};

class CORE_EXPORT EditorManager : public QObject
{
    Q_OBJECT

public:
    using WindowTitleHandler = std::function<QString (const QString &)>;

    static EditorManager *instance();

    enum OpenEditorFlag {
        NoFlags = 0,
        DoNotChangeCurrentEditor = 1,
        IgnoreNavigationHistory = 2,
        DoNotMakeVisible = 4,
        CanContainLineAndColumnNumber = 8,
        OpenInOtherSplit = 16,
        DoNotSwitchToDesignMode = 32,
        DoNotSwitchToEditMode = 64,
        SwitchSplitIfAlreadyVisible = 128
    };
    Q_DECLARE_FLAGS(OpenEditorFlags, OpenEditorFlag)

    struct FilePathInfo {
        QString filePath; // file path without line/column suffix
        QString postfix; // line/column suffix as string, e.g. ":10:1"
        int lineNumber; // extracted line number, -1 if none
        int columnNumber; // extracted column number, -1 if none
    };
    static FilePathInfo splitLineAndColumnNumber(const QString &filePath);
    static IEditor *openEditor(const QString &fileName, Utils::Id editorId = {},
        OpenEditorFlags flags = NoFlags, bool *newEditor = nullptr);
    static IEditor *openEditorAt(const QString &fileName,  int line, int column = 0,
                                 Utils::Id editorId = {}, OpenEditorFlags flags = NoFlags,
                                 bool *newEditor = nullptr);
    static void openEditorAtSearchResult(const SearchResultItem &item, OpenEditorFlags flags = NoFlags);
    static IEditor *openEditorWithContents(Utils::Id editorId, QString *titlePattern = nullptr,
                                           const QByteArray &contents = QByteArray(),
                                           const QString &uniqueId = QString(),
                                           OpenEditorFlags flags = NoFlags);
    static bool skipOpeningBigTextFile(const QString &filePath);
    static void clearUniqueId(IDocument *document);

    static bool openExternalEditor(const QString &fileName, Utils::Id editorId);
    static void addCloseEditorListener(const std::function<bool(IEditor *)> &listener);

    static QStringList getOpenFileNames();

    static IDocument *currentDocument();
    static IEditor *currentEditor();
    static QList<IEditor *> visibleEditors();

    static void activateEditor(IEditor *editor, OpenEditorFlags flags = NoFlags);
    static void activateEditorForEntry(DocumentModel::Entry *entry, OpenEditorFlags flags = NoFlags);
    static IEditor *activateEditorForDocument(IDocument *document, OpenEditorFlags flags = NoFlags);

    static bool closeDocument(IDocument *document, bool askAboutModifiedEditors = true);
    static bool closeDocuments(const QList<IDocument *> &documents, bool askAboutModifiedEditors = true);
    static void closeDocument(DocumentModel::Entry *entry);
    static bool closeDocuments(const QList<DocumentModel::Entry *> &entries);
    static void closeOtherDocuments(IDocument *document);
    static bool closeAllDocuments();

    static void addCurrentPositionToNavigationHistory(const QByteArray &saveState = QByteArray());
    static void setLastEditLocation(const IEditor *editor);
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
                                    QObject *object = nullptr,
                                    const std::function<void()> &function = {});
    static void hideEditorStatusBar(const QString &id);

    static bool isAutoSaveFile(const QString &fileName);

    static QTextCodec *defaultTextCodec();

    static Utils::TextFileFormat::LineTerminationMode defaultLineEnding();

    static qint64 maxTextFileSize();

    static void setWindowTitleAdditionHandler(WindowTitleHandler handler);
    static void setSessionTitleHandler(WindowTitleHandler handler);
    static void setWindowTitleVcsTopicHandler(WindowTitleHandler handler);

    static void addSaveAndCloseEditorActions(QMenu *contextMenu, DocumentModel::Entry *entry,
                                             IEditor *editor = nullptr);
    static void addPinEditorActions(QMenu *contextMenu, DocumentModel::Entry *entry);
    static void addNativeDirAndOpenWithActions(QMenu *contextMenu, DocumentModel::Entry *entry);
    static void populateOpenWithMenu(QMenu *menu, const QString &fileName);

public: // for tests
    static IDocument::ReloadSetting reloadSetting();
    static void setReloadSetting(IDocument::ReloadSetting behavior);

signals:
    void currentEditorChanged(Core::IEditor *editor);
    void currentDocumentStateChanged();
    void documentStateChanged(Core::IDocument *document);
    void editorCreated(Core::IEditor *editor, const QString &fileName);
    void editorOpened(Core::IEditor *editor);
    void documentOpened(Core::IDocument *document);
    void editorAboutToClose(Core::IEditor *editor);
    void editorsClosed(QList<Core::IEditor *> editors);
    void documentClosed(Core::IDocument *document);
    void findOnFileSystemRequest(const QString &path);
    void openFileProperties(const Utils::FilePath &path);
    void aboutToSave(IDocument *document);
    void saved(IDocument *document);
    void autoSaved();
    void currentEditorAboutToChange(Core::IEditor *editor);

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
    ~EditorManager() override;

    friend class Core::Internal::MainWindow;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditorManager::OpenEditorFlags)
