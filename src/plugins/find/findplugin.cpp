/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "findplugin.h"

#include "textfindconstants.h"
#include "currentdocumentfind.h"
#include "findtoolbar.h"
#include "findtoolwindow.h"
#include "searchresultwindow.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QSettings>

/*!
    \namespace Find
    The Find namespace provides everything that has to do with search term based searches.
*/

/*!
    \namespace Find::Internal
    \internal
*/
/*!
    \namespace Find::Internal::ItemDataRoles
    \internal
*/

Q_DECLARE_METATYPE(Find::IFindFilter*)

namespace {
    const int MAX_COMPLETIONS = 50;
}

using namespace Find;
using namespace Find::Internal;

FindPlugin *FindPlugin::m_instance = 0;

FindPlugin::FindPlugin()
  : m_currentDocumentFind(0),
    m_findToolBar(0),
    m_findDialog(0),
    m_findCompletionModel(new QStringListModel(this)),
    m_replaceCompletionModel(new QStringListModel(this))
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
}

FindPlugin::~FindPlugin()
{
    m_instance = 0;
    delete m_currentDocumentFind;
    delete m_findToolBar;
    delete m_findDialog;
}

FindPlugin *FindPlugin::instance()
{
    return m_instance;
}

bool FindPlugin::initialize(const QStringList &, QString *)
{
    setupMenu();

    m_currentDocumentFind = new CurrentDocumentFind;

    m_findToolBar = new FindToolBar(this, m_currentDocumentFind);
    m_findDialog = new FindToolWindow(this);
    SearchResultWindow *searchResultWindow = new SearchResultWindow;
    addAutoReleasedObject(searchResultWindow);
    return true;
}

void FindPlugin::extensionsInitialized()
{
    setupFilterMenuItems();
    readSettings();
}

void FindPlugin::shutdown()
{
    m_findToolBar->setVisible(false);
    m_findToolBar->setParent(0);
    m_currentDocumentFind->removeConnections();
    writeSettings();
}

void FindPlugin::filterChanged()
{
    IFindFilter *changedFilter = qobject_cast<IFindFilter *>(sender());
    QAction *action = m_filterActions.value(changedFilter);
    QTC_ASSERT(changedFilter, return);
    QTC_ASSERT(action, return);
    action->setEnabled(changedFilter->isEnabled());
    bool haveEnabledFilters = false;
    foreach (IFindFilter *filter, m_filterActions.keys()) {
        if (filter->isEnabled()) {
            haveEnabledFilters = true;
            break;
        }
    }
    m_openFindDialog->setEnabled(haveEnabledFilters);
}

void FindPlugin::openFindFilter()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QTC_ASSERT(action, return);
    IFindFilter *filter = action->data().value<IFindFilter *>();
    if (m_currentDocumentFind->candidateIsEnabled())
        m_currentDocumentFind->acceptCandidate();
    QString currentFindString = (m_currentDocumentFind->isEnabled() ? m_currentDocumentFind->currentFindString() : "");
    if (!currentFindString.isEmpty())
        m_findDialog->setFindText(currentFindString);
    m_findDialog->open(filter);
}

void FindPlugin::setupMenu()
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *medit = am->actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *mfind = am->createMenu(Constants::M_FIND);
    medit->addMenu(mfind, Core::Constants::G_EDIT_FIND);
    mfind->menu()->setTitle(tr("&Find/Replace"));
    mfind->appendGroup(Constants::G_FIND_CURRENTDOCUMENT);
    mfind->appendGroup(Constants::G_FIND_FILTERS);
    mfind->appendGroup(Constants::G_FIND_FLAGS);
    mfind->appendGroup(Constants::G_FIND_ACTIONS);
    QList<int> globalcontext = QList<int>() << Core::Constants::C_GLOBAL_ID;
    Core::Command *cmd;
    QAction *separator;
    separator = new QAction(this);
    separator->setSeparator(true);
    cmd = am->registerAction(separator, QLatin1String("Find.Sep.Flags"), globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    separator = new QAction(this);
    separator->setSeparator(true);
    cmd = am->registerAction(separator, QLatin1String("Find.Sep.Actions"), globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);

    Core::ActionContainer *mfindadvanced = am->createMenu(Constants::M_FIND_ADVANCED);
    mfindadvanced->menu()->setTitle(tr("Advanced Find"));
    mfind->addMenu(mfindadvanced, Constants::G_FIND_FILTERS);
    m_openFindDialog = new QAction(tr("Open Advanced Find..."), this);
    cmd = am->registerAction(m_openFindDialog, QLatin1String("Find.Dialog"), globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F")));
    mfindadvanced->addAction(cmd);
    connect(m_openFindDialog, SIGNAL(triggered()), this, SLOT(openFindFilter()));
}

void FindPlugin::setupFilterMenuItems()
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QList<IFindFilter*> findInterfaces =
        ExtensionSystem::PluginManager::instance()->getObjects<IFindFilter>();
    Core::Command *cmd;
    QList<int> globalcontext = QList<int>() << Core::Constants::C_GLOBAL_ID;

    Core::ActionContainer *mfindadvanced = am->actionContainer(Constants::M_FIND_ADVANCED);
    m_filterActions.clear();
    bool haveEnabledFilters = false;
    foreach (IFindFilter *filter, findInterfaces) {
        QAction *action = new QAction(QString("    %1").arg(filter->name()), this);
        bool isEnabled = filter->isEnabled();
        if (isEnabled)
            haveEnabledFilters = true;
        action->setEnabled(isEnabled);
        action->setData(qVariantFromValue(filter));
        cmd = am->registerAction(action, QLatin1String("FindFilter.")+filter->id(), globalcontext);
        cmd->setDefaultKeySequence(filter->defaultShortcut());
        mfindadvanced->addAction(cmd);
        m_filterActions.insert(filter, action);
        connect(action, SIGNAL(triggered(bool)), this, SLOT(openFindFilter()));
        connect(filter, SIGNAL(changed()), this, SLOT(filterChanged()));
    }
    m_findDialog->setFindFilters(findInterfaces);
    m_openFindDialog->setEnabled(haveEnabledFilters);
}

QTextDocument::FindFlags FindPlugin::findFlags() const
{
    return m_findFlags;
}

void FindPlugin::setCaseSensitive(bool sensitive)
{
    setFindFlag(QTextDocument::FindCaseSensitively, sensitive);
}

void FindPlugin::setWholeWord(bool wholeOnly)
{
    setFindFlag(QTextDocument::FindWholeWords, wholeOnly);
}

void FindPlugin::setBackward(bool backward)
{
    setFindFlag(QTextDocument::FindBackward, backward);
}

void FindPlugin::setFindFlag(QTextDocument::FindFlag flag, bool enabled)
{
    bool hasFlag = hasFindFlag(flag);
    if ((hasFlag && enabled) || (!hasFlag && !enabled))
        return;
    if (enabled)
        m_findFlags |= flag;
    else
        m_findFlags &= ~flag;
    if (flag != QTextDocument::FindBackward)
        emit findFlagsChanged();
}

bool FindPlugin::hasFindFlag(QTextDocument::FindFlag flag)
{
    return m_findFlags & flag;
}

void FindPlugin::writeSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    settings->setValue("Backward", QVariant((m_findFlags & QTextDocument::FindBackward) != 0));
    settings->setValue("CaseSensitively", QVariant((m_findFlags & QTextDocument::FindCaseSensitively) != 0));
    settings->setValue("WholeWords", QVariant((m_findFlags & QTextDocument::FindWholeWords) != 0));
    settings->setValue("FindStrings", m_findCompletions);
    settings->setValue("ReplaceStrings", m_replaceCompletions);
    settings->endGroup();
    m_findToolBar->writeSettings();
    m_findDialog->writeSettings();
}

void FindPlugin::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    bool block = blockSignals(true);
    setBackward(settings->value("Backward", false).toBool());
    setCaseSensitive(settings->value("CaseSensitively", false).toBool());
    setWholeWord(settings->value("WholeWords", false).toBool());
    blockSignals(block);
    m_findCompletions = settings->value("FindStrings").toStringList();
    m_replaceCompletions = settings->value("ReplaceStrings").toStringList();
    m_findCompletionModel->setStringList(m_findCompletions);
    m_replaceCompletionModel->setStringList(m_replaceCompletions);
    settings->endGroup();
    m_findToolBar->readSettings();
    m_findDialog->readSettings();
    emit findFlagsChanged(); // would have been done in the setXXX methods above
}

void FindPlugin::updateFindCompletion(const QString &text)
{
    updateCompletion(text, m_findCompletions, m_findCompletionModel);
}

void FindPlugin::updateReplaceCompletion(const QString &text)
{
    updateCompletion(text, m_replaceCompletions, m_replaceCompletionModel);
}

void FindPlugin::updateCompletion(const QString &text, QStringList &completions, QStringListModel *model)
{
    if (text.isEmpty())
        return;
    completions.removeAll(text);
    completions.prepend(text);
    while (completions.size() > MAX_COMPLETIONS)
        completions.removeLast();
    model->setStringList(completions);
}

void FindPlugin::setUseFakeVim(bool on)
{
    if (m_findToolBar)
        m_findToolBar->setUseFakeVim(on);
}

void FindPlugin::openFindToolBar(FindDirection direction)
{
    if (m_findToolBar) {
        m_findToolBar->setBackward(direction == FindBackward);
        m_findToolBar->openFindToolBar();
    }
}

Q_EXPORT_PLUGIN(FindPlugin)
