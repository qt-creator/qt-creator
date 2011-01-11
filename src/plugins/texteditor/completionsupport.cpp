/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "completionsupport.h"
#include "completionwidget.h"
#include "icompletioncollector.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditable.h>
#include <texteditor/completionsettings.h>
#include <utils/qtcassert.h>

#include <QtCore/QString>
#include <QtCore/QList>

namespace TextEditor {

CompletionSupport *CompletionSupport::instance()
{
    static CompletionSupport *m_instance = 0;
    if (!m_instance)
        m_instance = new CompletionSupport;
    return m_instance;
}

class CompletionSupportPrivate {
public:
    CompletionSupportPrivate();

    Internal::CompletionWidget *m_completionList;
    int m_startPosition;
    bool m_checkCompletionTrigger;          // Whether to check for completion trigger after cleanup
    ITextEditable *m_editor;
    const QList<ICompletionCollector *> m_completionCollectors;
    ICompletionCollector *m_completionCollector;
};

CompletionSupportPrivate::CompletionSupportPrivate() :
    m_completionList(0),
    m_startPosition(0),
    m_checkCompletionTrigger(false),
    m_editor(0),
    m_completionCollectors(ExtensionSystem::PluginManager::instance()
                           ->getObjects<ICompletionCollector>()),
    m_completionCollector(0)
{
}

CompletionSupport::CompletionSupport() :
    QObject(Core::ICore::instance()), d(new CompletionSupportPrivate)
{
}

CompletionSupport::~CompletionSupport()
{
}

void CompletionSupport::performCompletion(const CompletionItem &item)
{
    item.collector->complete(item, d->m_completionList->typedChar());
    d->m_checkCompletionTrigger = true;
}

void CompletionSupport::cleanupCompletions()
{
    if (d->m_completionList)
        disconnect(d->m_completionList, SIGNAL(destroyed(QObject*)),
                   this, SLOT(cleanupCompletions()));

    if (d->m_checkCompletionTrigger)
        d->m_checkCompletionTrigger = d->m_completionCollector->shouldRestartCompletion();

    d->m_completionList = 0;
    d->m_completionCollector->cleanup();

    if (d->m_checkCompletionTrigger) {
        d->m_checkCompletionTrigger = false;

        // Only check for completion trigger when some text was entered
        if (d->m_editor->position() > d->m_startPosition)
            autoComplete(d->m_editor, false);
    }
}

bool CompletionSupport::isActive() const
{
    return d->m_completionList != 0;
}

void CompletionSupport::autoComplete(ITextEditable *editor, bool forced)
{
    autoComplete_helper(editor, forced, /*quickFix = */ false);
}

void CompletionSupport::quickFix(ITextEditable *editor)
{
    autoComplete_helper(editor,
                        /*forced = */ true,
                        /*quickFix = */ true);
}

void CompletionSupport::autoComplete_helper(ITextEditable *editor, bool forced,
                                            bool quickFix)
{
    d->m_completionCollector = 0;

    foreach (ICompletionCollector *collector, d->m_completionCollectors) {
        if (quickFix)
            collector = qobject_cast<IQuickFixCollector *>(collector);

        if (collector && collector->supportsEditor(editor)) {
            d->m_completionCollector = collector;
            break;
        }
    }

    if (!d->m_completionCollector)
        return;

    d->m_editor = editor;
    QList<CompletionItem> completionItems;

    int currentIndex = 0;

    if (!d->m_completionList) {
        if (!forced) {
            const CompletionSettings &completionSettings = d->m_completionCollector->completionSettings();
            if (completionSettings.m_completionTrigger == ManualCompletion)
                return;
            if (!d->m_completionCollector->triggersCompletion(editor))
                return;
        }

        d->m_startPosition = d->m_completionCollector->startCompletion(editor);
        completionItems = getCompletions();

        QTC_ASSERT(!(d->m_startPosition == -1 && completionItems.size() > 0), return);

        if (completionItems.isEmpty()) {
            cleanupCompletions();
            return;
        }

        d->m_completionList = new Internal::CompletionWidget(this, editor);
        d->m_completionList->setQuickFix(quickFix);

        connect(d->m_completionList, SIGNAL(itemSelected(TextEditor::CompletionItem)),
                this, SLOT(performCompletion(TextEditor::CompletionItem)));
        connect(d->m_completionList, SIGNAL(completionListClosed()),
                this, SLOT(cleanupCompletions()));

        // Make sure to clean up the completions if the list is destroyed without
        // emitting completionListClosed (can happen when no focus out event is received,
        // for example when switching applications on the Mac)
        connect(d->m_completionList, SIGNAL(destroyed(QObject*)),
                this, SLOT(cleanupCompletions()));
    } else {
        completionItems = getCompletions();

        if (completionItems.isEmpty()) {
            d->m_completionList->closeList();
            return;
        }

        if (d->m_completionList->explicitlySelected()) {
            const int originalIndex = d->m_completionList->currentCompletionItem().originalIndex;

            for (int index = 0; index < completionItems.size(); ++index) {
                if (completionItems.at(index).originalIndex == originalIndex) {
                    currentIndex = index;
                    break;
                }
            }
        }
    }

    d->m_completionList->setCompletionItems(completionItems);

    if (currentIndex)
        d->m_completionList->setCurrentIndex(currentIndex);

    // Partially complete when completion was forced
    if (forced && d->m_completionCollector->partiallyComplete(completionItems)) {
        d->m_checkCompletionTrigger = true;
        d->m_completionList->closeList();
    } else {
        d->m_completionList->showCompletions(d->m_startPosition);
    }
}

QList<CompletionItem> CompletionSupport::getCompletions() const
{
    if (d->m_completionCollector)
        return d->m_completionCollector->getCompletions();

    return QList<CompletionItem>();
}

} // namespace TextEditor
