/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "actioneditor.h"

#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <metainfo.h>
#include <qmlmodelnodeproxy.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <propertyeditorvalue.h>

namespace QmlDesigner {

static ActionEditor *s_lastActionEditor = nullptr;

ActionEditor::ActionEditor(QObject *)
    : m_index(QModelIndex())
{
}

ActionEditor::~ActionEditor()
{
    hideWidget();
}

void ActionEditor::registerDeclarativeType()
{
    qmlRegisterType<ActionEditor>("HelperWidgets", 2, 0, "ActionEditor");
}

void ActionEditor::prepareDialog()
{
    if (s_lastActionEditor)
        s_lastActionEditor->hideWidget();
    s_lastActionEditor = this;

    m_dialog = new BindingEditorDialog(Core::ICore::dialogParent(),
                                       BindingEditorDialog::DialogType::ActionDialog);


    QObject::connect(m_dialog, &BindingEditorDialog::accepted,
                     this, &ActionEditor::accepted);
    QObject::connect(m_dialog, &BindingEditorDialog::rejected,
                     this, &ActionEditor::rejected);

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void ActionEditor::showWidget()
{
    prepareDialog();
    m_dialog->showWidget();
}

void ActionEditor::showWidget(int x, int y)
{
    prepareDialog();
    m_dialog->showWidget(x, y);
}

void ActionEditor::hideWidget()
{
    if (s_lastActionEditor == this)
        s_lastActionEditor = nullptr;
    if (m_dialog)
    {
        m_dialog->unregisterAutoCompletion(); //we have to do it separately, otherwise we have an autocompletion action override
        m_dialog->close();
    }
}

QString ActionEditor::bindingValue() const
{
    if (!m_dialog)
        return {};

    return m_dialog->editorValue();
}

void ActionEditor::setBindingValue(const QString &text)
{
    if (m_dialog)
        m_dialog->setEditorValue(text);
}

bool ActionEditor::hasModelIndex() const
{
    return m_index.isValid();
}

void ActionEditor::resetModelIndex()
{
    m_index = QModelIndex();
}

QModelIndex ActionEditor::modelIndex() const
{
    return m_index;
}

void ActionEditor::setModelIndex(const QModelIndex &index)
{
    m_index = index;
}

void ActionEditor::updateWindowName()
{
    if (!m_dialog.isNull())
    {
        m_dialog->setWindowTitle(tr("Connection Editor"));
        m_dialog->raise();
    }
}

} // QmlDesigner namespace
