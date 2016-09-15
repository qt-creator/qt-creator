/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "scxmleditorstack.h"
#include "scxmleditordocument.h"
#include "scxmltexteditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <utils/qtcassert.h>

using namespace ScxmlEditor;
using namespace ScxmlEditor::Internal;

ScxmlEditorStack::ScxmlEditorStack(QWidget *parent)
    : QStackedWidget(parent)
{
    setObjectName("ScxmlEditorStack");
}

void ScxmlEditorStack::add(ScxmlTextEditor *editor, QWidget *w)
{
    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeAboutToChange,
            this, &ScxmlEditorStack::modeAboutToChange);

    m_editors.append(editor);
    addWidget(w);
    connect(editor, &ScxmlTextEditor::destroyed,
            this, &ScxmlEditorStack::removeScxmlTextEditor);
}

void ScxmlEditorStack::removeScxmlTextEditor(QObject *xmlEditor)
{
    const int i = m_editors.indexOf(static_cast<ScxmlTextEditor*>(xmlEditor));
    QTC_ASSERT(i >= 0, return);

    QWidget *widget = this->widget(i);
    if (widget) {
        removeWidget(widget);
        widget->deleteLater();
    }
    m_editors.removeAt(i);
}

bool ScxmlEditorStack::setVisibleEditor(Core::IEditor *xmlEditor)
{
    const int i = m_editors.indexOf(static_cast<ScxmlTextEditor*>(xmlEditor));
    QTC_ASSERT(i >= 0, return false);

    if (i != currentIndex())
        setCurrentIndex(i);

    return true;
}

QWidget *ScxmlEditorStack::widgetForEditor(ScxmlTextEditor *xmlEditor)
{
    const int i = m_editors.indexOf(xmlEditor);
    QTC_ASSERT(i >= 0, return nullptr);

    return widget(i);
}

void ScxmlEditorStack::modeAboutToChange(Core::Id m)
{
    // Sync the editor when entering edit mode
    if (m == Core::Constants::MODE_EDIT) {
        for (const ScxmlTextEditor *editor: m_editors)
            if (ScxmlEditorDocument *document = qobject_cast<ScxmlEditorDocument*>(editor->textDocument()))
                document->syncXmlFromDesignWidget();
    }
}
