/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <coreplugin/editormanager/ieditor.h>

#include <QAbstractButton>

#include <functional>

QT_BEGIN_NAMESPACE
class QItemSelection;
class QToolButton;
QT_END_NAMESPACE

namespace qmt {
class MElement;
class MPackage;
class MDiagram;
class DElement;
class DocumentController;
}

namespace ModelEditor {
namespace Internal {

class UiController;
class ActionHandler;
class DiagramsViewManager;

enum class SelectedArea {
    Nothing,
    Diagram,
    TreeView
};

class ModelEditor :
        public Core::IEditor
{
    Q_OBJECT
    class ModelEditorPrivate;

public:
    explicit ModelEditor(UiController *uiController, ActionHandler *actionHandler,
                         QWidget *parent = 0);
    ~ModelEditor();

    Core::IDocument *document() override;
    QWidget *toolBar() override;
    QByteArray saveState() const override;
    bool restoreState(const QByteArray &state) override;

    qmt::MDiagram *currentDiagram() const;
    void showDiagram(qmt::MDiagram *diagram);
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void removeSelectedElements();
    void deleteSelectedElements();
    void selectAll();
    void openParentDiagram();
    void editProperties();
    void editSelectedItem();
    void exportDiagram();
    void zoomIn();
    void zoomOut();
    void resetZoom();

    qmt::MPackage *guessSelectedPackage() const;

private:
    void init(QWidget *parent);
    void initDocument();

    void updateSelectedArea(SelectedArea selectedArea);
    void showProperties(const QList<qmt::MElement *> &modelElements);
    void showProperties(qmt::MDiagram *diagram, const QList<qmt::DElement *> &diagramElements);
    void clearProperties();
    void expandModelTreeToDepth(int depth);
    QToolButton *createToolbarCommandButton(const Core::Id &id, const std::function<void()> &slot,
                                        const QIcon &icon,
                                        const QString &toolTipBase, QWidget *parent);
    bool updateButtonIconByTheme(QAbstractButton *button, const QString &name);
    void showZoomIndicator();

    void onAddPackage();
    void onAddComponent();
    void onAddClass();
    void onAddCanvasDiagram();
    void onCurrentEditorChanged(Core::IEditor *editor);
    void onCanUndoChanged(bool canUndo);
    void onCanRedoChanged(bool canRedo);
    void onTreeModelReset();
    void onTreeViewSelectionChanged(const QItemSelection &selected,
                                    const QItemSelection &deselected);
    void onTreeViewActivated();
    void onTreeViewDoubleClicked(const QModelIndex &index);
    void onCurrentDiagramChanged(const qmt::MDiagram *diagram);
    void onDiagramActivated(const qmt::MDiagram *diagram);
    void onDiagramClipboardChanged(bool isEmpty);
    void onNewElementCreated(qmt::DElement *element, qmt::MDiagram *diagram);
    void onDiagramSelectionChanged(const qmt::MDiagram *diagram);
    void onDiagramModified(const qmt::MDiagram *diagram);
    void onRightSplitterMoved(int pos, int index);
    void onRightSplitterChanged(const QByteArray &state);
    void onRightHorizSplitterMoved(int pos, int index);
    void onRightHorizSplitterChanged(const QByteArray &state);
    void onToolbarSelectionChanged();

    void initToolbars();
    void openDiagram(qmt::MDiagram *diagram, bool addToHistory);
    void closeCurrentDiagram(bool addToHistory);
    void closeDiagram(const qmt::MDiagram *diagram);
    void closeAllDiagrams();

    void onContentSet();

    void addDiagramToSelector(const qmt::MDiagram *diagram);
    void updateDiagramSelector();
    void onDiagramSelectorSelected(int index);
    QString buildDiagramLabel(const qmt::MDiagram *diagram);
    void storeToolbarIdInDiagram(qmt::MDiagram *diagram);

    void addToNavigationHistory(const qmt::MDiagram *diagram);
    QByteArray saveState(const qmt::MDiagram *diagram) const;

    void onEditSelectedElement();
    bool isSyncBrowserWithDiagram() const;
    bool isSyncDiagramWithBrowser() const;
    void synchronizeDiagramWithBrowser();
    void synchronizeBrowserWithDiagram(const qmt::MDiagram *diagram);

private:
    ModelEditorPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
