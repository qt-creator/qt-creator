/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
