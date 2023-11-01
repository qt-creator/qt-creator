// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findplugin.h"

#include "currentdocumentfind.h"
#include "findtoolbar.h"
#include "findtoolwindow.h"
#include "ifindfilter.h"
#include "searchresultwindow.h"
#include "textfindconstants.h"
#include "../actionmanager/actioncontainer.h"
#include "../actionmanager/actionmanager.h"
#include "../actionmanager/command.h"
#include "../coreconstants.h"
#include "../coreplugintr.h"
#include "../icontext.h"
#include "../icore.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QMenu>
#include <QStringListModel>
#include <QVector>
#include <QAction>

/*!
    \namespace Core::Internal::ItemDataRoles
    \internal
*/

/*!
    \class Core::Find
    \inmodule QtCreator
    \internal
*/

Q_DECLARE_METATYPE(Core::IFindFilter*)

using namespace Qt;
using namespace Utils;

namespace {
    const int MAX_COMPLETIONS = 50;
}

namespace Core {

struct CompletionEntry
{
    friend QDebug operator<<(QDebug d, const CompletionEntry &e)
    {
        QDebugStateSaver saver(d);
        d.noquote();
        d.nospace();
        d << "CompletionEntry(\"" << e.text << "\", flags="
          << "0x" << QString::number(e.findFlags, 16) << ')';
        return d;
    }

    QString text;
    FindFlags findFlags;
};

class CompletionModel : public QAbstractListModel
{
public:
    explicit CompletionModel(QObject *p = nullptr) : QAbstractListModel(p) {}

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_entries.size(); }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void writeSettings(QtcSettings *settings) const;
    void readSettings(QtcSettings *settings);

    void updateCompletion(const QString &text, FindFlags f);

private:
    QVector<CompletionEntry> m_entries;
};

QVariant CompletionModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const CompletionEntry &entry = m_entries.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return QVariant(entry.text);
        case Find::CompletionModelFindFlagsRole:
            return QVariant(int(entry.findFlags));
        default:
            break;
        }
    }
    return QVariant();
}

static Utils::Key completionSettingsArrayPrefix() { return "FindCompletions"; }
static Utils::Key completionSettingsTextKey() { return "Text"; }
static Utils::Key completionSettingsFlagsKey() { return "Flags"; }

void CompletionModel::writeSettings(QtcSettings *settings) const
{
    if (m_entries.isEmpty()) {
        settings->remove(completionSettingsArrayPrefix());
    } else {
        const int size = m_entries.size();
        settings->beginWriteArray(stringFromKey(completionSettingsArrayPrefix()), size);
        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            settings->setValue(completionSettingsTextKey(), m_entries.at(i).text);
            settings->setValue(completionSettingsFlagsKey(), int(m_entries.at(i).findFlags));
        }
        settings->endArray();
    }
}

void CompletionModel::readSettings(QtcSettings *settings)
{
    beginResetModel();
    const int size = settings->beginReadArray(stringFromKey(completionSettingsArrayPrefix()));
    m_entries.clear();
    m_entries.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        CompletionEntry entry;
        entry.text = settings->value(completionSettingsTextKey()).toString();
        entry.findFlags = FindFlags(settings->value(completionSettingsFlagsKey(), 0).toInt());
        if (!entry.text.isEmpty())
            m_entries.append(entry);
    }
    settings->endArray();
    endResetModel();
}

void CompletionModel::updateCompletion(const QString &text, FindFlags f)
{
    if (text.isEmpty())
         return;
     beginResetModel();
     Utils::erase(m_entries, Utils::equal(&CompletionEntry::text, text));
     m_entries.prepend({text, f});
     while (m_entries.size() > MAX_COMPLETIONS)
         m_entries.removeLast();
     endResetModel();
}

class FindPrivate : public QObject
{
public:
    bool isAnyFilterEnabled() const;
    void writeSettings();
    void setFindFlag(FindFlag flag, bool enabled);
    static void updateCompletion(const QString &text, QStringList &completions,
                                 QStringListModel *model);
    void setupMenu();
    void setupFilterMenuItems();
    void readSettings();

    Internal::CurrentDocumentFind *m_currentDocumentFind = nullptr;
    Internal::FindToolBar *m_findToolBar = nullptr;
    Internal::FindToolWindow *m_findDialog = nullptr;
    SearchResultWindow *m_searchResultWindow = nullptr;
    FindFlags m_findFlags;
    CompletionModel m_findCompletionModel;
    QStringListModel m_replaceCompletionModel;
    QStringList m_replaceCompletions;
    QAction *m_openFindDialog = nullptr;
};

Find *m_instance = nullptr;
FindPrivate *d = nullptr;

void Find::destroy()
{
    delete m_instance;
    m_instance = nullptr;
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
    d->m_findToolBar->setParent(nullptr);
    d->m_currentDocumentFind->removeConnections();
}

bool FindPrivate::isAnyFilterEnabled() const
{
    return Utils::anyOf(m_findDialog->findFilters(), &IFindFilter::isEnabled);
}

void Find::openFindDialog(IFindFilter *filter, const QString &findString)
{
    d->m_currentDocumentFind->acceptCandidate();
    const QString currentFindString = [findString] {
        if (!findString.isEmpty())
            return findString;
        if (d->m_findToolBar->isVisible()
            && QApplication::focusWidget() == d->m_findToolBar->focusWidget()
            && !d->m_findToolBar->getFindText().isEmpty()) {
            return d->m_findToolBar->getFindText();
        }
        if (d->m_currentDocumentFind->isEnabled())
            return d->m_currentDocumentFind->currentFindString();
        return QString();
    }();
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
    mfind->menu()->setTitle(Tr::tr("&Find/Replace"));
    mfind->appendGroup(Constants::G_FIND_CURRENTDOCUMENT);
    mfind->appendGroup(Constants::G_FIND_FILTERS);
    mfind->appendGroup(Constants::G_FIND_FLAGS);
    mfind->appendGroup(Constants::G_FIND_ACTIONS);
    Command *cmd;
    mfind->addSeparator(Constants::G_FIND_FLAGS);
    mfind->addSeparator(Constants::G_FIND_ACTIONS);

    ActionContainer *mfindadvanced = ActionManager::createMenu(Constants::M_FIND_ADVANCED);
    mfindadvanced->menu()->setTitle(Tr::tr("Advanced Find"));
    mfind->addMenu(mfindadvanced, Constants::G_FIND_FILTERS);
    m_openFindDialog = new QAction(Tr::tr("Open Advanced Find..."), this);
    m_openFindDialog->setIconText(Tr::tr("Advanced..."));
    cmd = ActionManager::registerAction(m_openFindDialog, Constants::ADVANCED_FIND);
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+F")));
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
    Command *cmd;

    ActionContainer *mfindadvanced = ActionManager::actionContainer(Constants::M_FIND_ADVANCED);
    bool haveEnabledFilters = false;
    const Id base("FindFilter.");
    const QList<IFindFilter *> sortedFilters = Utils::sorted(IFindFilter::allFindFilters(),
                                                             &IFindFilter::displayName);
    for (IFindFilter *filter : sortedFilters) {
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
    QtcSettings *settings = ICore::settings();
    settings->beginGroup("Find");
    settings->setValueWithDefault("Backward", bool(m_findFlags & FindBackward), false);
    settings->setValueWithDefault("CaseSensitively", bool(m_findFlags & FindCaseSensitively), false);
    settings->setValueWithDefault("WholeWords", bool(m_findFlags & FindWholeWords), false);
    settings->setValueWithDefault("RegularExpression",
                                  bool(m_findFlags & FindRegularExpression),
                                  false);
    settings->setValueWithDefault("PreserveCase", bool(m_findFlags & FindPreserveCase), false);
    m_findCompletionModel.writeSettings(settings);
    settings->setValueWithDefault("ReplaceStrings", m_replaceCompletions);
    settings->endGroup();
    m_findToolBar->writeSettings();
    m_findDialog->writeSettings();
    m_searchResultWindow->writeSettings();
}

void FindPrivate::readSettings()
{
    QtcSettings *settings = ICore::settings();
    settings->beginGroup("Find");
    {
        QSignalBlocker blocker(m_instance);
        Find::setBackward(settings->value("Backward", false).toBool());
        Find::setCaseSensitive(settings->value("CaseSensitively", false).toBool());
        Find::setWholeWord(settings->value("WholeWords", false).toBool());
        Find::setRegularExpression(settings->value("RegularExpression", false).toBool());
        Find::setPreserveCase(settings->value("PreserveCase", false).toBool());
    }
    m_findCompletionModel.readSettings(settings);
    m_replaceCompletions = settings->value("ReplaceStrings").toStringList();
    m_replaceCompletionModel.setStringList(m_replaceCompletions);
    settings->endGroup();
    m_findToolBar->readSettings();
    m_findDialog->readSettings();
    emit m_instance->findFlagsChanged(); // would have been done in the setXXX methods above
}

void Find::updateFindCompletion(const QString &text, FindFlags flags)
{
    d->m_findCompletionModel.updateCompletion(text, flags);
}

void Find::updateReplaceCompletion(const QString &text)
{
    FindPrivate::updateCompletion(text, d->m_replaceCompletions, &d->m_replaceCompletionModel);
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

QAbstractListModel *Find::findCompletionModel()
{
    return &(d->m_findCompletionModel);
}

QStringListModel *Find::replaceCompletionModel()
{
    return &(d->m_replaceCompletionModel);
}

} // namespace Core
