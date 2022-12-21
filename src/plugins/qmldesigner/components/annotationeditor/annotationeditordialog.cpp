// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationeditordialog.h"
#include "annotation.h"
#include "annotationcommenttab.h"
#include "defaultannotations.h"
#include "annotationeditorwidget.h"

#include <timelineicons.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QObject>
#include <QToolBar>
#include <QVBoxLayout>

namespace QmlDesigner {
AnnotationEditorDialog::AnnotationEditorDialog(QWidget *parent,
                                               const QString &targetId,
                                               const QString &customId)
    : QDialog(parent)
    , m_customId(customId)
    , m_defaults(std::make_unique<DefaultAnnotationsModel>())
    , m_editorWidget(new AnnotationEditorWidget(this, targetId, customId))
{
    setWindowTitle(tr("Annotation Editor"));

    setWindowFlag(Qt::Tool, true);
    setModal(true);

    const QDialogButtonBox::StandardButtons buttonsToCreate =
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply;

    m_buttonBox = new QDialogButtonBox(buttonsToCreate, this);

    if (!QWidget::layout())
        new QVBoxLayout(this);

    QWidget::layout()->addWidget(m_editorWidget);
    QWidget::layout()->addWidget(m_buttonBox);


    connect(this, &QDialog::accepted,
            this, &AnnotationEditorDialog::acceptedClicked);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &AnnotationEditorDialog::acceptedClicked);

    connect(m_buttonBox, &QDialogButtonBox::clicked,
            this, &AnnotationEditorDialog::buttonClicked);

    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::close);
}

AnnotationEditorDialog::~AnnotationEditorDialog() = default;

void AnnotationEditorDialog::buttonClicked(QAbstractButton *button)
{
    if (button) {
        const QDialogButtonBox::StandardButton buttonType = m_buttonBox->standardButton(button);
        if (buttonType == QDialogButtonBox::Apply) {
            appliedClicked();
        }
    }
}

void AnnotationEditorDialog::acceptedClicked()
{
    updateAnnotation();
    emit AnnotationEditorDialog::acceptedDialog();
}

void AnnotationEditorDialog::appliedClicked()
{
    updateAnnotation();
    emit AnnotationEditorDialog::appliedDialog();
}

void AnnotationEditorDialog::updateAnnotation()
{
    m_editorWidget->updateAnnotation();
    m_customId = m_editorWidget->customId();
    m_annotation = m_editorWidget->annotation();
}

const Annotation &AnnotationEditorDialog::annotation() const
{
    return m_annotation;
}

void AnnotationEditorDialog::setAnnotation(const Annotation &annotation)
{
    m_annotation = annotation;
    m_editorWidget->setAnnotation(m_annotation);
}

void AnnotationEditorDialog::loadDefaultAnnotations(const QString &filename)
{
    m_editorWidget->loadDefaultAnnotations(filename);
}

DefaultAnnotationsModel *AnnotationEditorDialog::defaultAnnotations() const
{
    return m_editorWidget->defaultAnnotations();
}

void AnnotationEditorDialog::setCustomId(const QString &customId)
{
    m_customId = customId;
    m_editorWidget->setCustomId(customId);
}

const QString &AnnotationEditorDialog::customId() const
{
    return m_customId;
}

} //namespace QmlDesigner
