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

#include "globalannotationeditordialog.h"
#include "annotation.h"

#include "qmlmodelnodeproxy.h"
#include <coreplugin/icore.h>

#include <QObject>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>

namespace QmlDesigner {

GlobalAnnotationEditor::GlobalAnnotationEditor(QObject *)
{
}

GlobalAnnotationEditor::~GlobalAnnotationEditor()
{
    hideWidget();
}

void GlobalAnnotationEditor::showWidget()
{
    m_dialog = new GlobalAnnotationEditorDialog(Core::ICore::dialogParent(),
                                                modelNode().globalAnnotation(),
                                                modelNode().globalStatus());

    QObject::connect(m_dialog, &GlobalAnnotationEditorDialog::accepted,
                     this, &GlobalAnnotationEditor::acceptedClicked);
    QObject::connect(m_dialog, &GlobalAnnotationEditorDialog::rejected,
                     this, &GlobalAnnotationEditor::cancelClicked);

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);

    m_dialog->show();
    m_dialog->raise();
}

void GlobalAnnotationEditor::showWidget(int x, int y)
{
    showWidget();
    m_dialog->move(x, y);
}

void GlobalAnnotationEditor::hideWidget()
{
    if (m_dialog)
        m_dialog->close();
    m_dialog = nullptr;
}

void GlobalAnnotationEditor::setModelNode(const ModelNode &modelNode)
{
    m_modelNode = modelNode;
}

ModelNode GlobalAnnotationEditor::modelNode() const
{
    return m_modelNode;
}

bool GlobalAnnotationEditor::hasAnnotation() const
{
    if (m_modelNode.isValid())
        return m_modelNode.hasGlobalAnnotation();
    return false;
}

void GlobalAnnotationEditor::removeFullAnnotation()
{
    if (!m_modelNode.isValid())
        return;

    QString dialogTitle = tr("Global Annotation");
    QMessageBox *deleteDialog = new QMessageBox(Core::ICore::dialogParent());
    deleteDialog->setWindowTitle(dialogTitle);
    deleteDialog->setText(tr("Delete this annotation?"));
    deleteDialog->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    deleteDialog->setDefaultButton(QMessageBox::Yes);

    int result = deleteDialog->exec();
    if (deleteDialog) deleteDialog->deleteLater();

    if (result == QMessageBox::Yes) {
        m_modelNode.removeGlobalAnnotation();
    }

    emit annotationChanged();
}

void GlobalAnnotationEditor::acceptedClicked()
{
    if (m_dialog) {

        Annotation annotation = m_dialog->annotation();

        if (annotation.comments().isEmpty())
            m_modelNode.removeGlobalAnnotation();
        else
            m_modelNode.setGlobalAnnotation(annotation);

        GlobalAnnotationStatus status = m_dialog->globalStatus();

        if (status.status() == GlobalAnnotationStatus::NoStatus) {
            if (m_modelNode.hasGlobalStatus()) {
                m_modelNode.removeGlobalStatus();
            }
        }
        else {
            m_modelNode.setGlobalStatus(status);
        }
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
