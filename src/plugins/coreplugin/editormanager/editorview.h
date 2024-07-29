// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/dropsupport.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <QList>
#include <QMap>
#include <QPointer>
#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QFrame;
class QLabel;
class QMenu;
class QSplitter;
class QStackedLayout;
class QStackedWidget;
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
class InfoBarDisplay;
}

namespace Core {
class IDocument;
class IEditor;
class EditorToolBar;

namespace Internal {

class EditLocation
{
public:
    QByteArray save() const;
    static EditLocation load(const QByteArray &data);
    static EditLocation forEditor(const IEditor *editor, const QByteArray &saveState = {});

    QString displayName() const;

    QPointer<IDocument> document;
    Utils::FilePath filePath;
    Utils::Id id;
    QByteArray state;
};

class SplitterOrView;

class EditorView : public QWidget
{
    Q_OBJECT

public:
    explicit EditorView(SplitterOrView *parentSplitterOrView, QWidget *parent = nullptr);
    ~EditorView() override;

    SplitterOrView *parentSplitterOrView() const;
    EditorView *findNextView() const;
    EditorView *findPreviousView() const;

    int editorCount() const;
    void addEditor(IEditor *editor);
    void removeEditor(IEditor *editor);
    IEditor *currentEditor() const;
    void setCurrentEditor(IEditor *editor);

    bool hasEditor(IEditor *editor) const;

    QList<IEditor *> editors() const;
    IEditor *editorForDocument(const IDocument *document) const;

    void showEditorStatusBar(const QString &id,
                           const QString &infoText,
                           const QString &buttonText,
                           QObject *object, const std::function<void()> &function);
    void hideEditorStatusBar(const QString &id);
    void setCloseSplitEnabled(bool enable);
    void setCloseSplitIcon(const QIcon &icon);

    bool canGoForward() const;
    bool canGoBack() const;
    bool canReopen() const;

    void goBackInNavigationHistory();
    void goForwardInNavigationHistory();

    void reopenLastClosedDocument();

    void goToEditLocation(const EditLocation &location);

    void addCurrentPositionToNavigationHistory(const QByteArray &saveState = QByteArray());
    void addClosedEditorToCloseHistory(IEditor *editor);
    void cutForwardNavigationHistory();

    QList<EditLocation> editorHistory() const { return m_editorHistory; }

    void copyNavigationHistoryFrom(EditorView* other);
    void updateEditorHistory(IEditor *editor);
    static void updateEditorHistory(IEditor *editor, QList<EditLocation> &history);

signals:
    void currentEditorChanged(Core::IEditor *editor);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void focusInEvent(QFocusEvent *) override;
    bool event(QEvent *e) override;

private:
    friend class SplitterOrView; // for setParentSplitterOrView

    void closeCurrentEditor();
    void listSelectionActivated(int index);
    void splitHorizontally();
    void splitVertically();
    void splitNewWindow();
    void closeSplit();
    void openDroppedFiles(const QList<Utils::DropSupport::FileSpec> &files);

    void setParentSplitterOrView(SplitterOrView *splitterOrView);

    void fillListContextMenu(QMenu *menu) const;
    void updateNavigatorActions();
    void updateToolBar(IEditor *editor);
    void checkProjectLoaded(IEditor *editor);

    void updateCurrentPositionInNavigationHistory();
    bool openEditorFromNavigationHistory(int index);
    void goToNavigationHistory(int index);

    SplitterOrView *m_parentSplitterOrView;
    EditorToolBar *m_toolBar;

    QStackedWidget *m_container;
    Utils::InfoBarDisplay *m_infoBarDisplay;
    QString m_statusWidgetId;
    QWidget *m_statusHLine;
    QFrame *m_statusWidget;
    QLabel *m_statusWidgetLabel;
    QToolButton *m_statusWidgetButton;
    QList<IEditor *> m_editors;
    QHash<QWidget *, IEditor *> m_widgetEditorMap;
    QLabel *m_emptyViewLabel;

    QList<EditLocation> m_navigationHistory;
    QMenu *m_backMenu;
    QMenu *m_forwardMenu;
    QList<EditLocation> m_editorHistory;
    QList<EditLocation> m_closedEditorHistory;
    int m_currentNavigationHistoryPosition = 0;
};

class SplitterOrView  : public QWidget
{
    Q_OBJECT
public:
    explicit SplitterOrView(IEditor *editor = nullptr);
    explicit SplitterOrView(EditorView *view);
    ~SplitterOrView() override;

    void split(Qt::Orientation orientation, bool activateView = true);
    void unsplit();

    bool isView() const { return m_view != nullptr; }
    bool isSplitter() const { return m_splitter != nullptr; }

    IEditor *editor() const { return m_view ? m_view->currentEditor() : nullptr; }
    QList<IEditor *> editors() const { return m_view ? m_view->editors() : QList<IEditor*>(); }
    bool hasEditor(IEditor *editor) const { return m_view && m_view->hasEditor(editor); }
    bool hasEditors() const { return m_view && m_view->editorCount() != 0; }
    EditorView *view() const { return m_view; }
    QSplitter *splitter() const { return m_splitter; }
    QSplitter *takeSplitter();
    EditorView *takeView();

    QByteArray saveState() const;
    void restoreState(const QByteArray &);

    EditorView *findFirstView() const;
    EditorView *findLastView() const;
    SplitterOrView *findParentSplitter() const;

    QSize sizeHint() const override { return minimumSizeHint(); }
    QSize minimumSizeHint() const override;

    void unsplitAll();

signals:
    void splitStateChanged();

private:
    const QList<IEditor *> unsplitAll_helper();
    QStackedLayout *m_layout;
    EditorView *m_view;
    QSplitter *m_splitter;
};

} // Internal
} // Core
