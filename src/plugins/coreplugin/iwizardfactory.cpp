/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
}

/* A utility to find all wizards supporting a view mode and matching a predicate */
QList<IWizardFactory*> findWizardFactories(const std::function<bool(IWizardFactory*)> &predicate)
{
    const QList<IWizardFactory *> allFactories = IWizardFactory::allWizardFactories();
    QList<IWizardFactory *> rc;
    auto cend = allFactories.constEnd();
    for (auto it = allFactories.constBegin(); it != cend; ++it) {
        if (predicate(*it))
            rc.push_back(*it);
    }
    return rc;
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
                        newFactory->runWizard(path, ICore::dialogParent(), QString(), QVariantMap());
                    }
                });

                sanityCheck.insert(newFactory->id(), newFactory);
                s_allFactories << newFactory;
            }
        }
    }

    return s_allFactories;
}

// Utility to find all registered wizards of a certain kind
QList<IWizardFactory*> IWizardFactory::wizardFactoriesOfKind(WizardKind kind)
{
    return findWizardFactories([kind](IWizardFactory *f) { return f->kind() == kind; });
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

Utils::Wizard *IWizardFactory::runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &variables)
{
    s_isWizardRunning = true;
    ICore::validateNewDialogIsRunning();

    Utils::Wizard *wizard = runWizardImpl(path, parent, platform, variables);

    if (wizard) {
        // Connect while wizard exists:
        connect(m_action, &QAction::triggered, wizard, [wizard]() { ICore::raiseWindow(wizard); });
        connect(s_inspectWizardAction, &QAction::triggered,
                wizard, [wizard]() { wizard->showVariables(); });
        connect(wizard, &Utils::Wizard::finished, [wizard]() {
            s_isWizardRunning = false;
            s_inspectWizardAction->setEnabled(false);
            ICore::validateNewDialogIsRunning();
            wizard->deleteLater();
        });
        s_inspectWizardAction->setEnabled(true);
        wizard->show();
        Core::ICore::registerWindow(wizard, Core::Context("Core.NewWizard"));
    } else {
        s_isWizardRunning = false;
        ICore::validateNewDialogIsRunning();
    }
    return wizard;
}

bool IWizardFactory::isAvailable(const QString &platformName) const
{
    if (platformName.isEmpty())
        return true;

    return availableFeatures(platformName).contains(requiredFeatures());
}

QStringList IWizardFactory::supportedPlatforms() const
{
    QStringList stringList;

    foreach (const QString &platform, allAvailablePlatforms()) {
        if (isAvailable(platform))
            stringList.append(platform);
    }

    return stringList;
}

void IWizardFactory::registerFactoryCreator(const IWizardFactory::FactoryCreator &creator)
{
    s_factoryCreators << creator;
}

QStringList IWizardFactory::allAvailablePlatforms()
{
    QStringList platforms;

    foreach (const IFeatureProvider *featureManager, s_providerList)
        platforms.append(featureManager->availablePlatforms());

    return platforms;
}

QString IWizardFactory::displayNameForPlatform(const QString &string)
{
    foreach (const IFeatureProvider *featureManager, s_providerList) {
        QString displayName = featureManager->displayNameForPlatform(string);
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

FeatureSet IWizardFactory::pluginFeatures() const
{
    static FeatureSet plugins;
    if (plugins.isEmpty()) {
        QStringList list;
        // Implicitly create a feature for each plugin loaded:
        foreach (ExtensionSystem::PluginSpec *s, ExtensionSystem::PluginManager::plugins()) {
            if (s->state() == ExtensionSystem::PluginSpec::Running)
                list.append(s->name());
        }
        plugins = FeatureSet::fromStringList(list);
    }
    return plugins;
}

FeatureSet IWizardFactory::availableFeatures(const QString &platformName) const
{
    FeatureSet availableFeatures;

    foreach (const IFeatureProvider *featureManager, s_providerList)
        availableFeatures |= featureManager->availableFeatures(platformName);

    return availableFeatures;
}

void IWizardFactory::initialize()
{
    connect(ICore::instance(), &ICore::coreAboutToClose, &IWizardFactory::clearWizardFactories);

    auto resetAction = new QAction(tr("Reload All Wizards"), ActionManager::instance());
    ActionManager::registerAction(resetAction, "Wizard.Factory.Reset");

    connect(resetAction, &QAction::triggered, &IWizardFactory::clearWizardFactories);
    connect(ICore::instance(), &ICore::newItemDialogRunningChanged, resetAction,
            [resetAction]() { resetAction->setEnabled(!ICore::isNewItemDialogRunning()); });

    s_inspectWizardAction = new QAction(tr("Inspect Wizard State"), ActionManager::instance());
    ActionManager::registerAction(s_inspectWizardAction, "Wizard.Inspect");
}
