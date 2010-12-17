/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "submiteditorwidget.h"
#include "submitfieldwidget.h"
#include "ui_submiteditorwidget.h"

#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QScopedPointer>

#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QSpacerItem>

enum { debug = 0 };
enum { defaultLineWidth = 72 };

enum { checkableColumn = 0 };

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
    if (const QAction *a = qobject_cast<QAction*>(sender())) {
        setEnabled(a->isEnabled());
        setText(a->text());
    }
}

// A helper parented on a QAction,
// making QAction::setText() a slot (which it currently is not).
class QActionSetTextSlotHelper : public QObject
{
    Q_OBJECT
public:
    explicit QActionSetTextSlotHelper(QAction *a) : QObject(a) {}

public slots:
    void setText(const QString &t) {
        if (QAction *action = qobject_cast<QAction *>(parent()))
            action->setText(t);
    }
};

// Helpers to retrieve model data
static inline bool listModelChecked(const QAbstractItemModel *model, int row, int column = 0)
{
    const QModelIndex checkableIndex = model->index(row, column, QModelIndex());
    return model->data(checkableIndex, Qt::CheckStateRole).toInt() == Qt::Checked;
}

static void setListModelChecked(QAbstractItemModel *model, bool checked, int column = 0)
{
    const QVariant data = QVariant(int(checked ? Qt::Checked : Qt::Unchecked));
    const int count = model->rowCount();
    for (int i = 0; i < count; i++) {
        const QModelIndex checkableIndex = model->index(i, column, QModelIndex());
        model->setData(checkableIndex, data, Qt::CheckStateRole);
    }
}

static inline QString listModelText(const QAbstractItemModel *model, int row, int column)
{
    const QModelIndex index = model->index(row, column, QModelIndex());
    return model->data(index, Qt::DisplayRole).toString();
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
    // A pair of position/action to extend context menus
    typedef QPair<int, QPointer<QAction> > AdditionalContextMenuAction;

    SubmitEditorWidgetPrivate();

    Ui::SubmitEditorWidget m_ui;
    bool m_filesSelected;
    int m_fileNameColumn;
    int m_activatedRow;
    bool m_emptyFileListEnabled;

    QList<AdditionalContextMenuAction> descriptionEditContextMenuActions;
    QVBoxLayout *m_fieldLayout;
    QList<SubmitFieldWidget *> m_fieldWidgets;
    int m_lineWidth;

    bool m_commitEnabled;
};

SubmitEditorWidgetPrivate::SubmitEditorWidgetPrivate() :
    m_filesSelected(false),
    m_fileNameColumn(1),
    m_activatedRow(-1),
    m_emptyFileListEnabled(false),
    m_fieldLayout(0),
    m_lineWidth(defaultLineWidth),
    m_commitEnabled(false)
{
}

SubmitEditorWidget::SubmitEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_d(new SubmitEditorWidgetPrivate)
{
    m_d->m_ui.setupUi(this);
    m_d->m_ui.description->setContextMenuPolicy(Qt::CustomContextMenu);
    m_d->m_ui.description->setLineWrapMode(QTextEdit::NoWrap);
    m_d->m_ui.description->setWordWrapMode(QTextOption::WordWrap);
    connect(m_d->m_ui.description, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(editorCustomContextMenuRequested(QPoint)));
    connect(m_d->m_ui.description, SIGNAL(textChanged()),
            this, SLOT(updateSubmitAction()));

    // File List
    m_d->m_ui.fileView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_d->m_ui.fileView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(fileListCustomContextMenuRequested(QPoint)));
    m_d->m_ui.fileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_d->m_ui.fileView->setRootIsDecorated(false);
    connect(m_d->m_ui.fileView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(diffActivated(QModelIndex)));

    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(m_d->m_ui.description);
}

SubmitEditorWidget::~SubmitEditorWidget()
{
    delete m_d;
}

void SubmitEditorWidget::registerActions(QAction *editorUndoAction, QAction *editorRedoAction,
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
            qDebug() << Q_FUNC_INFO << submitAction << count << "items";
        }
        m_d->m_commitEnabled = !canSubmit();
        connect(this, SIGNAL(submitActionEnabledChanged(bool)), submitAction, SLOT(setEnabled(bool)));
        // Wire setText via QActionSetTextSlotHelper.
        QActionSetTextSlotHelper *actionSlotHelper = submitAction->findChild<QActionSetTextSlotHelper *>();
        if (!actionSlotHelper)
            actionSlotHelper = new QActionSetTextSlotHelper(submitAction);
        connect(this, SIGNAL(submitActionTextChanged(QString)), actionSlotHelper, SLOT(setText(QString)));
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

    if (submitAction) {
        disconnect(this, SIGNAL(submitActionEnabledChanged(bool)), submitAction, SLOT(setEnabled(bool)));
        // Just deactivate the QActionSetTextSlotHelper on the action
        disconnect(this, SIGNAL(submitActionTextChanged(QString)), 0, 0);
    }

    if (diffAction) {
         disconnect(this, SIGNAL(fileSelectionChanged(bool)), diffAction, SLOT(setEnabled(bool)));
         disconnect(diffAction, SIGNAL(triggered()), this, SLOT(triggerDiffSelected()));
    }
}

// Make sure we have one terminating NL. Do not trim front as leading space might be
// required for some formattings.
static inline QString trimMessageText(QString t)
{
    if (t.isEmpty())
        return t;
    // Trim back of string.
    const int last = t.size() - 1;
    int lastWordCharacter = last;
    for ( ; lastWordCharacter >= 0 && t.at(lastWordCharacter).isSpace() ; lastWordCharacter--) ;
    if (lastWordCharacter != last)
        t.truncate(lastWordCharacter + 1);
    t += QLatin1Char('\n');
    return t;
}

// Extract the wrapped text from a text edit, which performs
// the wrapping only optically.
static QString wrappedText(const QTextEdit *e)
{
    const QChar newLine = QLatin1Char('\n');
    QString rc;
    QTextCursor cursor(e->document());
    cursor.movePosition(QTextCursor::Start);
    while (!cursor.atEnd()) {
        cursor.select(QTextCursor::LineUnderCursor);
        rc += cursor.selectedText();
        rc += newLine;
        cursor.movePosition(QTextCursor::EndOfLine); // Mac needs it
        cursor.movePosition(QTextCursor::Right);
    }
    return rc;
}

QString SubmitEditorWidget::descriptionText() const
{
    QString rc = trimMessageText(lineWrap() ? wrappedText(m_d->m_ui.description) :
                                              m_d->m_ui.description->toPlainText());
    // append field entries
    foreach(const SubmitFieldWidget *fw, m_d->m_fieldWidgets)
        rc += fw->fieldValues();
    return cleanupDescription(rc);
}

void SubmitEditorWidget::setDescriptionText(const QString &text)
{
    m_d->m_ui.description->setPlainText(text);
}

bool SubmitEditorWidget::lineWrap() const
{
    return m_d->m_ui.description->lineWrapMode() != QTextEdit::NoWrap;
}

void SubmitEditorWidget::setLineWrap(bool v)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << v;
    if (v) {
        m_d->m_ui.description->setLineWrapColumnOrWidth(m_d->m_lineWidth);
        m_d->m_ui.description->setLineWrapMode(QTextEdit::FixedColumnWidth);
    } else {
        m_d->m_ui.description->setLineWrapMode(QTextEdit::NoWrap);
    }
}

int SubmitEditorWidget::lineWrapWidth() const
{
    return m_d->m_lineWidth;
}

void SubmitEditorWidget::setLineWrapWidth(int v)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << v << lineWrap();
    if (m_d->m_lineWidth == v)
        return;
    m_d->m_lineWidth = v;
    if (lineWrap())
        m_d->m_ui.description->setLineWrapColumnOrWidth(v);
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
        if (listModelChecked(model, i, checkableColumn))
            rc.push_back(listModelText(model, i, fileNameColumn()));
    return rc;
}

QTextEdit *SubmitEditorWidget::descriptionEdit() const
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
    const unsigned checkedCount = checkedFilesCount();
    const bool newCommitState = canSubmit();
    // Emit signal to update action
    if (m_d->m_commitEnabled != newCommitState) {
        m_d->m_commitEnabled = newCommitState;
        emit submitActionEnabledChanged(m_d->m_commitEnabled);
    }
    if (m_d->m_ui.fileView && m_d->m_ui.fileView->model()) {
        // Update button text.
        const int fileCount = m_d->m_ui.fileView->model()->rowCount();
        const QString msg = checkedCount ?
                            tr("Commit %1/%n Files", 0, fileCount).arg(checkedCount) :
                            tr("Commit");
        emit submitActionTextChanged(msg);
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

unsigned SubmitEditorWidget::checkedFilesCount() const
{
    unsigned checkedCount = 0;
    if (const QAbstractItemModel *model = m_d->m_ui.fileView->model()) {
        const int count = model->rowCount();
        for (int i = 0; i < count; i++)
            if (listModelChecked(model, i, checkableColumn))
                checkedCount++;
    }
    return checkedCount;
}

QString SubmitEditorWidget::cleanupDescription(const QString &input) const
{
    return input;
}

void SubmitEditorWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
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

bool SubmitEditorWidget::canSubmit() const
{
    const unsigned checkedCount = checkedFilesCount();
    return m_d->m_emptyFileListEnabled || checkedCount > 0;
}

void SubmitEditorWidget::addSubmitFieldWidget(SubmitFieldWidget *f)
{
    if (!m_d->m_fieldLayout) {
        // VBox with horizontal, expanding spacer
        m_d->m_fieldLayout = new QVBoxLayout;
        QHBoxLayout *outerLayout = new QHBoxLayout;
        outerLayout->addLayout(m_d->m_fieldLayout);
        outerLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
        QBoxLayout *descrLayout = qobject_cast<QBoxLayout*>(m_d->m_ui.descriptionBox->layout());
        Q_ASSERT(descrLayout);
        descrLayout->addLayout(outerLayout);
    }
    m_d->m_fieldLayout->addWidget(f);
    m_d->m_fieldWidgets.push_back(f);
}

QList<SubmitFieldWidget *> SubmitEditorWidget::submitFieldWidgets() const
{
    return m_d->m_fieldWidgets;
}

void SubmitEditorWidget::addDescriptionEditContextMenuAction(QAction *a)
{
    m_d->descriptionEditContextMenuActions.push_back(SubmitEditorWidgetPrivate::AdditionalContextMenuAction(-1, a));
}

void SubmitEditorWidget::insertDescriptionEditContextMenuAction(int pos, QAction *a)
{
    m_d->descriptionEditContextMenuActions.push_back(SubmitEditorWidgetPrivate::AdditionalContextMenuAction(pos, a));
}

void SubmitEditorWidget::editorCustomContextMenuRequested(const QPoint &pos)
{
    QScopedPointer<QMenu> menu(m_d->m_ui.description->createStandardContextMenu());
    // Extend
    foreach (const SubmitEditorWidgetPrivate::AdditionalContextMenuAction &a, m_d->descriptionEditContextMenuActions) {
        if (a.second) {
            if (a.first >= 0) {
                menu->insertAction(menu->actions().at(a.first), a.second);
            } else {
                menu->addAction(a.second);
            }
        }
    }
    menu->exec(m_d->m_ui.description->mapToGlobal(pos));
}

void SubmitEditorWidget::checkAll()
{
    setListModelChecked(m_d->m_ui.fileView->model(), true, checkableColumn);
}

void SubmitEditorWidget::uncheckAll()
{
    setListModelChecked(m_d->m_ui.fileView->model(), false, checkableColumn);
}

void SubmitEditorWidget::fileListCustomContextMenuRequested(const QPoint & pos)
{
    // Execute menu offering to check/uncheck all
    QMenu menu;
    //: Check all for submit
    QAction *checkAllAction = menu.addAction(tr("Check All"));
    //: Uncheck all for submit
    QAction *uncheckAllAction = menu.addAction(tr("Uncheck All"));
    QAction *action = menu.exec(m_d->m_ui.fileView->mapToGlobal(pos));
    if (action == checkAllAction) {
        checkAll();
        return;
    }
    if (action == uncheckAllAction) {
        uncheckAll();
        return;
    }
}

bool SubmitEditorWidget::isEmptyFileListEnabled() const
{
    return m_d->m_emptyFileListEnabled;
}

void SubmitEditorWidget::setEmptyFileListEnabled(bool e)
{
    if (e != m_d->m_emptyFileListEnabled) {
        m_d->m_emptyFileListEnabled = e;
        updateSubmitAction();
    }
}

} // namespace Utils

#include "submiteditorwidget.moc"
