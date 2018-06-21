/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
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

#include "findmacrohandler.h"
#include "macroevent.h"
#include "macro.h"
#include "macrotextfind.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/ifindsupport.h>

#include <aggregation/aggregate.h>

using namespace Macros;
using namespace Macros::Internal;

static const char EVENTNAME[] = "Find";
static const quint8 TYPE = 0;
static const quint8 BEFORE = 1;
static const quint8 AFTER = 2;
static const quint8 FLAGS = 3;

static const quint8 FINDINCREMENTAL = 0;
static const quint8 FINDSTEP = 1;
static const quint8 REPLACE = 2;
static const quint8 REPLACESTEP = 3;
static const quint8 REPLACEALL = 4;
static const quint8 RESET = 5;


FindMacroHandler::FindMacroHandler():
    IMacroHandler()
{
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &FindMacroHandler::changeEditor);
}

bool FindMacroHandler::canExecuteEvent(const MacroEvent &macroEvent)
{
    return (macroEvent.id() == EVENTNAME);
}

bool FindMacroHandler::executeEvent(const MacroEvent &macroEvent)
{
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    if (!editor)
        return false;

    Aggregation::Aggregate *aggregate = Aggregation::Aggregate::parentAggregate(editor->widget());
    if (!aggregate)
        return false;

    Core::IFindSupport *currentFind = aggregate->component<Core::IFindSupport>();
    if (!currentFind)
        return false;

    switch (macroEvent.value(TYPE).toInt()) {
    case FINDINCREMENTAL:
        currentFind->findIncremental(macroEvent.value(BEFORE).toString(),
                                  (Core::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case FINDSTEP:
        currentFind->findStep(macroEvent.value(BEFORE).toString(),
                           (Core::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case REPLACE:
        currentFind->replace(macroEvent.value(BEFORE).toString(),
                             macroEvent.value(AFTER).toString(),
                             (Core::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case REPLACESTEP:
        currentFind->replaceStep(macroEvent.value(BEFORE).toString(),
                              macroEvent.value(AFTER).toString(),
                              (Core::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case REPLACEALL:
        currentFind->replaceAll(macroEvent.value(BEFORE).toString(),
                             macroEvent.value(AFTER).toString(),
                             (Core::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case RESET:
        currentFind->resetIncrementalSearch();
        break;
    }
    return true;
}

void FindMacroHandler::findIncremental(const QString &txt, Core::FindFlags findFlags)
{
    if (!isRecording())
        return;
    MacroEvent e;
    e.setId(EVENTNAME);
    e.setValue(BEFORE, txt);
    e.setValue(FLAGS, (int)findFlags);
    e.setValue(TYPE, FINDINCREMENTAL);
    addMacroEvent(e);
}

void FindMacroHandler::findStep(const QString &txt, Core::FindFlags findFlags)
{
    if (!isRecording())
        return;
    MacroEvent e;
    e.setId(EVENTNAME);
    e.setValue(BEFORE, txt);
    e.setValue(FLAGS, (int)findFlags);
    e.setValue(TYPE, FINDSTEP);
    addMacroEvent(e);
}

void FindMacroHandler::replace(const QString &before, const QString &after, Core::FindFlags findFlags)
{
    if (!isRecording())
        return;
    MacroEvent e;
    e.setId(EVENTNAME);
    e.setValue(BEFORE, before);
    e.setValue(AFTER, after);
    e.setValue(FLAGS, (int)findFlags);
    e.setValue(TYPE, REPLACE);
    addMacroEvent(e);
}

void FindMacroHandler::replaceStep(const QString &before, const QString &after, Core::FindFlags findFlags)
{
    if (!isRecording())
        return;
    MacroEvent e;
    e.setId(EVENTNAME);
    e.setValue(BEFORE, before);
    e.setValue(AFTER, after);
    e.setValue(FLAGS, (int)findFlags);
    e.setValue(TYPE, REPLACESTEP);
    addMacroEvent(e);
}

void FindMacroHandler::replaceAll(const QString &before, const QString &after, Core::FindFlags findFlags)
{
    if (!isRecording())
        return;
    MacroEvent e;
    e.setId(EVENTNAME);
    e.setValue(BEFORE, before);
    e.setValue(AFTER, after);
    e.setValue(FLAGS, (int)findFlags);
    e.setValue(TYPE, REPLACEALL);
    addMacroEvent(e);
}

void FindMacroHandler::resetIncrementalSearch()
{
    if (!isRecording())
        return;
    MacroEvent e;
    e.setId(EVENTNAME);
    e.setValue(TYPE, RESET);
    addMacroEvent(e);
}


void FindMacroHandler::changeEditor(Core::IEditor *editor)
{
    if (!isRecording() || !editor || !editor->widget())
        return;

    Aggregation::Aggregate *aggregate = Aggregation::Aggregate::parentAggregate(editor->widget());
    if (aggregate) {
        Core::IFindSupport *currentFind = aggregate->component<Core::IFindSupport>();
        if (currentFind) {
            MacroTextFind *macroFind = qobject_cast<MacroTextFind *>(currentFind);
            if (macroFind)
                return;

            aggregate->remove(currentFind);
            macroFind = new MacroTextFind(currentFind);
            aggregate->add(macroFind);

            // Connect all signals
            connect(macroFind, &MacroTextFind::allReplaced,
                    this, &FindMacroHandler::replaceAll);
            connect(macroFind, &MacroTextFind::incrementalFound,
                    this, &FindMacroHandler::findIncremental);
            connect(macroFind, &MacroTextFind::incrementalSearchReseted,
                    this, &FindMacroHandler::resetIncrementalSearch);
            connect(macroFind, &MacroTextFind::replaced,
                    this, &FindMacroHandler::replace);
            connect(macroFind, &MacroTextFind::stepFound,
                    this, &FindMacroHandler::findStep);
            connect(macroFind, &MacroTextFind::stepReplaced,
                    this, &FindMacroHandler::replaceStep);
        }
    }
}

void FindMacroHandler::startRecording(Macro* macro)
{
    IMacroHandler::startRecording(macro);
    Core::IEditor *current = Core::EditorManager::currentEditor();
    if (current)
        changeEditor(current);
}
