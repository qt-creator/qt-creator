/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "submiteditorwidget.h"
#include "submitfieldwidget.h"
#include "submitfilemodel.h"
#include "ui_submiteditorwidget.h"

#include <QDebug>
#include <QPointer>
#include <QTimer>
#include <QScopedPointer>

#include <QPushButton>
#include <QMenu>
#include <QHBoxLayout>
#include <QToolButton>
#include <QSpacerItem>
#include <QShortcut>

enum { debug = 0 };
enum { defaultLineWidth = 72 };

/*!
    \class VcsBase::SubmitEditorWidget

    \brief Presents a VCS commit message in a text editor and a
     checkable list of modified files in a list window.

    The user can delete files from the list by unchecking them or diff the selection
    by doubleclicking. A list model which contains state and file columns should be
    set using setFileModel().

    Additionally, standard creator actions  can be registered:
    Undo/redo will be set up to work with the description editor.
    Submit will be set up to be enabled according to checkstate.
    Diff will be set up to trigger diffSelected().

    Note that the actions are connected by signals; in the rare event that there
    are several instances of the SubmitEditorWidget belonging to the same
    context active, the actions must be registered/unregistered in the editor
    change event.
    Care should be taken to ensure the widget is deleted properly when the
    editor closes.
*/

namespace VcsBase {

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
    int m_activatedRow;
    bool m_emptyFileListEnabled;

    QList<AdditionalContextMenuAction> descriptionEditContextMenuActions;
    QVBoxLayout *m_fieldLayout;
    QList<SubmitFieldWidget *> m_fieldWidgets;
    QShortcut *m_submitShortcut;
    int m_lineWidth;

    bool m_commitEnabled;
    bool m_ignoreChange;
    bool m_descriptionMandatory;
};

SubmitEditorWidgetPrivate::SubmitEditorWidgetPrivate() :
    m_filesSelected(false),
    m_activatedRow(-1),
    m_emptyFileListEnabled(false),
    m_fieldLayout(0),
    m_submitShortcut(0),
    m_lineWidth(defaultLineWidth),
    m_commitEnabled(false),
    m_ignoreChange(false),
    m_descriptionMandatory(true)
{
}

SubmitEditorWidget::SubmitEditorWidget(QWidget *parent) :
    QWidget(parent),
    d(new SubmitEditorWidgetPrivate)
{
    d->m_ui.setupUi(this);
    d->m_ui.description->setContextMenuPolicy(Qt::CustomContextMenu);
    d->m_ui.description->setLineWrapMode(QTextEdit::NoWrap);
    d->m_ui.description->setWordWrapMode(QTextOption::WordWrap);
    connect(d->m_ui.description, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(editorCustomContextMenuRequested(QPoint)));
    connect(d->m_ui.description, SIGNAL(textChanged()),
            this, SLOT(descriptionTextChanged()));

    // File List
    d->m_ui.fileView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->m_ui.fileView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(fileListCustomContextMenuRequested(QPoint)));
    d->m_ui.fileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    d->m_ui.fileView->setRootIsDecorated(false);
    connect(d->m_ui.fileView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(diffActivated(QModelIndex)));

    connect(d->m_ui.checkAllCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(checkAllToggled()));

    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(d->m_ui.description);
}

SubmitEditorWidget::~SubmitEditorWidget()
{
    delete d;
}

void SubmitEditorWidget::registerActions(QAction *editorUndoAction, QAction *editorRedoAction,
                         QAction *submitAction, QAction *diffAction)
{
    if (editorUndoAction) {
        editorUndoAction->setEnabled(d->m_ui.description->document()->isUndoAvailable());
        connect(d->m_ui.description, SIGNAL(undoAvailable(bool)), editorUndoAction, SLOT(setEnabled(bool)));
        connect(editorUndoAction, SIGNAL(triggered()), d->m_ui.description, SLOT(undo()));
    }
    if (editorRedoAction) {
        editorRedoAction->setEnabled(d->m_ui.description->document()->isRedoAvailable());
        connect(d->m_ui.description, SIGNAL(redoAvailable(bool)), editorRedoAction, SLOT(setEnabled(bool)));
        connect(editorRedoAction, SIGNAL(triggered()), d->m_ui.description, SLOT(redo()));
    }

    if (submitAction) {
        if (debug) {
            const SubmitFileModel *model = fileModel();
            int count = model ? model->rowCount() : 0;
            qDebug() << Q_FUNC_INFO << submitAction << count << "items";
        }
        d->m_commitEnabled = !canSubmit();
        connect(this, SIGNAL(submitActionEnabledChanged(bool)), submitAction, SLOT(setEnabled(bool)));
        // Wire setText via QActionSetTextSlotHelper.
        QActionSetTextSlotHelper *actionSlotHelper = submitAction->findChild<QActionSetTextSlotHelper *>();
        if (!actionSlotHelper)
            actionSlotHelper = new QActionSetTextSlotHelper(submitAction);
        connect(this, SIGNAL(submitActionTextChanged(QString)), actionSlotHelper, SLOT(setText(QString)));
        d->m_ui.buttonLayout->addWidget(new QActionPushButton(submitAction));
        if (!d->m_submitShortcut)
            d->m_submitShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this);
        connect(d->m_submitShortcut, SIGNAL(activated()), submitAction, SLOT(trigger()));
    }
    if (diffAction) {
        if (debug)
            qDebug() << diffAction << d->m_filesSelected;
        diffAction->setEnabled(d->m_filesSelected);
        connect(this, SIGNAL(fileSelectionChanged(bool)), diffAction, SLOT(setEnabled(bool)));
        connect(diffAction, SIGNAL(triggered()), this, SLOT(triggerDiffSelected()));
        d->m_ui.buttonLayout->addWidget(new QActionPushButton(diffAction));
    }
}

void SubmitEditorWidget::unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                                           QAction *submitAction, QAction *diffAction)
{
    if (editorUndoAction) {
        disconnect(d->m_ui.description, SIGNAL(undoAvailableChanged(bool)), editorUndoAction, SLOT(setEnabled(bool)));
        disconnect(editorUndoAction, SIGNAL(triggered()), d->m_ui.description, SLOT(undo()));
    }
    if (editorRedoAction) {
        disconnect(d->m_ui.description, SIGNAL(redoAvailableChanged(bool)), editorRedoAction, SLOT(setEnabled(bool)));
        disconnect(editorRedoAction, SIGNAL(triggered()), d->m_ui.description, SLOT(redo()));
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
        cursor.movePosition(QTextCursor::NextCharacter);
    }
    return rc;
}

QString SubmitEditorWidget::descriptionText() const
{
    QString rc = trimMessageText(lineWrap() ? wrappedText(d->m_ui.description) :
                                              d->m_ui.description->toPlainText());
    // append field entries
    foreach (const SubmitFieldWidget *fw, d->m_fieldWidgets)
        rc += fw->fieldValues();
    return cleanupDescription(rc);
}

void SubmitEditorWidget::setDescriptionText(const QString &text)
{
    d->m_ui.description->setPlainText(text);
}

bool SubmitEditorWidget::lineWrap() const
{
    return d->m_ui.description->lineWrapMode() != QTextEdit::NoWrap;
}

void SubmitEditorWidget::setLineWrap(bool v)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << v;
    if (v) {
        d->m_ui.description->setLineWrapColumnOrWidth(d->m_lineWidth);
        d->m_ui.description->setLineWrapMode(QTextEdit::FixedColumnWidth);
    } else {
        d->m_ui.description->setLineWrapMode(QTextEdit::NoWrap);
    }
}

int SubmitEditorWidget::lineWrapWidth() const
{
    return d->m_lineWidth;
}

void SubmitEditorWidget::setLineWrapWidth(int v)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << v << lineWrap();
    if (d->m_lineWidth == v)
        return;
    d->m_lineWidth = v;
    if (lineWrap())
        d->m_ui.description->setLineWrapColumnOrWidth(v);
}

bool SubmitEditorWidget::isDescriptionMandatory() const
{
    return d->m_descriptionMandatory;
}

void SubmitEditorWidget::setDescriptionMandatory(bool v)
{
    d->m_descriptionMandatory = v;
}

QAbstractItemView::SelectionMode SubmitEditorWidget::fileListSelectionMode() const
{
    return d->m_ui.fileView->selectionMode();
}

void SubmitEditorWidget::setFileListSelectionMode(QAbstractItemView::SelectionMode sm)
{
    d->m_ui.fileView->setSelectionMode(sm);
}

void SubmitEditorWidget::setFileModel(SubmitFileModel *model)
{
    d->m_ui.fileView->clearSelection(); // trigger the change signals

    d->m_ui.fileView->setModel(model);

    if (model->rowCount()) {
        const int columnCount = model->columnCount();
        for (int c = 0;  c < columnCount; c++)
            d->m_ui.fileView->resizeColumnToContents(c);
    }

    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateSubmitAction()));
    connect(model, SIGNAL(modelReset()),
            this, SLOT(updateSubmitAction()));
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateCheckAllComboBox()));
    connect(model, SIGNAL(modelReset()),
            this, SLOT(updateCheckAllComboBox()));
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(updateSubmitAction()));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(updateSubmitAction()));
    connect(d->m_ui.fileView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateDiffAction()));
    updateActions();
}

SubmitFileModel *SubmitEditorWidget::fileModel() const
{
    return static_cast<SubmitFileModel *>(d->m_ui.fileView->model());
}

QStringList SubmitEditorWidget::checkedFiles() const
{
    QStringList rc;
    const SubmitFileModel *model = fileModel();
    if (!model)
        return rc;
    const int count = model->rowCount();
    for (int i = 0; i < count; i++)
        if (model->checked(i))
            rc.push_back(model->file(i));
    return rc;
}

Utils::CompletingTextEdit *SubmitEditorWidget::descriptionEdit() const
{
    return d->m_ui.description;
}

void SubmitEditorWidget::triggerDiffSelected()
{
    const QList<int> sel = selectedRows(d->m_ui.fileView);
    if (!sel.empty())
        emit diffSelected(sel);
}

void SubmitEditorWidget::diffActivatedDelayed()
{
    emit diffSelected(QList<int>() << d->m_activatedRow);
}

void SubmitEditorWidget::diffActivated(const QModelIndex &index)
{
    // We need to delay the signal, otherwise, the diff editor will not
    // be in the foreground.
    d->m_activatedRow = index.row();
    QTimer::singleShot(0, this, SLOT(diffActivatedDelayed()));
}

void SubmitEditorWidget::updateActions()
{
    updateSubmitAction();
    updateDiffAction();
    updateCheckAllComboBox();
}

// Enable submit depending on having checked files
void SubmitEditorWidget::updateSubmitAction()
{
    const unsigned checkedCount = checkedFilesCount();
    const bool newCommitState = canSubmit();
    // Emit signal to update action
    if (d->m_commitEnabled != newCommitState) {
        d->m_commitEnabled = newCommitState;
        emit submitActionEnabledChanged(d->m_commitEnabled);
    }
    if (d->m_ui.fileView && d->m_ui.fileView->model()) {
        // Update button text.
        const int fileCount = d->m_ui.fileView->model()->rowCount();
        const QString msg = checkedCount ?
                            tr("%1 %2/%n File(s)", 0, fileCount)
                            .arg(commitName()).arg(checkedCount) :
                            commitName();
        emit submitActionTextChanged(msg);
    }
}

// Enable diff depending on selected files
void SubmitEditorWidget::updateDiffAction()
{
    const bool filesSelected = hasSelection();
    if (d->m_filesSelected != filesSelected) {
        d->m_filesSelected = filesSelected;
        emit fileSelectionChanged(d->m_filesSelected);
    }
}

void SubmitEditorWidget::updateCheckAllComboBox()
{
    d->m_ignoreChange = true;
    int checkedCount = checkedFilesCount();
    if (checkedCount == 0)
        d->m_ui.checkAllCheckBox->setCheckState(Qt::Unchecked);
    else if (checkedCount == d->m_ui.fileView->model()->rowCount())
        d->m_ui.checkAllCheckBox->setCheckState(Qt::Checked);
    else
        d->m_ui.checkAllCheckBox->setCheckState(Qt::PartiallyChecked);
    d->m_ignoreChange = false;
}

bool SubmitEditorWidget::hasSelection() const
{
    // Not present until model is set
    if (const QItemSelectionModel *sm = d->m_ui.fileView->selectionModel())
        return sm->hasSelection();
    return false;
}

int SubmitEditorWidget::checkedFilesCount() const
{
    int checkedCount = 0;
    if (const SubmitFileModel *model = fileModel()) {
        const int count = model->rowCount();
        for (int i = 0; i < count; ++i)
            if (model->checked(i))
                ++checkedCount;
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
        d->m_ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

void SubmitEditorWidget::insertTopWidget(QWidget *w)
{
    d->m_ui.vboxLayout->insertWidget(0, w);
}

void SubmitEditorWidget::descriptionTextChanged()
{
#if QT_VERSION < 0x050000 // Fix Qt-Bug, see QTCREATORBUG-5633 && QTCREATORBUG-6082
    static QString lastText;
    const QString text = d->m_ui.description->toPlainText();
    if (lastText != text)
        lastText = text;
    else
        return;
#endif
    updateSubmitAction();
}

bool SubmitEditorWidget::canSubmit() const
{
    if (isDescriptionMandatory() && cleanupDescription(descriptionText()).trimmed().isEmpty())
        return false;
    const unsigned checkedCount = checkedFilesCount();
    return d->m_emptyFileListEnabled || checkedCount > 0;
}

QString SubmitEditorWidget::commitName() const
{
    return tr("&Commit");
}

void SubmitEditorWidget::addSubmitFieldWidget(SubmitFieldWidget *f)
{
    if (!d->m_fieldLayout) {
        // VBox with horizontal, expanding spacer
        d->m_fieldLayout = new QVBoxLayout;
        QHBoxLayout *outerLayout = new QHBoxLayout;
        outerLayout->addLayout(d->m_fieldLayout);
        outerLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
        QBoxLayout *descrLayout = qobject_cast<QBoxLayout*>(d->m_ui.descriptionBox->layout());
        Q_ASSERT(descrLayout);
        descrLayout->addLayout(outerLayout);
    }
    d->m_fieldLayout->addWidget(f);
    d->m_fieldWidgets.push_back(f);
}

QList<SubmitFieldWidget *> SubmitEditorWidget::submitFieldWidgets() const
{
    return d->m_fieldWidgets;
}

void SubmitEditorWidget::addDescriptionEditContextMenuAction(QAction *a)
{
    d->descriptionEditContextMenuActions.push_back(SubmitEditorWidgetPrivate::AdditionalContextMenuAction(-1, a));
}

void SubmitEditorWidget::insertDescriptionEditContextMenuAction(int pos, QAction *a)
{
    d->descriptionEditContextMenuActions.push_back(SubmitEditorWidgetPrivate::AdditionalContextMenuAction(pos, a));
}

void SubmitEditorWidget::editorCustomContextMenuRequested(const QPoint &pos)
{
    QScopedPointer<QMenu> menu(d->m_ui.description->createStandardContextMenu());
    // Extend
    foreach (const SubmitEditorWidgetPrivate::AdditionalContextMenuAction &a, d->descriptionEditContextMenuActions) {
        if (a.second) {
            if (a.first >= 0)
                menu->insertAction(menu->actions().at(a.first), a.second);
            else
                menu->addAction(a.second);
        }
    }
    menu->exec(d->m_ui.description->mapToGlobal(pos));
}

void SubmitEditorWidget::checkAllToggled()
{
    if (d->m_ignoreChange)
        return;
    Qt::CheckState checkState = d->m_ui.checkAllCheckBox->checkState();
    fileModel()->setAllChecked(checkState == Qt::Checked || checkState == Qt::PartiallyChecked);
    // Reset that again, so that the user can't do it
    d->m_ui.checkAllCheckBox->setTristate(false);
}

void SubmitEditorWidget::checkAll()
{
    fileModel()->setAllChecked(true);
}

void SubmitEditorWidget::uncheckAll()
{
    fileModel()->setAllChecked(false);
}

void SubmitEditorWidget::fileListCustomContextMenuRequested(const QPoint & pos)
{
    // Execute menu offering to check/uncheck all
    QMenu menu;
    //: Check all for submit
    QAction *checkAllAction = menu.addAction(tr("Check All"));
    //: Uncheck all for submit
    QAction *uncheckAllAction = menu.addAction(tr("Uncheck All"));
    QAction *action = menu.exec(d->m_ui.fileView->mapToGlobal(pos));
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
    return d->m_emptyFileListEnabled;
}

void SubmitEditorWidget::setEmptyFileListEnabled(bool e)
{
    if (e != d->m_emptyFileListEnabled) {
        d->m_emptyFileListEnabled = e;
        updateSubmitAction();
    }
}

} // namespace VcsBase

#include "submiteditorwidget.moc"
