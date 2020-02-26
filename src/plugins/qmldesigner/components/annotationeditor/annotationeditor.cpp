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

#include "annotationeditordialog.h"
#include "annotation.h"

#include "qmlmodelnodeproxy.h"
#include <coreplugin/icore.h>

#include <QObject>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>

namespace QmlDesigner {

AnnotationEditor::AnnotationEditor(QObject *)
{
}

AnnotationEditor::~AnnotationEditor()
{
    hideWidget();
}

void AnnotationEditor::registerDeclarativeType()
{
    qmlRegisterType<AnnotationEditor>("HelperWidgets", 2, 0, "AnnotationEditor");
}

void AnnotationEditor::showWidget()
{
    m_dialog = new AnnotationEditorDialog(Core::ICore::dialogParent(),
                                          modelNode().validId(),
                                          modelNode().customId(),
                                          modelNode().annotation());

    QObject::connect(m_dialog, &AnnotationEditorDialog::accepted,
                     this, &AnnotationEditor::acceptedClicked);
    QObject::connect(m_dialog, &AnnotationEditorDialog::rejected,
                     this, &AnnotationEditor::cancelClicked);

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);

    m_dialog->show();
    m_dialog->raise();
}

void AnnotationEditor::showWidget(int x, int y)
{
    showWidget();
    m_dialog->move(x, y);
}

void AnnotationEditor::hideWidget()
{
    if (m_dialog)
        m_dialog->close();
    m_dialog = nullptr;
}

void AnnotationEditor::setModelNode(const ModelNode &modelNode)
{
    m_modelNodeBackend = {};
    m_modelNode = modelNode;
}

ModelNode AnnotationEditor::modelNode() const
{
    return m_modelNode;
}

void AnnotationEditor::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    if (!modelNodeBackend.isNull() && modelNodeBackend.isValid()) {
        m_modelNodeBackend = modelNodeBackend;

        const auto modelNodeBackendObject = modelNodeBackend.value<QObject*>();
        const auto backendObjectCasted =
                qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(modelNodeBackendObject);

        if (backendObjectCasted)
            m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();

        emit modelNodeBackendChanged();
    }
}

QVariant AnnotationEditor::modelNodeBackend() const
{
    return m_modelNodeBackend;
}

bool AnnotationEditor::hasCustomId() const
{
    if (m_modelNode.isValid())
        return m_modelNode.hasCustomId();
    return false;
}

bool AnnotationEditor::hasAnnotation() const
{
    if (m_modelNode.isValid())
        return m_modelNode.hasAnnotation();
    return false;
}

void AnnotationEditor::removeFullAnnotation()
{
    if (!m_modelNode.isValid())
        return;

    QString dialogTitle = tr("Annotation");
    if (!m_modelNode.customId().isNull()) {
        dialogTitle = m_modelNode.customId();
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
        m_modelNode.removeCustomId();
        m_modelNode.removeAnnotation();
    }

    emit customIdChanged();
    emit annotationChanged();
}

void AnnotationEditor::acceptedClicked()
{
    if (m_dialog) {
        QString customId = m_dialog->customId();
        Annotation annotation = m_dialog->annotation();

        m_modelNode.setCustomId(customId);

        if (annotation.comments().isEmpty())
            m_modelNode.removeAnnotation();
        else
            m_modelNode.setAnnotation(annotation);
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
