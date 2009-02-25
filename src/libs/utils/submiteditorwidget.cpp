/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "submiteditorwidget.h"
#include "ui_submiteditorwidget.h"

#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include <QtGui/QPushButton>

enum { debug = 0 };

namespace Core {
namespace Utils {

// QActionPushButton: A push button tied to an action
// (similar to a QToolButton)
class QActionPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit QActionPushButton(QAction *a);

private slots:
    void actionChanged();
};

QActionPushButton::QActionPushButton(QAction *a) :
     QPushButton(a->icon(), a->text())
{
    connect(a, SIGNAL(changed()), this, SLOT(actionChanged()));
    connect(this, SIGNAL(clicked()), a, SLOT(trigger()));
    setEnabled(a->isEnabled());
}

void QActionPushButton::actionChanged()
{
    if (const QAction *a = qobject_cast<QAction*>(sender()))
        setEnabled(a->isEnabled());
}

// Helpers to retrieve model data
static inline bool listModelChecked(const QAbstractItemModel *model, int row, int column = 0)
{
    const QModelIndex checkableIndex = model->index(row, column, QModelIndex());
    return model->data(checkableIndex, Qt::CheckStateRole).toInt() == Qt::Checked;
}

static inline QString listModelText(const QAbstractItemModel *model, int row, int column)
{
    const QModelIndex index = model->index(row, column, QModelIndex());
    return model->data(index, Qt::DisplayRole).toString();
}

// Find a check item in a model
static bool listModelContainsCheckedItem(const QAbstractItemModel *model)
{
    const int count = model->rowCount();
    for (int i = 0; i < count; i++)
        if (listModelChecked(model, i, 0))
            return true;
    return false;
}

// Convenience to extract a list of selected indexes
QList<int> selectedRows(const QAbstractItemView *view)
{
    const QModelIndexList indexList = view->selectionModel()->selectedRows(0);
    if (indexList.empty())
        return QList<int>();
    QList<int> rc;
    const QModelIndexList::const_iterator cend = indexList.constEnd();
    for (QModelIndexList::const_iterator it = indexList.constBegin(); it != cend; ++it)
        rc.push_back(it->row());
    return rc;
}

// -----------  SubmitEditorWidgetPrivate
struct SubmitEditorWidgetPrivate
{
    SubmitEditorWidgetPrivate();

    Ui::SubmitEditorWidget m_ui;
    bool m_filesSelected;
    bool m_filesChecked;
    int m_fileNameColumn;
    int m_activatedRow;
};

SubmitEditorWidgetPrivate::SubmitEditorWidgetPrivate() :
    m_filesSelected(false),
    m_filesChecked(false),
    m_fileNameColumn(1),
    m_activatedRow(-1)
{
}

SubmitEditorWidget::SubmitEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_d(new SubmitEditorWidgetPrivate)
{
    m_d->m_ui.setupUi(this);
    // File List
    m_d->m_ui.fileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_d->m_ui.fileView->setRootIsDecorated(false);
    connect(m_d->m_ui.fileView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(diffActivated(QModelIndex)));

    // Text
    m_d->m_ui.description->setFont(QFont(QLatin1String("Courier")));

    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(m_d->m_ui.description);
}

SubmitEditorWidget::~SubmitEditorWidget()
{
    delete m_d;
}

void SubmitEditorWidget::registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction, QAction *diffAction)
{
    if (editorUndoAction) {
        editorUndoAction->setEnabled(m_d->m_ui.description->document()->isUndoAvailable());
        connect(m_d->m_ui.description, SIGNAL(undoAvailable(bool)), editorUndoAction, SLOT(setEnabled(bool)));
        connect(editorUndoAction, SIGNAL(triggered()), m_d->m_ui.description, SLOT(undo()));
    }
    if (editorRedoAction) {
        editorRedoAction->setEnabled(m_d->m_ui.description->document()->isRedoAvailable());
        connect(m_d->m_ui.description, SIGNAL(redoAvailable(bool)), editorRedoAction, SLOT(setEnabled(bool)));
        connect(editorRedoAction, SIGNAL(triggered()), m_d->m_ui.description, SLOT(redo()));
    }

    if (submitAction) {
        if (debug) {
            int count = 0;
            if (const QAbstractItemModel *model = m_d->m_ui.fileView->model())
                count = model->rowCount();
            qDebug() << submitAction << count << "items" << m_d->m_filesChecked;
        }
        submitAction->setEnabled(m_d->m_filesChecked);
        connect(this, SIGNAL(fileCheckStateChanged(bool)), submitAction, SLOT(setEnabled(bool)));
        m_d->m_ui.buttonLayout->addWidget(new QActionPushButton(submitAction));
    }
    if (diffAction) {
        if (debug)
            qDebug() << diffAction << m_d->m_filesSelected;
        diffAction->setEnabled(m_d->m_filesSelected);
        connect(this, SIGNAL(fileSelectionChanged(bool)), diffAction, SLOT(setEnabled(bool)));
        connect(diffAction, SIGNAL(triggered()), this, SLOT(triggerDiffSelected()));
        m_d->m_ui.buttonLayout->addWidget(new QActionPushButton(diffAction));
    }
}

void SubmitEditorWidget::unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                                           QAction *submitAction, QAction *diffAction)
{
    if (editorUndoAction) {
        disconnect(m_d->m_ui.description, SIGNAL(undoAvailableChanged(bool)), editorUndoAction, SLOT(setEnabled(bool)));
        disconnect(editorUndoAction, SIGNAL(triggered()), m_d->m_ui.description, SLOT(undo()));
    }
    if (editorRedoAction) {
        disconnect(m_d->m_ui.description, SIGNAL(redoAvailableChanged(bool)), editorRedoAction, SLOT(setEnabled(bool)));
        disconnect(editorRedoAction, SIGNAL(triggered()), m_d->m_ui.description, SLOT(redo()));
    }

    if (submitAction)
        disconnect(this, SIGNAL(fileCheckStateChanged(bool)), submitAction, SLOT(setEnabled(bool)));

    if (diffAction) {
         disconnect(this, SIGNAL(fileSelectionChanged(bool)), diffAction, SLOT(setEnabled(bool)));
         disconnect(diffAction, SIGNAL(triggered()), this, SLOT(triggerDiffSelected()));
    }
}

QString SubmitEditorWidget::trimmedDescriptionText() const
{
    // Make sure we have one terminating NL
    QString text = descriptionText().trimmed();
    text += QLatin1Char('\n');
    return text;
}

QString SubmitEditorWidget::descriptionText() const
{
    return m_d->m_ui.description->toPlainText();
}

void SubmitEditorWidget::setDescriptionText(const QString &text)
{
    m_d->m_ui.description->setPlainText(text);
}

int SubmitEditorWidget::fileNameColumn() const
{
    return m_d->m_fileNameColumn;
}

void SubmitEditorWidget::setFileNameColumn(int c)
{
    m_d->m_fileNameColumn = c;
}

QAbstractItemView::SelectionMode SubmitEditorWidget::fileListSelectionMode() const
{
    return m_d->m_ui.fileView->selectionMode();
}

void SubmitEditorWidget::setFileListSelectionMode(QAbstractItemView::SelectionMode sm)
{
    m_d->m_ui.fileView->setSelectionMode(sm);
}

void SubmitEditorWidget::setFileModel(QAbstractItemModel *model)
{
    m_d->m_ui.fileView->clearSelection(); // trigger the change signals

    m_d->m_ui.fileView->setModel(model);

    if (model->rowCount()) {
        const int columnCount = model->columnCount();
        for (int c = 0;  c < columnCount; c++)
            m_d->m_ui.fileView->resizeColumnToContents(c);
    }

    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateSubmitAction()));
    connect(model, SIGNAL(modelReset()),
            this, SLOT(updateSubmitAction()));
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(updateSubmitAction()));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(updateSubmitAction()));
    connect(m_d->m_ui.fileView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(updateDiffAction()));
    updateActions();
}

QAbstractItemModel *SubmitEditorWidget::fileModel() const
{
    return m_d->m_ui.fileView->model();
}

QStringList SubmitEditorWidget::selectedFiles() const
{
    const QList<int> selection = selectedRows(m_d->m_ui.fileView);
    if (selection.empty())
        return QStringList();

    QStringList rc;
    const QAbstractItemModel *model = m_d->m_ui.fileView->model();
    const int count = selection.size();
    for (int i = 0; i < count; i++)
        rc.push_back(listModelText(model, selection.at(i), fileNameColumn()));
    return rc;
}

QStringList SubmitEditorWidget::checkedFiles() const
{
    QStringList rc;
    const QAbstractItemModel *model = m_d->m_ui.fileView->model();
    if (!model)
        return rc;
    const int count = model->rowCount();
    for (int i = 0; i < count; i++)
        if (listModelChecked(model, i, 0))
            rc.push_back(listModelText(model, i, fileNameColumn()));
    return rc;
}

QPlainTextEdit *SubmitEditorWidget::descriptionEdit() const
{
    return m_d->m_ui.description;
}

void SubmitEditorWidget::triggerDiffSelected()
{
    const QStringList sel = selectedFiles();
    if (!sel.empty())
        emit diffSelected(sel);
}

void SubmitEditorWidget::diffActivatedDelayed()
{
    const QStringList files = QStringList(listModelText(m_d->m_ui.fileView->model(), m_d->m_activatedRow, fileNameColumn()));
    emit diffSelected(files);
}

void SubmitEditorWidget::diffActivated(const QModelIndex &index)
{
    // We need to delay the signal, otherwise, the diff editor will not
    // be in the foreground.
    m_d->m_activatedRow = index.row();
    QTimer::singleShot(0, this, SLOT(diffActivatedDelayed()));
}

void SubmitEditorWidget::updateActions()
{
    updateSubmitAction();
    updateDiffAction();
}

// Enable submit depending on having checked files
void SubmitEditorWidget::updateSubmitAction()
{
    const bool newFilesCheckedState = hasCheckedFiles();
    if (m_d->m_filesChecked != newFilesCheckedState) {
        m_d->m_filesChecked = newFilesCheckedState;
        emit fileCheckStateChanged(m_d->m_filesChecked);
    }
}

// Enable diff depending on selected files
void SubmitEditorWidget::updateDiffAction()
{
    const bool filesSelected = hasSelection();
    if (m_d->m_filesSelected != filesSelected) {
        m_d->m_filesSelected = filesSelected;
        emit fileSelectionChanged(m_d->m_filesSelected);
    }
}

bool SubmitEditorWidget::hasSelection() const
{
    // Not present until model is set
    if (const QItemSelectionModel *sm = m_d->m_ui.fileView->selectionModel())
        return sm->hasSelection();
    return false;
}

bool SubmitEditorWidget::hasCheckedFiles() const
{
    if (const QAbstractItemModel *model = m_d->m_ui.fileView->model())
        return listModelContainsCheckedItem(model);
    return false;
}

void SubmitEditorWidget::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_d->m_ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

void SubmitEditorWidget::insertTopWidget(QWidget *w)
{
    m_d->m_ui.vboxLayout->insertWidget(0, w);
}

} // namespace Utils
} // namespace Core

#include "submiteditorwidget.moc"
