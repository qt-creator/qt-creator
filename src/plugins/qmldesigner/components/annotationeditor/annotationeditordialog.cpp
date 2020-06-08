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

#include "annotationeditordialog.h"
#include "ui_annotationeditordialog.h"
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

AnnotationEditorDialog::AnnotationEditorDialog(QWidget *parent, const QString &targetId, const QString &customId, const Annotation &annotation)
    : QDialog(parent)
    , ui(new Ui::AnnotationEditorDialog)
    , m_customId(customId)
    , m_annotation(annotation)
{
    ui->setupUi(this);

    setWindowFlag(Qt::Tool, true);
    setModal(true);

    connect(this, &QDialog::accepted, this, &AnnotationEditorDialog::acceptedClicked);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &AnnotationEditorDialog::tabChanged);

    auto *commentCornerWidget = new QToolBar;

    auto *commentAddAction = new QAction(TimelineIcons::ADD_TIMELINE.icon(), tr("Add Comment")); //timeline icons?
    auto *commentRemoveAction = new QAction(TimelineIcons::REMOVE_TIMELINE.icon(),
                                             tr("Remove Comment")); //timeline icons?

    connect(commentAddAction, &QAction::triggered, this, [this]() {
        addComment(Comment());
    });

    connect(commentRemoveAction, &QAction::triggered, this, [this]() {

        if (ui->tabWidget->count() == 0) { //it is not even supposed to happen but lets be sure
            QTC_ASSERT(true, return);
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
    ui->targetIdEdit->setText(targetId);

    fillFields();
    setWindowTitle(annotationEditorTitle);
}

AnnotationEditorDialog::~AnnotationEditorDialog()
{
    delete ui;
}

void AnnotationEditorDialog::setAnnotation(const Annotation &annotation)
{
    m_annotation = annotation;
    fillFields();
}

Annotation AnnotationEditorDialog::annotation() const
{
    return m_annotation;
}

void AnnotationEditorDialog::setCustomId(const QString &customId)
{
    m_customId = customId;
    ui->customIdEdit->setText(m_customId);
}

QString AnnotationEditorDialog::customId() const
{
    return m_customId;
}

void AnnotationEditorDialog::acceptedClicked()
{
    m_customId = ui->customIdEdit->text();

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

    emit AnnotationEditorDialog::accepted();
}

void AnnotationEditorDialog::commentTitleChanged(const QString &text, QWidget *tab)
{
    int tabIndex = ui->tabWidget->indexOf(tab);
    if (tabIndex >= 0)
        ui->tabWidget->setTabText(tabIndex, text);

    if (text.isEmpty())
        ui->tabWidget->setTabText(tabIndex,
                                  (defaultTabName + " " + QString::number(tabIndex+1)));
}

void AnnotationEditorDialog::fillFields()
{
    ui->customIdEdit->setText(m_customId);
    setupComments();
}

void AnnotationEditorDialog::setupComments()
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

void AnnotationEditorDialog::addComment(const Comment &comment)
{
    m_annotation.addComment(comment);
    addCommentTab(comment);
}

void AnnotationEditorDialog::removeComment(int index)
{
    if ((m_annotation.commentsSize() > index) && (index >= 0)) {
        m_annotation.removeComment(index);
        removeCommentTab(index);
    }
}

void AnnotationEditorDialog::addCommentTab(const Comment &comment)
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
            this, &AnnotationEditorDialog::commentTitleChanged);
}

void AnnotationEditorDialog::removeCommentTab(int index)
{
    if ((ui->tabWidget->count() > index) && (index >= 0)) {
        ui->tabWidget->removeTab(index);
    }
}

void AnnotationEditorDialog::deleteAllTabs()
{
    while (ui->tabWidget->count() > 0) {
        QWidget *w = ui->tabWidget->widget(0);
        ui->tabWidget->removeTab(0);
        delete w;
    }
}

void AnnotationEditorDialog::tabChanged(int index)
{
    (void) index;
}

} //namespace QmlDesigner
