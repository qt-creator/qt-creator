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

#include "globalannotationeditor.h"

#include "annotation.h"
#include "annotationeditordialog.h"


#include <coreplugin/icore.h>
#include <QMessageBox>

namespace QmlDesigner {

GlobalAnnotationEditor::GlobalAnnotationEditor(QObject *parent)
    : ModelNodeEditorProxy(parent)
{}

GlobalAnnotationEditor::~GlobalAnnotationEditor() {}

QWidget *GlobalAnnotationEditor::createWidget()
{
    auto *dialog = new AnnotationEditorDialog(Core::ICore::dialogParent());

    dialog->setGlobal(true);
    dialog->setStatus(m_modelNode.globalStatus());

    dialog->setAnnotation(this->m_modelNode.globalAnnotation());
    QObject::connect(dialog,
                     &AnnotationEditorDialog::acceptedDialog,
                     this,
                     &GlobalAnnotationEditor::acceptedClicked);
    QObject::connect(dialog,
                     &AnnotationEditorDialog::rejected,
                     this,
                     &GlobalAnnotationEditor::cancelClicked);
    return dialog;
};

void GlobalAnnotationEditor::removeFullAnnotation()
{
    auto &node = this->m_modelNode;
    if (!node.isValid())
        return;

    QString dialogTitle = tr("Global Annotation");
    if (QMessageBox::question(Core::ICore::dialogParent(),
                              tr("Global Annotation"),
                              tr("Delete this annotation?"))
        == QMessageBox::Yes) {
        node.removeGlobalAnnotation();
        emit annotationChanged();
    }
}

void GlobalAnnotationEditor::acceptedClicked()
{
    if (const auto *dialog = qobject_cast<AnnotationEditorDialog *>(widget())) {
        auto &node = this->m_modelNode;
        const Annotation annotation = dialog->annotation();

        if (annotation.comments().isEmpty())
            node.removeGlobalAnnotation();
        else
            node.setGlobalAnnotation(annotation);

        const GlobalAnnotationStatus status = dialog->globalStatus();

        if (status.status() == GlobalAnnotationStatus::NoStatus)
            node.removeGlobalStatus();
        else
            node.setGlobalStatus(status);
    }

    hideWidget();

    emit accepted();
    emit annotationChanged();
}

void GlobalAnnotationEditor::cancelClicked()
{
    hideWidget();

    emit canceled();
    emit annotationChanged();
}

} //namespace QmlDesigner
