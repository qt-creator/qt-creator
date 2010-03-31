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

#ifndef EDITORVIEW_H
#define EDITORVIEW_H

#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QWidget>
#include <QtCore/QPointer>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QLabel;
class QStackedWidget;
class QSplitter;
class QStackedLayout;
class QAction;
class QFrame;
QT_END_NAMESPACE

namespace Core {
class IContext;
class IFile;
class IEditor;
class OpenEditorsModel;
class EditorToolBar;

namespace Internal {

struct EditLocation {
    QPointer<IFile> file;
    QString fileName;
    QString id;
    QVariant state;
};

class EditorView : public QWidget
{
    Q_OBJECT

public:
    explicit EditorView(QWidget *parent = 0);
    virtual ~EditorView();

    int editorCount() const;
    void addEditor(IEditor *editor);
    void removeEditor(IEditor *editor);
    IEditor *currentEditor() const;
    void setCurrentEditor(IEditor *editor);

    bool hasEditor(IEditor *editor) const;

    QList<IEditor *> editors() const;
    void showEditorInfoBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText,
                           QObject *object, const char *buttonPressMember,
                           const char *cancelButtonPressMember = 0);
    void hideEditorInfoBar(const QString &id);

    void showEditorStatusBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText,
                           QObject *object, const char *member);
    void hideEditorStatusBar(const QString &id);

private slots:
    void closeView();
    void listSelectionActivated(int index);

private:
    void updateNavigatorActions();
    void updateToolBar(IEditor *editor);
    void checkProjectLoaded(IEditor *editor);

    EditorToolBar *m_toolBar;

    QStackedWidget *m_container;
    QString m_infoWidgetId;
    QFrame *m_infoWidget;
    QLabel *m_infoWidgetLabel;
    QToolButton *m_infoWidgetButton;
    QToolButton *m_infoWidgetCloseButton;
    IEditor *m_editorForInfoWidget;
    QString m_statusWidgetId;
    QFrame *m_statusHLine;
    QFrame *m_statusWidget;
    QLabel *m_statusWidgetLabel;
    QToolButton *m_statusWidgetButton;
    QList<IEditor *> m_editors;
    QMap<QWidget *, IEditor *> m_widgetEditorMap;

    QList<EditLocation> m_navigationHistory;
    QList<EditLocation> m_editorHistory;
    int m_currentNavigationHistoryPosition;
    void updateCurrentPositionInNavigationHistory();

public:
    inline bool canGoForward() const { return m_currentNavigationHistoryPosition < m_navigationHistory.size()-1; }
    inline bool canGoBack() const { return m_currentNavigationHistoryPosition > 0; }

public slots:
    void goBackInNavigationHistory();
    void goForwardInNavigationHistory();

public:
    void addCurrentPositionToNavigationHistory(IEditor *editor = 0, const QByteArray &saveState = QByteArray());
    inline QList<EditLocation> editorHistory() const { return m_editorHistory; }

    void copyNavigationHistoryFrom(EditorView* other);
    void updateEditorHistory(IEditor *editor);
};

class SplitterOrView  : public QWidget
{
    Q_OBJECT
public:
    explicit SplitterOrView(OpenEditorsModel *model); // creates a root splitter
    explicit SplitterOrView(Core::IEditor *editor = 0);
    ~SplitterOrView();

    void split(Qt::Orientation orientation);
    void unsplit();

    inline bool isView() const { return m_view != 0; }
    inline bool isRoot() const { return m_isRoot; }

    inline bool isSplitter() const { return m_splitter != 0; }
    inline Core::IEditor *editor() const { return m_view ? m_view->currentEditor() : 0; }
    inline QList<Core::IEditor *> editors() const { return m_view ? m_view->editors() : QList<Core::IEditor*>(); }
    inline bool hasEditor(Core::IEditor *editor) const { return m_view && m_view->hasEditor(editor); }
    inline bool hasEditors() const { return m_view && m_view->editorCount() != 0; }
    inline EditorView *view() const { return m_view; }
    inline QSplitter *splitter() const { return m_splitter; }
    QSplitter *takeSplitter();
    EditorView *takeView();

    QByteArray saveState() const;
    void restoreState(const QByteArray &);

    SplitterOrView *findView(Core::IEditor *editor);
    SplitterOrView *findView(EditorView *view);
    SplitterOrView *findFirstView();
    SplitterOrView *findEmptyView();
    SplitterOrView *findSplitter(Core::IEditor *editor);
    SplitterOrView *findSplitter(SplitterOrView *child);

    SplitterOrView *findNextView(SplitterOrView *view);

    QSize sizeHint() const { return minimumSizeHint(); }
    QSize minimumSizeHint() const;

    void unsplitAll();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *e);

private:
    void unsplitAll_helper();
    SplitterOrView *findNextView_helper(SplitterOrView *view, bool *found);
    bool m_isRoot;
    QStackedLayout *m_layout;
    EditorView *m_view;
    QSplitter *m_splitter;
};

}
}
#endif // EDITORVIEW_H
