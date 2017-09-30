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

#include "findplugin.h"

#include "currentdocumentfind.h"
#include "findtoolbar.h"
#include "findtoolwindow.h"
#include "searchresultwindow.h"
#include "ifindfilter.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/coreplugin.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QMenu>
#include <QStringListModel>
#include <QAction>

#include <QtPlugin>
#include <QSettings>

/*!
    \namespace Core::Internal
    \internal
*/
/*!
    \namespace Core::Internal::ItemDataRoles
    \internal
*/

Q_DECLARE_METATYPE(Core::IFindFilter*)

namespace {
    const int MAX_COMPLETIONS = 50;
}

namespace Core {

class FindPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Core::Find)

public:
    bool isAnyFilterEnabled() const;
    void writeSettings();
    void setFindFlag(Core::FindFlag flag, bool enabled);
    void updateCompletion(const QString &text, QStringList &completions, QStringListModel *model);
    void setupMenu();
    void setupFilterMenuItems();
    void readSettings();

    Internal::CurrentDocumentFind *m_currentDocumentFind = 0;
    Internal::FindToolBar *m_findToolBar = 0;
    Internal::FindToolWindow *m_findDialog = 0;
    SearchResultWindow *m_searchResultWindow = 0;
    FindFlags m_findFlags;
    QStringListModel m_findCompletionModel;
    QStringListModel m_replaceCompletionModel;
    QStringList m_findCompletions;
    QStringList m_replaceCompletions;
    QAction *m_openFindDialog = 0;
};

Find *m_instance = 0;
FindPrivate *d = 0;

void Find::destroy()
{
    delete m_instance;
    m_instance = 0;
    if (d) {
        delete d->m_currentDocumentFind;
        delete d->m_findToolBar;
        delete d->m_findDialog;
        ExtensionSystem::PluginManager::removeObject(d->m_searchResultWindow);
        delete d->m_searchResultWindow;
        delete d;
    }
}

Find *Find::instance()
{
    return m_instance;
}

void Find::initialize()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = new Find;

    d = new FindPrivate;
    d->setupMenu();

    d->m_currentDocumentFind = new Internal::CurrentDocumentFind;

    d->m_findToolBar = new Internal::FindToolBar(d->m_currentDocumentFind);
    auto *findToolBarContext = new IContext(m_instance);
    findToolBarContext->setWidget(d->m_findToolBar);
    findToolBarContext->setContext(Context(Constants::C_FINDTOOLBAR));
    ICore::addContextObject(findToolBarContext);

    d->m_findDialog = new Internal::FindToolWindow;
    d->m_searchResultWindow = new SearchResultWindow(d->m_findDialog);
    ExtensionSystem::PluginManager::addObject(d->m_searchResultWindow);
    QObject::connect(ICore::instance(), &ICore::saveSettingsRequested, d, &FindPrivate::writeSettings);
}

void Find::extensionsInitialized()
{
    d->setupFilterMenuItems();
    d->readSettings();
}

void Find::aboutToShutdown()
{
    d->m_findToolBar->setVisible(false);
    d->m_findToolBar->setParent(0);
    d->m_currentDocumentFind->removeConnections();
}

bool FindPrivate::isAnyFilterEnabled() const
{
    return Utils::anyOf(m_findDialog->findFilters(), &IFindFilter::isEnabled);
}

void Find::openFindDialog(IFindFilter *filter)
{
    d->m_currentDocumentFind->acceptCandidate();
    const QString currentFindString =
        d->m_currentDocumentFind->isEnabled() ?
        d->m_currentDocumentFind->currentFindString() : QString();
    if (!currentFindString.isEmpty())
        d->m_findDialog->setFindText(currentFindString);
    d->m_findDialog->setCurrentFilter(filter);
    SearchResultWindow::instance()->openNewSearchPanel();
}

void FindPrivate::setupMenu()
{
    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *mfind = ActionManager::createMenu(Constants::M_FIND);
    medit->addMenu(mfind, Constants::G_EDIT_FIND);
    mfind->menu()->setTitle(tr("&Find/Replace"));
    mfind->appendGroup(Constants::G_FIND_CURRENTDOCUMENT);
    mfind->appendGroup(Constants::G_FIND_FILTERS);
    mfind->appendGroup(Constants::G_FIND_FLAGS);
    mfind->appendGroup(Constants::G_FIND_ACTIONS);
    Command *cmd;
    mfind->addSeparator(Constants::G_FIND_FLAGS);
    mfind->addSeparator(Constants::G_FIND_ACTIONS);

    ActionContainer *mfindadvanced = ActionManager::createMenu(Constants::M_FIND_ADVANCED);
    mfindadvanced->menu()->setTitle(tr("Advanced Find"));
    mfind->addMenu(mfindadvanced, Constants::G_FIND_FILTERS);
    m_openFindDialog = new QAction(tr("Open Advanced Find..."), this);
    m_openFindDialog->setIconText(tr("Advanced..."));
    cmd = ActionManager::registerAction(m_openFindDialog, Constants::ADVANCED_FIND);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F")));
    mfindadvanced->addAction(cmd);
    connect(m_openFindDialog, &QAction::triggered,
            this, [] { Find::openFindDialog(nullptr); });
}

static QString filterActionName(const IFindFilter *filter)
{
    return QLatin1String("    ") + filter->displayName();
}

void FindPrivate::setupFilterMenuItems()
{
    QList<IFindFilter*> findInterfaces = ExtensionSystem::PluginManager::getObjects<IFindFilter>();
    Command *cmd;

    ActionContainer *mfindadvanced = ActionManager::actionContainer(Constants::M_FIND_ADVANCED);
    bool haveEnabledFilters = false;
    const Id base("FindFilter.");
    QList<IFindFilter *> sortedFilters = findInterfaces;
    Utils::sort(sortedFilters, &IFindFilter::displayName);
    foreach (IFindFilter *filter, sortedFilters) {
        QAction *action = new QAction(filterActionName(filter), this);
        bool isEnabled = filter->isEnabled();
        if (isEnabled)
            haveEnabledFilters = true;
        action->setEnabled(isEnabled);
        cmd = ActionManager::registerAction(action, base.withSuffix(filter->id()));
        cmd->setDefaultKeySequence(filter->defaultShortcut());
        cmd->setAttribute(Command::CA_UpdateText);
        mfindadvanced->addAction(cmd);
        connect(action, &QAction::triggered, this, [filter] { Find::openFindDialog(filter); });
        connect(filter, &IFindFilter::enabledChanged, this, [filter, action] {
            action->setEnabled(filter->isEnabled());
            d->m_openFindDialog->setEnabled(d->isAnyFilterEnabled());
        });
        connect(filter, &IFindFilter::displayNameChanged,
                this, [filter, action] { action->setText(filterActionName(filter)); });
    }
    d->m_findDialog->setFindFilters(sortedFilters);
    d->m_openFindDialog->setEnabled(haveEnabledFilters);
}

FindFlags Find::findFlags()
{
    return d->m_findFlags;
}

void Find::setCaseSensitive(bool sensitive)
{
    d->setFindFlag(FindCaseSensitively, sensitive);
}

void Find::setWholeWord(bool wholeOnly)
{
    d->setFindFlag(FindWholeWords, wholeOnly);
}

void Find::setBackward(bool backward)
{
    d->setFindFlag(FindBackward, backward);
}

void Find::setRegularExpression(bool regExp)
{
    d->setFindFlag(FindRegularExpression, regExp);
}

void Find::setPreserveCase(bool preserveCase)
{
    d->setFindFlag(FindPreserveCase, preserveCase);
}

void FindPrivate::setFindFlag(FindFlag flag, bool enabled)
{
    bool hasFlag = m_findFlags & flag;
    if ((hasFlag && enabled) || (!hasFlag && !enabled))
        return;
    if (enabled)
        m_findFlags |= flag;
    else
        m_findFlags &= ~flag;
    if (flag != FindBackward)
        emit m_instance->findFlagsChanged();
}

bool Find::hasFindFlag(FindFlag flag)
{
    return d->m_findFlags & flag;
}

void FindPrivate::writeSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    settings->setValue(QLatin1String("Backward"), bool(m_findFlags & FindBackward));
    settings->setValue(QLatin1String("CaseSensitively"), bool(m_findFlags & FindCaseSensitively));
    settings->setValue(QLatin1String("WholeWords"), bool(m_findFlags & FindWholeWords));
    settings->setValue(QLatin1String("RegularExpression"), bool(m_findFlags & FindRegularExpression));
    settings->setValue(QLatin1String("PreserveCase"), bool(m_findFlags & FindPreserveCase));
    settings->setValue(QLatin1String("FindStrings"), m_findCompletions);
    settings->setValue(QLatin1String("ReplaceStrings"), m_replaceCompletions);
    settings->endGroup();
    m_findToolBar->writeSettings();
    m_findDialog->writeSettings();
    m_searchResultWindow->writeSettings();
}

void FindPrivate::readSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    {
        QSignalBlocker blocker(m_instance);
        Find::setBackward(settings->value(QLatin1String("Backward"), false).toBool());
        Find::setCaseSensitive(settings->value(QLatin1String("CaseSensitively"), false).toBool());
        Find::setWholeWord(settings->value(QLatin1String("WholeWords"), false).toBool());
        Find::setRegularExpression(settings->value(QLatin1String("RegularExpression"), false).toBool());
        Find::setPreserveCase(settings->value(QLatin1String("PreserveCase"), false).toBool());
    }
    m_findCompletions = settings->value(QLatin1String("FindStrings")).toStringList();
    m_replaceCompletions = settings->value(QLatin1String("ReplaceStrings")).toStringList();
    m_findCompletionModel.setStringList(m_findCompletions);
    m_replaceCompletionModel.setStringList(m_replaceCompletions);
    settings->endGroup();
    m_findToolBar->readSettings();
    m_findDialog->readSettings();
    emit m_instance->findFlagsChanged(); // would have been done in the setXXX methods above
}

void Find::updateFindCompletion(const QString &text)
{
    d->updateCompletion(text, d->m_findCompletions, &d->m_findCompletionModel);
}

void Find::updateReplaceCompletion(const QString &text)
{
    d->updateCompletion(text, d->m_replaceCompletions, &d->m_replaceCompletionModel);
}

void FindPrivate::updateCompletion(const QString &text, QStringList &completions, QStringListModel *model)
{
    if (text.isEmpty())
        return;
    completions.removeAll(text);
    completions.prepend(text);
    while (completions.size() > MAX_COMPLETIONS)
        completions.removeLast();
    model->setStringList(completions);
}

void Find::setUseFakeVim(bool on)
{
    if (d->m_findToolBar)
        d->m_findToolBar->setUseFakeVim(on);
}

void Find::openFindToolBar(FindDirection direction)
{
    if (d->m_findToolBar) {
        d->m_findToolBar->setBackward(direction == FindBackwardDirection);
        d->m_findToolBar->openFindToolBar();
    }
}

QStringListModel *Find::findCompletionModel()
{
    return &(d->m_findCompletionModel);
}

QStringListModel *Find::replaceCompletionModel()
{
    return &(d->m_replaceCompletionModel);
}

// declared in textfindconstants.h
QTextDocument::FindFlags textDocumentFlagsForFindFlags(FindFlags flags)
{
    QTextDocument::FindFlags textDocFlags;
    if (flags & FindBackward)
        textDocFlags |= QTextDocument::FindBackward;
    if (flags & FindCaseSensitively)
        textDocFlags |= QTextDocument::FindCaseSensitively;
    if (flags & FindWholeWords)
        textDocFlags |= QTextDocument::FindWholeWords;
    return textDocFlags;
}

} // namespace Core
