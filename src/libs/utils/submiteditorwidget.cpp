/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "submiteditorwidget.h"
#include "ui_submiteditorwidget.h"

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtGui/QPushButton>

enum { debug = 0 };

namespace Core {
namespace Utils {

// QActionPushButton: A push button tied to an action
// (similar to a QToolButton)
class QActionPushButton : public QPushButton {
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

// -----------  SubmitEditorWidgetPrivate
struct SubmitEditorWidgetPrivate
{
    SubmitEditorWidgetPrivate();

    Ui::SubmitEditorWidget m_ui;
    bool m_filesSelected;
    bool m_filesChecked;
};

SubmitEditorWidgetPrivate::SubmitEditorWidgetPrivate() :
    m_filesSelected(false),
    m_filesChecked(false)
{
}

SubmitEditorWidget::SubmitEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_d(new SubmitEditorWidgetPrivate)
{
    m_d->m_ui.setupUi(this);
    // File List
    m_d->m_ui.fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_d->m_ui.fileList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(triggerDiffSelected()));
    connect(m_d->m_ui.fileList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(fileItemChanged(QListWidgetItem*)));
    connect(m_d->m_ui.fileList, SIGNAL(itemSelectionChanged()), this, SLOT(fileSelectionChanged()));

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
        if (debug)
            qDebug() << submitAction << m_d->m_ui.fileList->count() << "items" << m_d->m_filesChecked;
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

QStringList SubmitEditorWidget::fileList() const
{
    QStringList rc;
    const int count = m_d->m_ui.fileList->count();
    for (int i = 0; i < count; i++)
        rc.push_back(m_d->m_ui.fileList->item(i)->text());
    return rc;
}

void SubmitEditorWidget::addFilesUnblocked(const QStringList &list, bool checked, bool userCheckable)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << list << checked << userCheckable;
    foreach (const QString &f, list) {
        QListWidgetItem *item = new QListWidgetItem(f);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        if (!userCheckable)
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        m_d->m_ui.fileList->addItem(item);
    }
}

void SubmitEditorWidget::addFiles(const QStringList &list, bool checked, bool userCheckable)
{
    if (list.empty())
        return;

    const bool blocked = m_d->m_ui.fileList->blockSignals(true);
    addFilesUnblocked(list, checked, userCheckable);
    m_d->m_ui.fileList->blockSignals(blocked);
    // Did we gain any checked files..update action accordingly
    if (!m_d->m_filesChecked && checked) {
        m_d->m_filesChecked = true;
        emit fileCheckStateChanged(m_d->m_filesChecked);
    }
}

void SubmitEditorWidget::setFileList(const QStringList &list)
{
    // Trigger enabling of menu action
    m_d->m_ui.fileList->clearSelection();

    const bool blocked = m_d->m_ui.fileList->blockSignals(true);
    m_d->m_ui.fileList->clear();
    if (!list.empty()) {
        addFilesUnblocked(list, true, true);
        // Checked files added?
        if (!m_d->m_filesChecked) {
            m_d->m_filesChecked = true;
            emit fileCheckStateChanged(m_d->m_filesChecked);
        }
    }
    m_d->m_ui.fileList->blockSignals(blocked);
}

static bool containsCheckState(const QListWidget *lw,  Qt::CheckState cs)
{
    const int count = lw->count();
    for (int i = 0; i < count; i++)
        if (lw->item(i)->checkState() == cs)
            return true;
    return false;
}

QStringList SubmitEditorWidget::selectedFiles() const
{
    QStringList rc;
    const int count = m_d->m_ui.fileList->count();
    for (int i = 0; i < count; i++) {
        const QListWidgetItem *item = m_d->m_ui.fileList->item(i);
        if (item->isSelected())
            rc.push_back(item->text());
    }
    return rc;
}

QStringList SubmitEditorWidget::checkedFiles() const
{
    QStringList rc;
    const int count = m_d->m_ui.fileList->count();
    for (int i = 0; i < count; i++) {
        const QListWidgetItem *item = m_d->m_ui.fileList->item(i);
        if (item->checkState() == Qt::Checked)
            rc.push_back(item->text());
    }
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

void SubmitEditorWidget::fileItemChanged(QListWidgetItem *item)
{
    const Qt::CheckState st = item->checkState();
    if (debug)
        qDebug() << Q_FUNC_INFO << st << item->text() << m_d->m_filesChecked;
    // Enable the actions according to check state
    switch (st) {
    case Qt::Unchecked: // Item was unchecked: Any checked items left?
        if (m_d->m_filesChecked && !containsCheckState(m_d->m_ui.fileList, Qt::Checked)) {
            m_d->m_filesChecked = false;
            emit fileCheckStateChanged(m_d->m_filesChecked);
        }
        break;
    case Qt::Checked:
        // Item was Checked. First one?
        if (!m_d->m_filesChecked) {
            m_d->m_filesChecked = true;
            emit fileCheckStateChanged(m_d->m_filesChecked);
        }
        break;
    case Qt::PartiallyChecked: // Errm?
        break;
    }
}

void SubmitEditorWidget::fileSelectionChanged()
{
    const bool newFilesSelected = !m_d->m_ui.fileList->selectedItems().empty();
    if (debug)
        qDebug() << Q_FUNC_INFO << newFilesSelected;
    if (m_d->m_filesSelected != newFilesSelected) {
        m_d->m_filesSelected = newFilesSelected;
        emit fileSelectionChanged(m_d->m_filesSelected);
        if (debug)
            qDebug() << Q_FUNC_INFO << m_d->m_filesSelected;
    }
}

void SubmitEditorWidget::changeEvent(QEvent *e)
{
    switch(e->type()) {
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
