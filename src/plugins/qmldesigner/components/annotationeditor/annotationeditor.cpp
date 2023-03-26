// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QObject::connect(dialog, &AnnotationEditorDialog::appliedDialog,
                     this, &AnnotationEditor::appliedClicked);
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

    if (QMessageBox::question(Core::ICore::dialogParent(),
                              node.customId().isNull() ? tr("Annotation") : node.customId(),
                              tr("Delete this annotation?"))
        == QMessageBox::Yes) {
        node.removeCustomId();
        node.removeAnnotation();
        emit customIdChanged();
        emit annotationChanged();
    }
}

void AnnotationEditor::acceptedClicked()
{
    applyChanges();

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

void AnnotationEditor::appliedClicked()
{
    applyChanges();

    emit applied();
    emit customIdChanged();
    emit annotationChanged();
}

void AnnotationEditor::applyChanges()
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
}

} //namespace QmlDesigner
