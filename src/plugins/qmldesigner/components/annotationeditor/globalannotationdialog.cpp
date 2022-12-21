// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalannotationdialog.h"
#include "annotation.h"
#include "annotationcommenttab.h"
#include "defaultannotations.h"
#include "annotationeditorwidget.h"
#include "annotationlistwidget.h"

#include <timelineicons.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMessageBox>
#include <QObject>
#include <QToolBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QAbstractButton>

namespace QmlDesigner {

GlobalAnnotationDialog::GlobalAnnotationDialog(ModelNode rootNode, QWidget *parent)
    : QDialog(parent)
    , m_statusIsActive(false)
    , m_defaults(std::make_unique<DefaultAnnotationsModel>())
    , m_editorWidget(new AnnotationEditorWidget(this))
    , m_annotationListWidget(new AnnotationListWidget(rootNode, this))
{
    setupUI();

    setStatus(m_globalStatus);
    m_editorWidget->setGlobal(true);

    connect(this, &QDialog::accepted,
            this, &GlobalAnnotationDialog::acceptedClicked);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &GlobalAnnotationDialog::acceptedClicked);

    connect(m_buttonBox, &QDialogButtonBox::clicked,
            this, &GlobalAnnotationDialog::buttonClicked);

    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::close);
}

GlobalAnnotationDialog::~GlobalAnnotationDialog() = default;

void GlobalAnnotationDialog::setStatus(GlobalAnnotationStatus status)
{
    m_editorWidget->setStatus(status);
}

GlobalAnnotationStatus GlobalAnnotationDialog::globalStatus() const
{
    return m_editorWidget->globalStatus();
}

GlobalAnnotationDialog::ViewMode GlobalAnnotationDialog::viewMode() const
{
    if (!m_tabWidget)
        return ViewMode::GlobalAnnotation;

    return (m_tabWidget->currentIndex() == 0)
            ? ViewMode::GlobalAnnotation
            : ViewMode::List;
}

void GlobalAnnotationDialog::saveAnnotationListChanges()
{
    m_annotationListWidget->saveAllChanges();
}

void GlobalAnnotationDialog::buttonClicked(QAbstractButton *button)
{
    if (button) {
        const QDialogButtonBox::StandardButton buttonType = m_buttonBox->standardButton(button);
        if (buttonType == QDialogButtonBox::Apply) {
            appliedClicked();
        }
    }
}

void GlobalAnnotationDialog::acceptedClicked()
{
    updateAnnotation();
    emit GlobalAnnotationDialog::acceptedDialog();
}

void GlobalAnnotationDialog::appliedClicked()
{
    updateAnnotation();
    emit GlobalAnnotationDialog::appliedDialog();
}

void GlobalAnnotationDialog::updateAnnotation()
{
    m_editorWidget->updateAnnotation();
    m_annotation = m_editorWidget->annotation();

    m_statusIsActive = (m_editorWidget->globalStatus().status() != GlobalAnnotationStatus::NoStatus);
    m_globalStatus = m_editorWidget->globalStatus();
}

void GlobalAnnotationDialog::setupUI()
{
    setWindowFlag(Qt::Tool, true);
    setWindowTitle(tr("Global Annotation Editor"));
    setModal(true);

    if (!QWidget::layout())
        new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(false);
    m_tabWidget->setMovable(false);
    QWidget::layout()->addWidget(m_tabWidget);

    m_tabWidget->addTab(m_editorWidget, tr("Global Annotation"));
    m_tabWidget->addTab(m_annotationListWidget, tr("All Annotations"));

    const QDialogButtonBox::StandardButtons buttonsToCreate =
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply;

    m_buttonBox = new QDialogButtonBox(buttonsToCreate, this);
    QWidget::layout()->addWidget(m_buttonBox);
}

const Annotation &GlobalAnnotationDialog::annotation() const
{
    return m_annotation;
}

void GlobalAnnotationDialog::setAnnotation(const Annotation &annotation)
{
    m_annotation = annotation;

    m_editorWidget->setAnnotation(m_annotation);
}

void GlobalAnnotationDialog::loadDefaultAnnotations(const QString &filename)
{
    m_editorWidget->loadDefaultAnnotations(filename);
}

DefaultAnnotationsModel *GlobalAnnotationDialog::defaultAnnotations() const
{
    return m_editorWidget->defaultAnnotations();
}


} //namespace QmlDesigner
