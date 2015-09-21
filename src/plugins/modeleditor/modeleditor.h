/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef MODELDIAGRAMEDITOR_H
#define MODELDIAGRAMEDITOR_H

#include <coreplugin/editormanager/ieditor.h>

#include <QAbstractButton>

#include <functional>

QT_BEGIN_NAMESPACE
class QItemSelection;
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
    void editProperties();
    qmt::MPackage *guessSelectedPackage() const;

private:
    void init(QWidget *parent);
    void initDocument();

    void updateSelectedArea(SelectedArea selectedArea);
    void showProperties(const QList<qmt::MElement *> &modelElements);
    void showProperties(qmt::MDiagram *diagram, const QList<qmt::DElement *> &diagramElements);
    void clearProperties();
    void expandModelTreeToDepth(int depth);
    QWidget *createToolbarCommandButton(const Core::Id &id, const std::function<void()> &slot,
                                        const QIcon &icon,
                                        const QString &toolTipBase, QWidget *parent);
    bool updateButtonIconByTheme(QAbstractButton *button, const QString &name);

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
    Q_SLOT void onEditSelectedElement();

    void onRightSplitterMoved(int pos, int index);
    void onRightSplitterChanged(const QByteArray &state);
    void onRightHorizSplitterMoved(int pos, int index);
    void onRightHorizSplitterChanged(const QByteArray &state);

    void initToolbars();
    void closeCurrentDiagram();
    void closeDiagram(const qmt::MDiagram *diagram);
    void closeAllDiagrams();

    Q_SLOT void onContentSet();

private:
    ModelEditorPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor

#endif // MODELDIAGRAMEDITOR_H
