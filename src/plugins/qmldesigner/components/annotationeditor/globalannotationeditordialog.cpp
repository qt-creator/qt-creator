/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "globalannotationeditordialog.h"
#include "ui_globalannotationeditordialog.h"
#include "annotation.h"
#include "annotationcommenttab.h"

#include "ui_annotationcommenttab.h"

#include <timelineicons.h>
#include <utils/qtcassert.h>

#include <QObject>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>

namespace QmlDesigner {

GlobalAnnotationEditorDialog::GlobalAnnotationEditorDialog(QWidget *parent, const Annotation &annotation, GlobalAnnotationStatus status)
    : QDialog(parent)
    , ui(new Ui::GlobalAnnotationEditorDialog)
    , m_annotation(annotation)
    , m_globalStatus(status)
    , m_statusIsActive(false)
{
    ui->setupUi(this);

    setWindowFlag(Qt::Tool, true);
    setModal(true);

    connect(this, &QDialog::accepted, this, &GlobalAnnotationEditorDialog::acceptedClicked);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &GlobalAnnotationEditorDialog::tabChanged);

    auto *commentCornerWidget = new QToolBar;

    auto *commentAddAction = new QAction(TimelineIcons::ADD_TIMELINE.icon(), tr("Add Comment")); //timeline icons?
    auto *commentRemoveAction = new QAction(TimelineIcons::REMOVE_TIMELINE.icon(),
                                             tr("Remove Comment")); //timeline icons?

    connect(commentAddAction, &QAction::triggered, this, [this]() {
        addComment(Comment());
    });

    connect(commentRemoveAction, &QAction::triggered, this, [this]() {

        if (ui->tabWidget->count() == 0) { //it is not even supposed to happen but lets be sure
            QTC_ASSERT(false, return);
            return;
        }

        int currentIndex = ui->tabWidget->currentIndex();
        QString currentTitle = ui->tabWidget->tabText(currentIndex);

        QMessageBox *deleteDialog = new QMessageBox(this);
        deleteDialog->setWindowTitle(currentTitle);
        deleteDialog->setText(tr("Delete this comment?"));
        deleteDialog->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        deleteDialog->setDefaultButton(QMessageBox::Yes);

        int result = deleteDialog->exec();

        if (result == QMessageBox::Yes) {
            removeComment(currentIndex);
        }

        if (ui->tabWidget->count() == 0) //lets be sure that tabWidget is never empty
            addComment(Comment());
    });

    commentCornerWidget->addAction(commentAddAction);
    commentCornerWidget->addAction(commentRemoveAction);

    ui->tabWidget->setCornerWidget(commentCornerWidget, Qt::TopRightCorner);

    connect(ui->statusAddButton, &QPushButton::clicked, [&](bool){
        setStatusVisibility(true);
    });

    setStatus(m_globalStatus);

    fillFields();
    setWindowTitle(globalEditorTitle);
}

GlobalAnnotationEditorDialog::~GlobalAnnotationEditorDialog()
{
    delete ui;
}

void GlobalAnnotationEditorDialog::setAnnotation(const Annotation &annotation)
{
    m_annotation = annotation;
    fillFields();
}

Annotation GlobalAnnotationEditorDialog::annotation() const
{
    return m_annotation;
}

void GlobalAnnotationEditorDialog::setStatus(GlobalAnnotationStatus status)
{
    m_globalStatus = status;

    bool hasStatus = (status.status() != GlobalAnnotationStatus::NoStatus);

    if (hasStatus) {
        ui->statusComboBox->setCurrentIndex(int(status.status()));
    }

    setStatusVisibility(hasStatus);
}

GlobalAnnotationStatus GlobalAnnotationEditorDialog::globalStatus() const
{
    return m_globalStatus;
}

void GlobalAnnotationEditorDialog::acceptedClicked()
{
    Annotation annotation;

    annotation.removeComments();

    for (int i = 0; i < ui->tabWidget->count(); i++) {
        AnnotationCommentTab* tab = reinterpret_cast<AnnotationCommentTab*>(ui->tabWidget->widget(i));
        if (!tab)
            continue;

        Comment comment = tab->currentComment();

        if (!comment.isEmpty())
            annotation.addComment(comment);
    }

    m_annotation = annotation;

    if (m_statusIsActive) {
        m_globalStatus.setStatus(ui->statusComboBox->currentIndex());
    }

    emit GlobalAnnotationEditorDialog::accepted();
}

void GlobalAnnotationEditorDialog::commentTitleChanged(const QString &text, QWidget *tab)
{
    int tabIndex = ui->tabWidget->indexOf(tab);
    if (tabIndex >= 0)
        ui->tabWidget->setTabText(tabIndex, text);

    if (text.isEmpty())
        ui->tabWidget->setTabText(tabIndex,
                                  (defaultTabName + " " + QString::number(tabIndex+1)));
}

void GlobalAnnotationEditorDialog::fillFields()
{
    setupComments();
}

void GlobalAnnotationEditorDialog::setupComments()
{
    ui->tabWidget->setUpdatesEnabled(false);

    deleteAllTabs();

    const QVector<Comment> comments = m_annotation.comments();

    if (comments.isEmpty())
        addComment(Comment());

    for (const Comment &comment : comments) {
        addCommentTab(comment);
    }

    ui->tabWidget->setUpdatesEnabled(true);
}

void GlobalAnnotationEditorDialog::addComment(const Comment &comment)
{
    m_annotation.addComment(comment);
    addCommentTab(comment);
}

void GlobalAnnotationEditorDialog::removeComment(int index)
{
    if ((m_annotation.commentsSize() > index) && (index >= 0)) {
        m_annotation.removeComment(index);
        removeCommentTab(index);
    }
}

void GlobalAnnotationEditorDialog::addCommentTab(const Comment &comment)
{
    auto commentTab = new AnnotationCommentTab();
    commentTab->setComment(comment);

    QString tabTitle(comment.title());
    int tabIndex = ui->tabWidget->addTab(commentTab, tabTitle);
    ui->tabWidget->setCurrentIndex(tabIndex);

    if (tabTitle.isEmpty()) {
        const QString appendix = ((tabIndex > 0) ? QString::number(tabIndex+1) : "");

        tabTitle = QString("%1 %2").arg(defaultTabName).arg(appendix);

        ui->tabWidget->setTabText(tabIndex, tabTitle);
    }

    connect(commentTab, &AnnotationCommentTab::titleChanged,
            this, &GlobalAnnotationEditorDialog::commentTitleChanged);
}

void GlobalAnnotationEditorDialog::removeCommentTab(int index)
{
    if ((ui->tabWidget->count() > index) && (index >= 0)) {
        ui->tabWidget->removeTab(index);
    }
}

void GlobalAnnotationEditorDialog::deleteAllTabs()
{
    while (ui->tabWidget->count() > 0) {
        QWidget *w = ui->tabWidget->widget(0);
        ui->tabWidget->removeTab(0);
        delete w;
    }
}

void GlobalAnnotationEditorDialog::setStatusVisibility(bool hasStatus)
{
    ui->statusAddButton->setVisible(!hasStatus);
    ui->statusComboBox->setVisible(hasStatus);

    m_statusIsActive = hasStatus;
}

void GlobalAnnotationEditorDialog::tabChanged(int index)
{
    (void) index;
}

} //namespace QmlDesigner
