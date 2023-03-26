// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationeditorwidget.h"
#include "annotation.h"
#include "annotationcommenttab.h"
#include "defaultannotations.h"

#include "ui_annotationeditorwidget.h"

#include <timelineicons.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMessageBox>
#include <QObject>
#include <QToolBar>
#include <QAbstractButton>

namespace QmlDesigner {
AnnotationEditorWidget::AnnotationEditorWidget(QWidget *parent,
                                               const QString &targetId,
                                               const QString &customId)
    : QWidget(parent)
    , m_defaults(std::make_unique<DefaultAnnotationsModel>())
    , ui(std::make_unique<Ui::AnnotationEditorWidget>())
    , m_statusIsActive(false)
    , m_customId(customId)
{
    ui->setupUi(this);
    setGlobal(m_isGlobal);

    loadDefaultAnnotations(DefaultAnnotationsModel::defaultJsonFilePath());
    ui->tabWidget->setDefaultAnnotations(defaultAnnotations());
    ui->tableView->setDefaultAnnotations(defaultAnnotations());

    connect(ui->tableView,
            &AnnotationTableView::richTextEditorRequested,
            this,
            [&](int index, const QString &) {
                switchToTabView();
                ui->tabWidget->setCurrentIndex(index);
            });

    connect(ui->statusAddButton, &QPushButton::clicked, this, [&](bool) {
        setStatusVisibility(true);
    });

    connect(ui->rbTableView,
            &QRadioButton::clicked,
            this,
            &AnnotationEditorWidget::switchToTableView);
    connect(ui->rbTabView,
            &QRadioButton::clicked,
            this, &AnnotationEditorWidget::switchToTabView);

    setStatus(m_globalStatus);
    switchToTabView();

    ui->targetIdEdit->setText(targetId);
}

//have to default it in here instead of the header because of unique_ptr with forward declared type
AnnotationEditorWidget::~AnnotationEditorWidget() = default;

bool AnnotationEditorWidget::isGlobal() const
{
    return m_isGlobal;
}

void AnnotationEditorWidget::setGlobal(bool global)
{
    ui->annotationContainer->setVisible(!global);
    ui->statusAddButton->setVisible(global);
    ui->statusComboBox->setVisible(global);

    if (m_isGlobal != global) {
        m_isGlobal = global;
        emit globalChanged();
    }
}

AnnotationEditorWidget::ViewMode AnnotationEditorWidget::viewMode() const
{
    return ui->rbTableView->isChecked() ? ViewMode::TableView : ViewMode::TabsView;
}

void AnnotationEditorWidget::setStatus(GlobalAnnotationStatus status)
{
    m_globalStatus = status;
    bool hasStatus = status.status() != GlobalAnnotationStatus::NoStatus;

    if (hasStatus)
        ui->statusComboBox->setCurrentIndex(static_cast<int>(status.status()));

    setStatusVisibility(hasStatus);
}

GlobalAnnotationStatus AnnotationEditorWidget::globalStatus() const
{
    return m_globalStatus;
}

void AnnotationEditorWidget::showStatusContainer(bool show)
{
    ui->statusContainer->setVisible(show);
}

void AnnotationEditorWidget::switchToTabView()
{
    m_annotation.setComments(ui->tableView->fetchComments());
    ui->rbTabView->setChecked(true);
    ui->tableView->hide();
    ui->tabWidget->show();
    fillFields();
    if (ui->tabWidget->count() > 0)
        ui->tabWidget->setCurrentIndex(0);
}

void AnnotationEditorWidget::switchToTableView()
{
    m_annotation.setComments(ui->tabWidget->fetchComments());
    ui->rbTableView->setChecked(true);
    ui->tabWidget->hide();
    ui->tableView->show();
    fillFields();
}

void AnnotationEditorWidget::fillFields()
{
    ui->customIdEdit->setText(m_customId);
    ui->tabWidget->setupComments(m_annotation.comments());
    ui->tableView->setupComments(m_annotation.comments());
}

void AnnotationEditorWidget::updateAnnotation()
{
    m_customId = ui->customIdEdit->text();
    Annotation annotation;
    switch (viewMode()) {
    case ViewMode::TabsView:
        annotation.setComments(ui->tabWidget->fetchComments());
        break;
    case ViewMode::TableView:
        annotation.setComments(ui->tableView->fetchComments());
        break;
    }

    m_annotation = annotation;

    if (m_statusIsActive && m_isGlobal)
        m_globalStatus.setStatus(ui->statusComboBox->currentIndex());
}

void AnnotationEditorWidget::addComment(const Comment &comment)
{
    m_annotation.addComment(comment);
    ui->tabWidget->addCommentTab(comment);
}

void AnnotationEditorWidget::removeComment(int index)
{
    if ((m_annotation.commentsSize() > index) && (index >= 0)) {
        m_annotation.removeComment(index);
        ui->tabWidget->removeTab(index);
    }
}

void AnnotationEditorWidget::setStatusVisibility(bool hasStatus)
{
    ui->statusAddButton->setVisible(!hasStatus && m_isGlobal);
    ui->statusComboBox->setVisible(hasStatus && m_isGlobal);

    m_statusIsActive = hasStatus;
}


const Annotation &AnnotationEditorWidget::annotation() const
{
    return m_annotation;
}

void AnnotationEditorWidget::setAnnotation(const Annotation &annotation)
{
    m_annotation = annotation;
    fillFields();
}

void AnnotationEditorWidget::loadDefaultAnnotations(const QString &filename)
{
    m_defaults->loadFromFile(filename);
}

DefaultAnnotationsModel *AnnotationEditorWidget::defaultAnnotations() const
{
    return m_defaults.get();
}

void AnnotationEditorWidget::setTargetId(const QString &targetId)
{
    ui->targetIdEdit->setText(targetId);
}

QString AnnotationEditorWidget::targetId() const
{
    return ui->targetIdEdit->text();
}

void AnnotationEditorWidget::setCustomId(const QString &customId)
{
    m_customId = customId;
    ui->customIdEdit->setText(m_customId);
}

const QString &AnnotationEditorWidget::customId() const
{
    return m_customId;
}


} //namespace QmlDesigner
