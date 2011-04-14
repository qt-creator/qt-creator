/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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
#include <texteditor/itexteditor.h>
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

class CompletionSupportPrivate : public QObject
{
    Q_OBJECT

public:
    explicit CompletionSupportPrivate(CompletionSupport *support);

private slots:
    void performCompletion(const TextEditor::CompletionItem &item);
    void cleanupCompletions();

public:
    QList<CompletionItem> getCompletions() const;
    void complete(ITextEditor *editor, CompletionPolicy policy, bool forced);

    CompletionSupport *m_support;
    Internal::CompletionWidget *m_completionList;
    int m_startPosition;
    bool m_checkCompletionTrigger;          // Whether to check for completion trigger after cleanup
    ITextEditor *m_editor;
    const QList<ICompletionCollector *> m_completionCollectors;
    ICompletionCollector *m_completionCollector;
    CompletionPolicy m_policy;
};

CompletionSupportPrivate::CompletionSupportPrivate(CompletionSupport *support) :
    m_support(support),
    m_completionList(0),
    m_startPosition(0),
    m_checkCompletionTrigger(false),
    m_editor(0),
    m_completionCollectors(ExtensionSystem::PluginManager::instance()
                           ->getObjects<ICompletionCollector>()),
    m_completionCollector(0),
    m_policy(SemanticCompletion)
{
}

QList<CompletionItem> CompletionSupportPrivate::getCompletions() const
{
    if (m_completionCollector)
        return m_completionCollector->getCompletions();
    return QList<CompletionItem>();
}


CompletionSupport::CompletionSupport()
  : QObject(Core::ICore::instance()),
    d(new CompletionSupportPrivate(this))
{
}

CompletionSupport::~CompletionSupport()
{
}

void CompletionSupportPrivate::performCompletion(const CompletionItem &item)
{
    item.collector->complete(item, m_completionList->typedChar());
    m_checkCompletionTrigger = true;
}

void CompletionSupportPrivate::cleanupCompletions()
{
    if (m_completionList)
        disconnect(m_completionList, SIGNAL(destroyed(QObject*)),
                   this, SLOT(cleanupCompletions()));

    if (m_checkCompletionTrigger)
        m_checkCompletionTrigger = m_completionCollector->shouldRestartCompletion();

    m_completionList = 0;
    m_completionCollector->cleanup();

    if (m_checkCompletionTrigger) {
        m_checkCompletionTrigger = false;

        // Only check for completion trigger when some text was entered
        if (m_editor->position() > m_startPosition)
            complete(m_editor, m_policy, false);
    }
}

bool CompletionSupport::isActive() const
{
    return d->m_completionList != 0;
}

CompletionPolicy CompletionSupport::policy() const
{
    return d->m_policy;
}

void CompletionSupport::complete(ITextEditor *editor, CompletionPolicy policy, bool forced)
{
    d->complete(editor, policy, forced);
}

void CompletionSupportPrivate::complete(ITextEditor *editor, CompletionPolicy policy, bool forced)
{
    m_completionCollector = 0;

    foreach (ICompletionCollector *collector, m_completionCollectors) {
        QTC_ASSERT(collector, continue);
        if (collector->supportsEditor(editor)
                && collector->supportsPolicy(policy)) {
            m_policy = policy;
            m_completionCollector = collector;
            break;
        }
    }

    if (!m_completionCollector)
        return;

    m_editor = editor;
    QList<CompletionItem> completionItems;

    int currentIndex = 0;

    if (!m_completionList) {
        if (!forced) {
            const CompletionSettings &completionSettings = m_completionCollector->completionSettings();
            if (completionSettings.m_completionTrigger == ManualCompletion)
                return;
            if (!m_completionCollector->triggersCompletion(editor))
                return;
        }

        m_startPosition = m_completionCollector->startCompletion(editor);
        completionItems = getCompletions();

        QTC_ASSERT(!(m_startPosition == -1 && completionItems.size() > 0), return);

        if (completionItems.isEmpty()) {
            cleanupCompletions();
            return;
        }

        m_completionList = new Internal::CompletionWidget(m_support, editor);

        connect(m_completionList, SIGNAL(itemSelected(TextEditor::CompletionItem)),
                this, SLOT(performCompletion(TextEditor::CompletionItem)));
        connect(m_completionList, SIGNAL(completionListClosed()),
                this, SLOT(cleanupCompletions()));

        // Make sure to clean up the completions if the list is destroyed without
        // emitting completionListClosed (can happen when no focus out event is received,
        // for example when switching applications on the Mac)
        connect(m_completionList, SIGNAL(destroyed(QObject*)),
                this, SLOT(cleanupCompletions()));
    } else {
        completionItems = getCompletions();

        if (completionItems.isEmpty()) {
            m_completionList->closeList();
            return;
        }

        if (m_completionList->explicitlySelected()) {
            const int originalIndex = m_completionList->currentCompletionItem().originalIndex;

            for (int index = 0; index < completionItems.size(); ++index) {
                if (completionItems.at(index).originalIndex == originalIndex) {
                    currentIndex = index;
                    break;
                }
            }
        }
    }

    m_completionList->setCompletionItems(completionItems);

    if (currentIndex)
        m_completionList->setCurrentIndex(currentIndex);

    // Partially complete when completion was forced
    if (forced && m_completionCollector->partiallyComplete(completionItems)) {
        m_checkCompletionTrigger = true;
        m_completionList->closeList();
    } else {
        m_completionList->showCompletions(m_startPosition);
    }
}

} // namespace TextEditor

#include "completionsupport.moc"
