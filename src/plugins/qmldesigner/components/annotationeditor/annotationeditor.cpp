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

#include "annotationeditor.h"

#include "annotation.h"
#include "annotationeditordialog.h"

#include "qmlmodelnodeproxy.h"

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>

#include <QAction>
#include <QMessageBox>
#include <QObject>
#include <QToolBar>

namespace QmlDesigner {

AnnotationEditor::AnnotationEditor(QObject *parent)
    : ModelNodeEditorProxy(parent)
{}

AnnotationEditor::~AnnotationEditor() {}

QWidget *AnnotationEditor::createWidget()
{
    const auto &node = m_modelNode;
    auto dialog = new AnnotationEditorDialog(Core::ICore::dialogParent(),
                                             node.id(),
                                             node.customId());
    dialog->setAnnotation(node.annotation());

    QObject::connect(dialog,
                     &AnnotationEditorDialog::acceptedDialog,
                     this,
                     &AnnotationEditor::acceptedClicked);
    QObject::connect(dialog,
                     &AnnotationEditorDialog::rejected,
                     this,
                     &AnnotationEditor::cancelClicked);
    return dialog;
}

void AnnotationEditor::registerDeclarativeType()
{
    registerType<AnnotationEditor>("AnnotationEditor");
}

void AnnotationEditor::removeFullAnnotation()
{
    auto &node = this->m_modelNode;
    if (!node.isValid())
        return;

    QString dialogTitle = tr("Annotation");
    if (!node.customId().isNull()) {
        dialogTitle = node.customId();
    }
    QPointer<QMessageBox> deleteDialog = new QMessageBox(Core::ICore::dialogParent());
    deleteDialog->setWindowTitle(dialogTitle);
    deleteDialog->setText(tr("Delete this annotation?"));
    deleteDialog->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    deleteDialog->setDefaultButton(QMessageBox::Yes);

    int result = deleteDialog->exec();

    if (deleteDialog)
        deleteDialog->deleteLater();

    if (result == QMessageBox::Yes) {
        node.removeCustomId();
        node.removeAnnotation();
    }

    emit customIdChanged();
    emit annotationChanged();
}

void AnnotationEditor::acceptedClicked()
{
    if (const auto *dialog = qobject_cast<AnnotationEditorDialog *>(widget())) {
        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_ANNOTATION_ADDED);
        const QString customId = dialog->customId();
        const Annotation annotation = dialog->annotation();
        auto &node = this->m_modelNode;

        node.setCustomId(customId);

        if (annotation.comments().isEmpty())
            node.removeAnnotation();
        else
            node.setAnnotation(annotation);
    }

    hideWidget();

    emit accepted();
    emit customIdChanged();
    emit annotationChanged();
}

void AnnotationEditor::cancelClicked()
{
    hideWidget();

    emit canceled();

    emit customIdChanged();
    emit annotationChanged();
}

} //namespace QmlDesigner
