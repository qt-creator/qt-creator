/**************************************************************************
**
** Copyright (c) 2013 Nicolas Arnaud-Cormos
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "findmacrohandler.h"
#include "macroevent.h"
#include "macro.h"
#include "macrotextfind.h"

#include <find/ifindsupport.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

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
    const Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeEditor(Core::IEditor*)));
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

    Find::IFindSupport *currentFind = aggregate->component<Find::IFindSupport>();
    if (!currentFind)
        return false;

    switch (macroEvent.value(TYPE).toInt()) {
    case FINDINCREMENTAL:
        currentFind->findIncremental(macroEvent.value(BEFORE).toString(),
                                  (Find::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case FINDSTEP:
        currentFind->findStep(macroEvent.value(BEFORE).toString(),
                           (Find::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case REPLACE:
        currentFind->replace(macroEvent.value(BEFORE).toString(),
                             macroEvent.value(AFTER).toString(),
                             (Find::FindFlags)macroEvent.value(FLAGS).toInt());
    case REPLACESTEP:
        currentFind->replaceStep(macroEvent.value(BEFORE).toString(),
                              macroEvent.value(AFTER).toString(),
                              (Find::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case REPLACEALL:
        currentFind->replaceAll(macroEvent.value(BEFORE).toString(),
                             macroEvent.value(AFTER).toString(),
                             (Find::FindFlags)macroEvent.value(FLAGS).toInt());
        break;
    case RESET:
        currentFind->resetIncrementalSearch();
        break;
    }
    return true;
}

void FindMacroHandler::findIncremental(const QString &txt, Find::FindFlags findFlags)
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

void FindMacroHandler::findStep(const QString &txt, Find::FindFlags findFlags)
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

void FindMacroHandler::replace(const QString &before, const QString &after, Find::FindFlags findFlags)
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

void FindMacroHandler::replaceStep(const QString &before, const QString &after, Find::FindFlags findFlags)
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

void FindMacroHandler::replaceAll(const QString &before, const QString &after, Find::FindFlags findFlags)
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
        Find::IFindSupport *currentFind = aggregate->component<Find::IFindSupport>();
        if (currentFind) {
            MacroTextFind *macroFind = qobject_cast<MacroTextFind *>(currentFind);
            if (macroFind)
                return;

            aggregate->remove(currentFind);
            macroFind = new MacroTextFind(currentFind);
            aggregate->add(macroFind);

            // Connect all signals
            connect(macroFind, SIGNAL(allReplaced(QString,QString,Find::FindFlags)),
                    this, SLOT(replaceAll(QString,QString,Find::FindFlags)));
            connect(macroFind, SIGNAL(incrementalFound(QString,Find::FindFlags)),
                    this, SLOT(findIncremental(QString,Find::FindFlags)));
            connect(macroFind, SIGNAL(incrementalSearchReseted()),
                    this, SLOT(resetIncrementalSearch()));
            connect(macroFind, SIGNAL(replaced(QString,QString,Find::FindFlags)),
                    this, SLOT(replace(QString,QString,Find::FindFlags)));
            connect(macroFind, SIGNAL(stepFound(QString,Find::FindFlags)),
                    this, SLOT(findStep(QString,Find::FindFlags)));
            connect(macroFind, SIGNAL(stepReplaced(QString,QString,Find::FindFlags)),
                    this, SLOT(replaceStep(QString,QString,Find::FindFlags)));
        }
    }
}

void FindMacroHandler::startRecording(Macros::Macro* macro)
{
    IMacroHandler::startRecording(macro);
    Core::IEditor *current = Core::EditorManager::currentEditor();
    if (current)
        changeEditor(current);
}
