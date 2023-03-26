// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalannotationeditor.h"

#include "annotation.h"
#include "globalannotationdialog.h"

#include <coreplugin/icore.h>

#include <QMessageBox>

namespace QmlDesigner {

GlobalAnnotationEditor::GlobalAnnotationEditor(QObject *parent)
    : ModelNodeEditorProxy(parent)
{}

QWidget *GlobalAnnotationEditor::createWidget()
{
    GlobalAnnotationDialog *dialog = new GlobalAnnotationDialog(m_modelNode, Core::ICore::dialogParent());

    dialog->setStatus(m_modelNode.globalStatus());

    dialog->setAnnotation(this->m_modelNode.globalAnnotation());
    QObject::connect(dialog, &GlobalAnnotationDialog::acceptedDialog,
                     this, &GlobalAnnotationEditor::acceptedClicked);
    QObject::connect(dialog, &GlobalAnnotationDialog::rejected,
                     this, &GlobalAnnotationEditor::cancelClicked);
    QObject::connect(dialog, &GlobalAnnotationDialog::appliedDialog,
                     this, &GlobalAnnotationEditor::appliedClicked);
    return dialog;
};

void GlobalAnnotationEditor::removeFullAnnotation()
{
    auto &node = this->m_modelNode;
    if (!node.isValid())
        return;

    const QString dialogTitle = tr("Global Annotation");
    if (QMessageBox::question(Core::ICore::dialogParent(),
                              dialogTitle,
                              tr("Delete this annotation?"))
        == QMessageBox::Yes) {
        node.removeGlobalAnnotation();
        emit annotationChanged();
    }
}

void GlobalAnnotationEditor::acceptedClicked()
{
    applyChanges();

    hideWidget();

    emit accepted();

    emit annotationChanged();
    emit customIdChanged();
}

void GlobalAnnotationEditor::appliedClicked()
{
    applyChanges();

    emit applied();

    emit annotationChanged();
    emit customIdChanged();
}

void GlobalAnnotationEditor::cancelClicked()
{
    hideWidget();

    emit canceled();

    emit annotationChanged();
    emit customIdChanged();
}

void GlobalAnnotationEditor::applyChanges()
{
    if (GlobalAnnotationDialog * const dialog = qobject_cast<GlobalAnnotationDialog *>(widget())) {
        //first save global annotation:
        auto &node = this->m_modelNode;
        const Annotation &annotation = dialog->annotation();

        if (annotation.comments().isEmpty())
            node.removeGlobalAnnotation();
        else
            node.setGlobalAnnotation(annotation);

        const GlobalAnnotationStatus status = dialog->globalStatus();

        if (status.status() == GlobalAnnotationStatus::NoStatus)
            node.removeGlobalStatus();
        else
            node.setGlobalStatus(status);


        //then save annotations list:
        dialog->saveAnnotationListChanges();
    }
}

} //namespace QmlDesigner
