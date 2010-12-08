/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef EDITORMANAGER_H
#define EDITORMANAGER_H

#include "../core_global.h"

#include <coreplugin/ifile.h> // enumerations

#include <QtGui/QWidget>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Core {

class EditorGroup;
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
    typedef QList<IEditorFactory*> EditorFactoryList;
    typedef QList<IExternalEditor*> ExternalEditorList;

    explicit EditorManager(ICore *core, QWidget *parent);
    virtual ~EditorManager();
    void init();
    static EditorManager *instance() { return m_instance; }

    static EditorToolBar *createToolBar(QWidget *parent = 0);

    enum OpenEditorFlag {
        NoActivate = 1,
        IgnoreNavigationHistory = 2,
        ModeSwitch = 4,
        CanContainLineNumber = 8
    };
    Q_DECLARE_FLAGS(OpenEditorFlags, OpenEditorFlag)

    IEditor *openEditor(const QString &fileName,
                        const QString &editorId = QString(),
                        OpenEditorFlags flags = 0,
                        bool *newEditor = 0);
    IEditor *openEditorWithContents(const QString &editorId,
                     QString *titlePattern = 0,
                     const QString &contents = QString());

    bool openExternalEditor(const QString &fileName, const QString &editorId);

    QStringList getOpenFileNames() const;
    QString getOpenWithEditorId(const QString &fileName, bool *isExternalEditor = 0) const;

    void switchToPreferedMode();
    bool hasEditor(const QString &fileName) const;
    QList<IEditor *> editorsForFileName(const QString &filename) const;
    QList<IEditor *> editorsForFile(IFile *file) const;

    IEditor *currentEditor() const;
    QList<IEditor *> visibleEditors() const;
    QList<IEditor*> openedEditors() const;

    IEditor *activateEditor(IEditor *editor, OpenEditorFlags flags = 0);
    IEditor *activateEditor(const QModelIndex &index, Internal::EditorView *view = 0, OpenEditorFlags = 0);
    IEditor *activateEditor(Core::Internal::EditorView *view, Core::IFile*file, OpenEditorFlags flags = 0);

    OpenEditorsModel *openedEditorsModel() const;
    void closeEditor(const QModelIndex &index);
    void closeOtherEditors(IEditor *editor);

    QList<IEditor*> editorsForFiles(QList<IFile*> files) const;
    void addCurrentPositionToNavigationHistory(IEditor *editor = 0, const QByteArray &saveState = QByteArray());
    void cutForwardNavigationHistory();

    bool saveEditor(IEditor *editor);

    bool closeEditors(const QList<IEditor *> &editorsToClose, bool askAboutModifiedEditors = true);

    MakeWritableResult makeFileWritable(IFile *file);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    bool hasSplitter() const;

    IEditor *restoreEditor(QString fileName, QString editorId, EditorGroup *group);

    void saveSettings();
    void readSettings();

    Internal::OpenEditorsWindow *windowPopup() const;
    void showPopupOrSelectDocument() const;

    void showEditorInfoBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText = QString(),
                           QObject *object = 0, const char *buttonPressMember = 0,
                           const char *cancelButtonPressMember = 0);

    void hideEditorInfoBar(const QString &id);

    void showEditorStatusBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText = QString(),
                           QObject *object = 0, const char *member = 0);

    void hideEditorStatusBar(const QString &id);

    EditorFactoryList editorFactories(const MimeType &mimeType, bool bestMatchOnly = true) const;
    ExternalEditorList externalEditors(const MimeType &mimeType, bool bestMatchOnly = true) const;

    void setExternalEditor(const QString &);
    QString externalEditor() const;
    QString defaultExternalEditor() const;
    QString externalEditorHelpText() const;

    void setReloadSetting(IFile::ReloadSetting behavior);
    IFile::ReloadSetting reloadSetting() const;

    void setUtf8BomSetting(IFile::Utf8BomSetting behavior);
    IFile::Utf8BomSetting utf8BomSetting() const;

    QTextCodec *defaultTextEncoding() const;

    static qint64 maxTextFileSize();

    void setWindowTitleAddition(const QString &addition);
    QString windowTitleAddition() const;

signals:
    void currentEditorChanged(Core::IEditor *editor);
    void currentEditorStateChanged(Core::IEditor *editor);
    void editorCreated(Core::IEditor *editor, const QString &fileName);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void editorsClosed(QList<Core::IEditor *> editors);

public slots:
    bool closeAllEditors(bool askAboutModifiedEditors = true);
    void openInExternalEditor();

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
    void updateWindowTitle();
    void handleEditorStateChange();

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
    IEditor *createEditor(const QString &mimeType = QString(),
                          const QString &fileName = QString());
    void addEditor(IEditor *editor, bool isDuplicate = false);
    void removeEditor(IEditor *editor);

    void restoreEditorState(IEditor *editor);

    IEditor *placeEditor(Core::Internal::EditorView *view, Core::IEditor *editor);
    Core::IEditor *duplicateEditor(IEditor *editor);
    void setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory = false);
    void setCurrentView(Core::Internal::SplitterOrView *view);
    IEditor *activateEditor(Core::Internal::EditorView *view, Core::IEditor *editor, OpenEditorFlags flags = 0);
    IEditor *openEditor(Core::Internal::EditorView *view, const QString &fileName,
                        const QString &editorId = QString(),
                        OpenEditorFlags flags = 0,
                        bool *newEditor = 0);
    Core::Internal::SplitterOrView *currentSplitterOrView() const;

    void closeEditor(Core::IEditor *editor);
    void closeDuplicate(Core::IEditor *editor);
    void closeView(Core::Internal::EditorView *view);
    void emptyView(Core::Internal::EditorView *view);
    Core::Internal::EditorView *currentEditorView() const;
    IEditor *pickUnusedEditor() const;
    void addFileToRecentFiles(IFile *file);

    static EditorManager *m_instance;
    EditorManagerPrivate *m_d;

    friend class Core::Internal::SplitterOrView;
    friend class Core::Internal::EditorView;
    friend class Core::EditorToolBar;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditorManager::OpenEditorFlags);
#endif // EDITORMANAGER_H
