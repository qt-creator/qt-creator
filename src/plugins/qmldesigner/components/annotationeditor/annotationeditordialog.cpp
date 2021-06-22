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
#include "annotation.h"
#include "annotationcommenttab.h"
#include "defaultannotations.h"

#include "ui_annotationeditordialog.h"

#include <timelineicons.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMessageBox>
#include <QObject>
#include <QToolBar>

namespace QmlDesigner {
AnnotationEditorDialog::AnnotationEditorDialog(QWidget *parent,
                                               const QString &targetId,
                                               const QString &customId)
    : QDialog(parent)
    , m_defaults(std::make_unique<DefaultAnnotationsModel>())
    , m_customId(customId)
    , ui(std::make_unique<Ui::AnnotationEditorDialog>())
    , m_statusIsActive(false)
{
    ui->setupUi(this);
    setGlobal(m_isGlobal);

    setWindowFlag(Qt::Tool, true);
    setModal(true);
    loadDefaultAnnotations(DefaultAnnotationsModel::defaultJsonFilePath());
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
            &AnnotationEditorDialog::switchToTableView);
    connect(ui->rbTabView,
            &QRadioButton::clicked,
            this, &AnnotationEditorDialog::switchToTabView);

    setStatus(m_globalStatus);
    switchToTabView();

    connect(this, &QDialog::accepted, this, &AnnotationEditorDialog::acceptedClicked);

    ui->targetIdEdit->setText(targetId);

}

AnnotationEditorDialog::~AnnotationEditorDialog() {
}

bool AnnotationEditorDialog::isGlobal() const {
    return m_isGlobal;
}

void AnnotationEditorDialog::setGlobal(bool global) {
    ui->annotationContainer->setVisible(!global);
    ui->statusAddButton->setVisible(global);
    ui->statusComboBox->setVisible(global);

    setWindowTitle(global ? tr("Global Annotation Editor") : tr("Annotation Editor"));

    if (m_isGlobal != global) {
        m_isGlobal = global;
        emit globalChanged();
    }
}

AnnotationEditorDialog::ViewMode AnnotationEditorDialog::viewMode() const
{
    return ui->rbTableView->isChecked() ? TableView : TabsView;
}

void AnnotationEditorDialog::setStatus(GlobalAnnotationStatus status)
{
    m_globalStatus = status;
    bool hasStatus = status.status() != GlobalAnnotationStatus::NoStatus;

    if (hasStatus)
        ui->statusComboBox->setCurrentIndex(int(status.status()));

    setStatusVisibility(hasStatus);
}

GlobalAnnotationStatus AnnotationEditorDialog::globalStatus() const
{
    return m_globalStatus;
}

void AnnotationEditorDialog::showStatusContainer(bool show)
{
    ui->statusContainer->setVisible(show);
}

void AnnotationEditorDialog::switchToTabView()
{
    m_annotation.setComments(ui->tableView->fetchComments());
    ui->rbTabView->setChecked(true);
    ui->tableView->hide();
    ui->tabWidget->show();
    fillFields();
}

void AnnotationEditorDialog::switchToTableView()
{
    m_annotation.setComments(ui->tabWidget->fetchComments());
    ui->rbTableView->setChecked(true);
    ui->tabWidget->hide();
    ui->tableView->show();
    fillFields();
}

void AnnotationEditorDialog::acceptedClicked()
{
    updateAnnotation();
    emit AnnotationEditorDialog::acceptedDialog();
}

void AnnotationEditorDialog::fillFields()
{
    ui->customIdEdit->setText(m_customId);
    ui->tabWidget->setupComments(m_annotation.comments());
    ui->tableView->setupComments(m_annotation.comments());
}

void AnnotationEditorDialog::updateAnnotation()
{
    m_customId = ui->customIdEdit->text();
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

    if (m_statusIsActive && m_isGlobal)
        m_globalStatus.setStatus(ui->statusComboBox->currentIndex());
}

void AnnotationEditorDialog::addComment(const Comment &comment)
{
    m_annotation.addComment(comment);
    ui->tabWidget->addCommentTab(comment);
}

void AnnotationEditorDialog::removeComment(int index)
{
    if ((m_annotation.commentsSize() > index) && (index >= 0)) {
        m_annotation.removeComment(index);
        ui->tabWidget->removeTab(index);
    }
}

void AnnotationEditorDialog::setStatusVisibility(bool hasStatus)
{
    ui->statusAddButton->setVisible(!hasStatus && m_isGlobal);
    ui->statusComboBox->setVisible(hasStatus && m_isGlobal);

    m_statusIsActive = hasStatus;
}


Annotation const &AnnotationEditorDialog::annotation() const
{
    return m_annotation;
}

void AnnotationEditorDialog::setAnnotation(const Annotation &annotation)
{
    m_annotation = annotation;
    fillFields();
}

void AnnotationEditorDialog::loadDefaultAnnotations(QString const &filename)
{
    m_defaults->loadFromFile(filename);
}

DefaultAnnotationsModel *AnnotationEditorDialog::defaultAnnotations() const
{
    return m_defaults.get();
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


} //namespace QmlDesigner
