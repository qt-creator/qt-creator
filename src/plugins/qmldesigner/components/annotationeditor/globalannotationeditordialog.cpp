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
#include "annotation.h"
#include "annotationcommenttab.h"
#include "ui_globalannotationeditordialog.h"

#include <timelineicons.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMessageBox>
#include <QObject>
#include <QToolBar>

namespace QmlDesigner {

GlobalAnnotationEditorDialog::GlobalAnnotationEditorDialog(QWidget *parent,
                                                           GlobalAnnotationStatus status)
    : BasicAnnotationEditorDialog(parent)
    , ui(new Ui::GlobalAnnotationEditorDialog)
    , m_globalStatus(status)
    , m_statusIsActive(false)
{
    ui->setupUi(this);
    ui->tabWidget->setDefaultAnnotations(defaultAnnotations());
    ui->tableView->setDefaultAnnotations(defaultAnnotations());

    connect(ui->tableView,
            &AnnotationTableView::richTextEditorRequested,
            this,
            [&](int index, QString const &) {
                switchToTabView();
                ui->tabWidget->setCurrentIndex(index);
            });

    connect(ui->statusAddButton, &QPushButton::clicked, this, [&](bool) {
        setStatusVisibility(true);
    });

    connect(ui->rbTableView,
            &QRadioButton::clicked,
            this,
            &GlobalAnnotationEditorDialog::switchToTableView);
    connect(ui->rbTabView,
            &QRadioButton::clicked,
            this,
            &GlobalAnnotationEditorDialog::switchToTabView);

    setStatus(m_globalStatus);
    setWindowTitle(globalEditorTitle);
    switchToTabView();
}

GlobalAnnotationEditorDialog::~GlobalAnnotationEditorDialog()
{
    delete ui;
}

GlobalAnnotationEditorDialog::ViewMode GlobalAnnotationEditorDialog::viewMode() const
{
    return ui->rbTableView->isChecked() ? TableView : TabsView;
}

void GlobalAnnotationEditorDialog::setStatus(GlobalAnnotationStatus status)
{
    m_globalStatus = status;
    bool hasStatus = status.status() != GlobalAnnotationStatus::NoStatus;

    if (hasStatus)
        ui->statusComboBox->setCurrentIndex(int(status.status()));

    setStatusVisibility(hasStatus);
}

GlobalAnnotationStatus GlobalAnnotationEditorDialog::globalStatus() const
{
    return m_globalStatus;
}

void GlobalAnnotationEditorDialog::showStatusContainer(bool show)
{
    ui->statusContainer->setVisible(show);
}

void GlobalAnnotationEditorDialog::switchToTabView()
{
    m_annotation.setComments(ui->tableView->fetchComments());
    ui->rbTabView->setChecked(true);
    ui->tableView->hide();
    ui->tabWidget->show();
    fillFields();
}

void GlobalAnnotationEditorDialog::switchToTableView()
{
    m_annotation.setComments(ui->tabWidget->fetchComments());
    ui->rbTableView->setChecked(true);
    ui->tabWidget->hide();
    ui->tableView->show();
    fillFields();
}

void GlobalAnnotationEditorDialog::acceptedClicked()
{
    updateAnnotation();
    emit GlobalAnnotationEditorDialog::acceptedDialog();
}

void GlobalAnnotationEditorDialog::fillFields()
{
    ui->tabWidget->setupComments(m_annotation.comments());
    ui->tableView->setupComments(m_annotation.comments());
}

void GlobalAnnotationEditorDialog::updateAnnotation()
{
    Annotation annotation;
    switch (viewMode()) {
    case TabsView:
        annotation.setComments(ui->tabWidget->fetchComments());
        break;
    case TableView:
        annotation.setComments(ui->tableView->fetchComments());
        break;
    }

    m_annotation = annotation;

    if (m_statusIsActive)
        m_globalStatus.setStatus(ui->statusComboBox->currentIndex());
}

void GlobalAnnotationEditorDialog::addComment(const Comment &comment)
{
    m_annotation.addComment(comment);
    ui->tabWidget->addCommentTab(comment);
}

void GlobalAnnotationEditorDialog::removeComment(int index)
{
    if ((m_annotation.commentsSize() > index) && (index >= 0)) {
        m_annotation.removeComment(index);
        ui->tabWidget->removeTab(index);
    }
}

void GlobalAnnotationEditorDialog::setStatusVisibility(bool hasStatus)
{
    ui->statusAddButton->setVisible(!hasStatus);
    ui->statusComboBox->setVisible(hasStatus);

    m_statusIsActive = hasStatus;
}

} //namespace QmlDesigner
