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
BasicAnnotationEditorDialog::BasicAnnotationEditorDialog(QWidget *parent)
    : QDialog(parent)
    , m_defaults(std::make_unique<DefaultAnnotationsModel>())
{
    setWindowFlag(Qt::Tool, true);
    setModal(true);
    loadDefaultAnnotations(DefaultAnnotationsModel::defaultJsonFilePath());

    connect(this, &QDialog::accepted, this, &BasicAnnotationEditorDialog::acceptedClicked);
}

BasicAnnotationEditorDialog::~BasicAnnotationEditorDialog() {}

Annotation const &BasicAnnotationEditorDialog::annotation() const
{
    return m_annotation;
}

void BasicAnnotationEditorDialog::setAnnotation(const Annotation &annotation)
{
    m_annotation = annotation;
    fillFields();
}

void BasicAnnotationEditorDialog::loadDefaultAnnotations(QString const &filename)
{
    m_defaults->loadFromFile(filename);
}

DefaultAnnotationsModel *BasicAnnotationEditorDialog::defaultAnnotations() const
{
    return m_defaults.get();
}

AnnotationEditorDialog::AnnotationEditorDialog(QWidget *parent,
                                               const QString &targetId,
                                               const QString &customId)
    : BasicAnnotationEditorDialog(parent)
    , ui(new Ui::AnnotationEditorDialog)
    , m_customId(customId)
{
    ui->setupUi(this);
    ui->targetIdEdit->setText(targetId);

    setWindowTitle(annotationEditorTitle);
}

AnnotationEditorDialog::~AnnotationEditorDialog()
{
    delete ui;
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
    updateAnnotation();
    emit AnnotationEditorDialog::acceptedDialog();
}

void AnnotationEditorDialog::fillFields()
{
    ui->customIdEdit->setText(m_customId);
    ui->tabWidget->setupComments(m_annotation.comments());
}

void AnnotationEditorDialog::updateAnnotation()
{
    m_customId = ui->customIdEdit->text();
    Annotation annotation;
    annotation.setComments(ui->tabWidget->fetchComments());
    m_annotation = annotation;
}

void AnnotationEditorDialog::addComment(const Comment &comment)
{
    m_annotation.addComment(comment);
    ui->tabWidget->addCommentTab(comment);
}

void AnnotationEditorDialog::removeComment(int index)
{
    m_annotation.removeComment(index);
    ui->tabWidget->removeTab(index);
}

} //namespace QmlDesigner
