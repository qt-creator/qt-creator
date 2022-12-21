// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

void ScxmlEditorStack::modeAboutToChange(Utils::Id m)
{
    // Sync the editor when entering edit mode
    if (m == Core::Constants::MODE_EDIT) {
        for (auto editor: std::as_const(m_editors))
            if (auto document = qobject_cast<ScxmlEditorDocument*>(editor->textDocument()))
                document->syncXmlFromDesignWidget();
    }
}
