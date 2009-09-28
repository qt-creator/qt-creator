/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <coreplugin/icorelistener.h>

#include <QtGui/QWidget>
#include <QtCore/QList>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QSettings;
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
    EditorManagerPlaceHolder(Core::IMode *mode, QWidget *parent = 0);
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

    enum OpenEditorFlag {
        NoActivate = 1,
        IgnoreNavigationHistory = 2,
        NoModeSwitch = 4
    };
    Q_DECLARE_FLAGS(OpenEditorFlags, OpenEditorFlag)

    IEditor *openEditor(const QString &fileName,
                        const QString &editorKind = QString(),
                        OpenEditorFlags flags = 0);
    IEditor *openEditorWithContents(const QString &editorKind,
                     QString *titlePattern = 0,
                     const QString &contents = QString());

    bool openExternalEditor(const QString &fileName, const QString &editorKind);
    
    QStringList getOpenFileNames() const;
    QString getOpenWithEditorKind(const QString &fileName, bool *isExternalEditor = 0) const;

    void ensureEditorManagerVisible();
    bool hasEditor(const QString &fileName) const;
    QList<IEditor *> editorsForFileName(const QString &filename) const;
    QList<IEditor *> editorsForFile(IFile *file) const;

    IEditor *currentEditor() const;
    IEditor *activateEditor(IEditor *editor, OpenEditorFlags flags = 0);
    IEditor *activateEditor(const QModelIndex &index, Internal::EditorView *view = 0, OpenEditorFlags = 0);
    IEditor *activateEditor(Core::Internal::EditorView *view, Core::IFile*file, OpenEditorFlags flags = 0);

    QList<IEditor*> openedEditors() const;

    OpenEditorsModel *openedEditorsModel() const;
    void closeEditor(const QModelIndex &index);
    void closeOtherEditors(IEditor *editor);

    QList<IEditor*> editorsForFiles(QList<IFile*> files) const;
    //QList<EditorGroup *> editorGroups() const;
    void addCurrentPositionToNavigationHistory(IEditor *editor = 0, const QByteArray &saveState = QByteArray());

    bool saveEditor(IEditor *editor);

    bool closeEditors(const QList<IEditor *> editorsToClose, bool askAboutModifiedEditors = true);

    MakeWritableResult makeEditorWritable(IEditor *editor);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    IEditor *restoreEditor(QString fileName, QString editorKind, EditorGroup *group);

    void saveSettings();
    void readSettings();

    Internal::OpenEditorsWindow *windowPopup() const;
    void showWindowPopup() const;

    void showEditorInfoBar(const QString &kind,
                           const QString &infoText,
                           const QString &buttonText = QString(),
                           QObject *object = 0, const char *member = 0);

    void hideEditorInfoBar(const QString &kind);

    void showEditorStatusBar(const QString &kind,
                           const QString &infoText,
                           const QString &buttonText = QString(),
                           QObject *object = 0, const char *member = 0);

    void hideEditorStatusBar(const QString &kind);

    EditorFactoryList editorFactories(const MimeType &mimeType, bool bestMatchOnly = true) const;
    ExternalEditorList externalEditors(const MimeType &mimeType, bool bestMatchOnly = true) const;

    void setExternalEditor(const QString &);
    QString externalEditor() const;
    QString defaultExternalEditor() const;
    QString externalEditorHelpText() const;

    void setReloadBehavior(IFile::ReloadBehavior behavior);
    IFile::ReloadBehavior reloadBehavior() const;

    // Helper to display a message dialog when encountering a read-only
    // file, prompting the user about how to make it writeable.
    enum ReadOnlyAction { RO_Cancel, RO_OpenVCS, RO_MakeWriteable, RO_SaveAs };

    static ReadOnlyAction promptReadOnlyFile(const QString &fileName,
                                             const IVersionControl *versionControl,
                                             QWidget *parent,
                                             bool displaySaveAsButton = false);

signals:
    void currentEditorChanged(Core::IEditor *editor);
    void editorCreated(Core::IEditor *editor, const QString &fileName);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void editorsClosed(QList<Core::IEditor *> editors);

public slots:
    bool closeAllEditors(bool askAboutModifiedEditors = true);
    void openInExternalEditor();

    bool saveFile(Core::IEditor *editor = 0);
    bool saveFileAs(Core::IEditor *editor = 0);
    void revertToSaved();
    void closeEditor();
    void closeOtherEditors();

private slots:
    void gotoNextDocHistory();
    void gotoPreviousDocHistory();
    void handleContextChange(Core::IContext *context);
    void updateActions();
    void makeCurrentEditorWritable();

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
                        const QString &editorKind = QString(),
                        OpenEditorFlags flags = 0);
    Core::Internal::SplitterOrView *currentSplitterOrView() const;

    void closeEditor(Core::IEditor *editor);
    void closeDuplicate(Core::IEditor *editor);
    void closeView(Core::Internal::EditorView *view);
    void emptyView(Core::Internal::EditorView *view);
    Core::Internal::EditorView *currentEditorView() const;
    IEditor *pickUnusedEditor() const;


    static EditorManager *m_instance;
    EditorManagerPrivate *m_d;

    friend class Core::Internal::SplitterOrView;
    friend class Core::Internal::EditorView;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditorManager::OpenEditorFlags);


//===================EditorClosingCoreListener======================

namespace Core {
namespace Internal {

class EditorClosingCoreListener : public ICoreListener
{
    Q_OBJECT

public:
    EditorClosingCoreListener(EditorManager *em);
    bool editorAboutToClose(IEditor *editor);
    bool coreAboutToClose();

private:
    EditorManager *m_em;
};

} // namespace Internal
} // namespace Core

#endif // EDITORMANAGER_H
