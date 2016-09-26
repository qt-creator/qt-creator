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

#include "iwizardfactory.h"

#include "actionmanager/actionmanager.h"
#include "documentmanager.h"
#include "icore.h"
#include "featureprovider.h"

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>
#include <utils/wizard.h>

#include <QAction>

/*!
    \class Core::IWizardFactory
    \mainclass

    \brief The class IWizardFactory is the base class for all wizard factories
    (for example shown in \gui {File | New}).

    The wizard interface is a very thin abstraction for the \gui{New...} wizards.
    Basically it defines what to show to the user in the wizard selection dialogs,
    and a hook that is called if the user selects the wizard.

    Wizards can then perform any operations they like, including showing dialogs and
    creating files. Often it is not necessary to create your own wizard from scratch,
    instead use one of the predefined wizards and adapt it to your needs.

    To make your wizard known to the system, add your IWizardFactory instance to the
    plugin manager's object pool in your plugin's initialize function:
    \code
        bool MyPlugin::initialize(const QStringList &arguments, QString *errorString)
        {
            // ... do setup
            addAutoReleasedObject(new MyWizardFactory);
            // ... do more setup
        }
    \endcode
    \sa Core::BaseFileWizard
    \sa Core::StandardFileWizard
*/

/*!
    \enum Core::IWizardFactory::WizardKind
    Used to specify what kind of objects the wizard creates. This information is used
    to show e.g. only wizards that create projects when selecting a \gui{New Project}
    menu item.
    \value FileWizard
        The wizard creates one or more files.
    \value ClassWizard
        The wizard creates a new class (e.g. source+header files).
    \value ProjectWizard
        The wizard creates a new project.
*/

/*!
    \fn IWizardFactory::IWizardFactory(QObject *parent)
    \internal
*/

/*!
    \fn IWizardFactory::~IWizardFactory()
    \internal
*/

/*!
    \fn Kind IWizardFactory::kind() const
    Returns what kind of objects are created by the wizard.
    \sa Kind
*/

/*!
    \fn QIcon IWizardFactory::icon() const
    Returns an icon to show in the wizard selection dialog.
*/

/*!
    \fn QString IWizardFactory::description() const
    Returns a translated description to show when this wizard is selected
    in the dialog.
*/

/*!
    \fn QString IWizardFactory::displayName() const
    Returns the translated name of the wizard, how it should appear in the
    dialog.
*/

/*!
    \fn QString IWizardFactory::id() const
    Returns an arbitrary id that is used for sorting within the category.
*/


/*!
    \fn QString IWizardFactory::category() const
    Returns a category ID to add the wizard to.
*/

/*!
    \fn QString IWizardFactory::displayCategory() const
    Returns the translated string of the category, how it should appear
    in the dialog.
*/

/*!
    \fn void IWizardFactory::runWizard(const QString &path,
                                      QWidget *parent,
                                      const QString &platform,
                                      const QVariantMap &variables)

    This function is executed when the wizard has been selected by the user
    for execution. Any dialogs the wizard opens should use the given \a parent.
    The \a path argument is a suggestion for the location where files should be
    created. The wizard should fill this in its path selection elements as a
    default path.
*/

using namespace Core;


namespace {
static QList<IFeatureProvider *> s_providerList;
QList<IWizardFactory *> s_allFactories;
QList<IWizardFactory::FactoryCreator> s_factoryCreators;
QAction *s_inspectWizardAction = 0;
bool s_areFactoriesLoaded = false;
bool s_isWizardRunning = false;
QWidget *s_currentWizard = nullptr;

// NewItemDialog reopening data:
class NewItemDialogData
{
public:
    void setData(const QString t, const QList<IWizardFactory *> f,
                 const QString &dl, const QVariantMap &ev)
    {
        QTC_ASSERT(!hasData(), return);

        QTC_ASSERT(!t.isEmpty(), return);
        QTC_ASSERT(!f.isEmpty(), return);

        title = t;
        factories = f;
        defaultLocation = dl;
        extraVariables = ev;
    }

    bool hasData() const { return !factories.isEmpty(); }

    void clear() {
        title.clear();
        factories.clear();
        defaultLocation.clear();
        extraVariables.clear();
    }

    void reopen() {
        if (!hasData())
            return;
        ICore::showNewItemDialog(title, factories, defaultLocation, extraVariables);
        clear();
    }

private:
    QString title;
    QList<IWizardFactory *> factories;
    QString defaultLocation;
    QVariantMap extraVariables;
};

NewItemDialogData s_reopenData;
}

static Id actionId(const IWizardFactory *factory)
{
    return factory->id().withPrefix("Wizard.Impl.");
}

QList<IWizardFactory*> IWizardFactory::allWizardFactories()
{
    if (!s_areFactoriesLoaded) {
        QTC_ASSERT(s_allFactories.isEmpty(), return s_allFactories);

        s_areFactoriesLoaded = true;

        QHash<Id, IWizardFactory *> sanityCheck;
        foreach (const FactoryCreator &fc, s_factoryCreators) {
            QList<IWizardFactory *> tmp = fc();
            foreach (IWizardFactory *newFactory, tmp) {
                QTC_ASSERT(newFactory, continue);
                IWizardFactory *existingFactory = sanityCheck.value(newFactory->id());

                QTC_ASSERT(existingFactory != newFactory, continue);
                if (existingFactory) {
                    qWarning("%s", qPrintable(tr("Factory with id=\"%1\" already registered. Deleting.")
                                              .arg(existingFactory->id().toString())));
                    delete newFactory;
                    continue;
                }

                QTC_ASSERT(!newFactory->m_action, continue);
                newFactory->m_action = new QAction(newFactory->displayName(), newFactory);
                ActionManager::registerAction(newFactory->m_action, actionId(newFactory));

                connect(newFactory->m_action, &QAction::triggered, newFactory, [newFactory]() {
                    if (!ICore::isNewItemDialogRunning()) {
                        QString path = newFactory->runPath(QString());
                        newFactory->runWizard(path, ICore::dialogParent(), Id(), QVariantMap());
                    }
                });

                sanityCheck.insert(newFactory->id(), newFactory);
                s_allFactories << newFactory;
            }
        }
    }

    return s_allFactories;
}

QString IWizardFactory::runPath(const QString &defaultPath)
{
    QString path = defaultPath;
    if (path.isEmpty()) {
        switch (kind()) {
        case IWizardFactory::ProjectWizard:
            // Project wizards: Check for projects directory or
            // use last visited directory of file dialog. Never start
            // at current.
            path = DocumentManager::useProjectsDirectory() ?
                       DocumentManager::projectsDirectory() :
                       DocumentManager::fileDialogLastVisitedDirectory();
            break;
        default:
            path = DocumentManager::fileDialogInitialDirectory();
            break;
        }
    }
    return path;
}

Utils::Wizard *IWizardFactory::runWizard(const QString &path, QWidget *parent, Id platform, const QVariantMap &variables)
{
    QTC_ASSERT(!s_isWizardRunning, return 0);

    s_isWizardRunning = true;
    ICore::updateNewItemDialogState();

    Utils::Wizard *wizard = runWizardImpl(path, parent, platform, variables);

    if (wizard) {
        s_currentWizard = wizard;
        // Connect while wizard exists:
        connect(m_action, &QAction::triggered, wizard, [wizard]() { ICore::raiseWindow(wizard); });
        connect(s_inspectWizardAction, &QAction::triggered,
                wizard, [wizard]() { wizard->showVariables(); });
        connect(wizard, &Utils::Wizard::finished, this, [wizard](int result) {
            if (result != QDialog::Accepted)
                s_reopenData.clear();
            wizard->deleteLater();
        });
        connect(wizard, &QObject::destroyed, this, [wizard]() {
            s_isWizardRunning = false;
            s_currentWizard = nullptr;
            s_inspectWizardAction->setEnabled(false);
            ICore::updateNewItemDialogState();
            s_reopenData.reopen();
        });
        s_inspectWizardAction->setEnabled(true);
        wizard->show();
        Core::ICore::registerWindow(wizard, Core::Context("Core.NewWizard"));
    } else {
        s_isWizardRunning = false;
        ICore::updateNewItemDialogState();
        s_reopenData.reopen();
    }
    return wizard;
}

bool IWizardFactory::isAvailable(Id platformId) const
{
    if (!platformId.isValid())
        return true;

    return availableFeatures(platformId).contains(requiredFeatures());
}

QSet<Id> IWizardFactory::supportedPlatforms() const
{
    QSet<Id> platformIds;

    foreach (Id platform, allAvailablePlatforms()) {
        if (isAvailable(platform))
            platformIds.insert(platform);
    }

    return platformIds;
}

void IWizardFactory::registerFactoryCreator(const IWizardFactory::FactoryCreator &creator)
{
    s_factoryCreators << creator;
}

QSet<Id> IWizardFactory::allAvailablePlatforms()
{
    QSet<Id> platforms;
    foreach (const IFeatureProvider *featureManager, s_providerList)
        platforms.unite(featureManager->availablePlatforms());

    return platforms;
}

QString IWizardFactory::displayNameForPlatform(Id i)
{
    foreach (const IFeatureProvider *featureManager, s_providerList) {
        const QString displayName = featureManager->displayNameForPlatform(i);
        if (!displayName.isEmpty())
            return displayName;
    }
    return QString();
}

void IWizardFactory::registerFeatureProvider(IFeatureProvider *provider)
{
    QTC_ASSERT(!s_providerList.contains(provider), return);
    s_providerList.append(provider);
}

bool IWizardFactory::isWizardRunning()
{
    return s_isWizardRunning;
}

QWidget *IWizardFactory::currentWizard()
{
    return s_currentWizard;
}

void IWizardFactory::requestNewItemDialog(const QString &title,
                                          const QList<IWizardFactory *> &factories,
                                          const QString &defaultLocation,
                                          const QVariantMap &extraVariables)
{
    s_reopenData.setData(title, factories, defaultLocation, extraVariables);
}

void IWizardFactory::destroyFeatureProvider()
{
    qDeleteAll(s_providerList);
    s_providerList.clear();
}

void IWizardFactory::clearWizardFactories()
{
    foreach (IWizardFactory *factory, s_allFactories)
        ActionManager::unregisterAction(factory->m_action, actionId(factory));

    qDeleteAll(s_allFactories);
    s_allFactories.clear();

    s_areFactoriesLoaded = false;
}

QSet<Id> IWizardFactory::pluginFeatures() const
{
    static QSet<Id> plugins;
    if (plugins.isEmpty()) {
        // Implicitly create a feature for each plugin loaded:
        foreach (ExtensionSystem::PluginSpec *s, ExtensionSystem::PluginManager::plugins()) {
            if (s->state() == ExtensionSystem::PluginSpec::Running)
                plugins.insert(Id::fromString(s->name()));
        }
    }
    return plugins;
}

QSet<Id> IWizardFactory::availableFeatures(Id platformId) const
{
    QSet<Id> availableFeatures;

    foreach (const IFeatureProvider *featureManager, s_providerList)
        availableFeatures.unite(featureManager->availableFeatures(platformId));

    return availableFeatures;
}

void IWizardFactory::initialize()
{
    connect(ICore::instance(), &ICore::coreAboutToClose, &IWizardFactory::clearWizardFactories);

    auto resetAction = new QAction(tr("Reload All Wizards"), ActionManager::instance());
    ActionManager::registerAction(resetAction, "Wizard.Factory.Reset");

    connect(resetAction, &QAction::triggered, &IWizardFactory::clearWizardFactories);
    connect(ICore::instance(), &ICore::newItemDialogStateChanged, resetAction,
            [resetAction]() { resetAction->setEnabled(!ICore::isNewItemDialogRunning()); });

    s_inspectWizardAction = new QAction(tr("Inspect Wizard State"), ActionManager::instance());
    ActionManager::registerAction(s_inspectWizardAction, "Wizard.Inspect");
}
