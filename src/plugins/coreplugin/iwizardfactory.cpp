// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iwizardfactory.h"

#include "actionmanager/actionmanager.h"
#include "coreplugintr.h"
#include "documentmanager.h"
#include "icore.h"
#include "featureprovider.h"

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/filepath.h>
#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/wizard.h>

#include <QAction>
#include <QPainter>

/*!
    \class Core::IWizardFactory
    \inheaderfile coreplugin/iwizardfactory.h
    \inmodule QtCreator
    \ingroup mainclasses

    \brief The IWizardFactory class is the base class for all wizard factories.

    \note Instead of using this class, we recommend that you create JSON-based
    wizards, as instructed in \l{https://doc.qt.io/qtcreator/creator-project-wizards.html}
    {Adding New Custom Wizards}.

    The wizard interface is a very thin abstraction for the wizards in
    \uicontrol File > \uicontrol {New File} and \uicontrol{New Project}.
    Basically, it defines what to show to the user in the wizard selection dialogs,
    and a hook that is called if the user selects the wizard.

    Wizards can then perform any operations they like, including showing dialogs and
    creating files. Often it is not necessary to create your own wizard from scratch.
    Use one of the predefined wizards and adapt it to your needs.

    To make your wizard known to the system, add your IWizardFactory instance to the
    plugin manager's object pool in your plugin's initialize function:
    \code
        void MyPlugin::initialize()
        {
            // ... do setup
            addAutoReleasedObject(new MyWizardFactory);
            // ... do more setup
        }
    \endcode

    \sa Core::BaseFileWizardFactory, Core::BaseFileWizard
*/

/*!
    \enum Core::IWizardFactory::WizardKind
    Used to specify what kind of objects the wizard creates. This information is used
    to show e.g. only wizards that create projects when selecting a \uicontrol{New Project}
    menu item.
    \value FileWizard
        The wizard creates one or more files.
    \value ProjectWizard
        The wizard creates a new project.
*/

/*!
    \enum Core::IWizardFactory::WizardFlag

    Holds information about the created projects and files.

    \value PlatformIndependent
        The wizard creates projects that run on all platforms.
    \value ForceCapitalLetterForFileName
        The wizard uses an initial capital letter for the names of new files.
*/

/*!
    \fn Core::IWizardFactory::WizardKind Core::IWizardFactory::kind() const
    Returns what kind of objects are created by the wizard.
*/

/*!
    \fn QIcon Core::IWizardFactory::icon() const
    Returns an icon to show in the wizard selection dialog.
*/

/*!
    \fn QString Core::IWizardFactory::description() const
    Returns a translated description to show when this wizard is selected
    in the dialog.
*/

/*!
    \fn QString Core::IWizardFactory::displayName() const
    Returns the translated name of the wizard, how it should appear in the
    dialog.
*/

/*!
    \fn QString Core::IWizardFactory::id() const
    Returns an arbitrary id that is used for sorting within the category.
*/


/*!
    \fn QString Core::IWizardFactory::category() const
    Returns a category ID to add the wizard to.
*/

/*!
    \fn QString Core::IWizardFactory::displayCategory() const
    Returns the translated string of the category, how it should appear
    in the dialog.
*/

using namespace Core;
using namespace Utils;

namespace {
static QList<IFeatureProvider *> s_providerList;
QList<IWizardFactory *> s_allFactories;
QList<IWizardFactory::FactoryCreator> s_factoryCreators;
QAction *s_inspectWizardAction = nullptr;
bool s_areFactoriesLoaded = false;
bool s_isWizardRunning = false;
QWidget *s_currentWizard = nullptr;

// NewItemDialog reopening data:
class NewItemDialogData
{
public:
    void setData(const QString &t, const QList<IWizardFactory *> &f,
                 const FilePath &dl, const QVariantMap &ev)
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
    FilePath defaultLocation;
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
        for (const FactoryCreator &fc : std::as_const(s_factoryCreators)) {
            IWizardFactory *newFactory = fc();
            // skip factories referencing wizard page generators provided by plugins not loaded
            if (!newFactory)
                continue;
            IWizardFactory *existingFactory = sanityCheck.value(newFactory->id());

            QTC_ASSERT(existingFactory != newFactory, continue);
            if (existingFactory) {
                qWarning("%s", qPrintable(Tr::tr("Factory with id=\"%1\" already registered. Deleting.")
                                          .arg(existingFactory->id().toString())));
                delete newFactory;
                continue;
            }

            QTC_ASSERT(!newFactory->m_action, continue);
            newFactory->m_action = new QAction(newFactory->displayName(), newFactory);
            ActionManager::registerAction(newFactory->m_action, actionId(newFactory));

            connect(newFactory->m_action, &QAction::triggered, newFactory, [newFactory] {
                if (!ICore::isNewItemDialogRunning()) {
                    FilePath path = newFactory->runPath({});
                    newFactory->runWizard(path, ICore::dialogParent(), Id(), QVariantMap());
                }
            });

            sanityCheck.insert(newFactory->id(), newFactory);
            s_allFactories << newFactory;
        }
    }

    return s_allFactories;
}

FilePath IWizardFactory::runPath(const FilePath &defaultPath) const
{
    FilePath path = defaultPath;
    if (path.isEmpty()) {
        switch (kind()) {
        case IWizardFactory::ProjectWizard:
            // Project wizards: Check for projects directory or
            // use last visited directory of file dialog. Never start
            // at current.
            path = DocumentManager::useProjectsDirectory()
                       ? DocumentManager::projectsDirectory()
                       : DocumentManager::fileDialogLastVisitedDirectory();
            break;
        default:
            path = DocumentManager::fileDialogInitialDirectory();
            break;
        }
    }
    return path;
}

/*!
    Creates the wizard that the user selected for execution on the operating
    system \a platform with \a variables.

    Any dialogs the wizard opens should use the given \a parent.
    The \a path argument is a suggestion for the location where files should be
    created. The wizard should fill this in its path selection elements as a
    default path.

    When \a showWizard is \c false, the wizard instance is created and set up
    but not actually shown.
*/
Wizard *IWizardFactory::runWizard(const FilePath &path, QWidget *parent, Id platform,
                                  const QVariantMap &variables,
                                  bool showWizard)
{
    QTC_ASSERT(!s_isWizardRunning, return nullptr);

    s_isWizardRunning = true;
    ICore::updateNewItemDialogState();

    Utils::Wizard *wizard = runWizardImpl(path, parent, platform, variables, showWizard);


    if (wizard) {
        s_currentWizard = wizard;
        // Connect while wizard exists:
        if (m_action)
            connect(m_action, &QAction::triggered, wizard, [wizard] { ICore::raiseWindow(wizard); });
        connect(s_inspectWizardAction, &QAction::triggered,
                wizard, [wizard] { wizard->showVariables(); });
        connect(wizard, &Utils::Wizard::finished, this, [wizard](int result) {
            if (result != QDialog::Accepted)
                s_reopenData.clear();
            wizard->deleteLater();
        });
        connect(wizard, &QObject::destroyed, this, [] {
            s_isWizardRunning = false;
            s_currentWizard = nullptr;
            s_inspectWizardAction->setEnabled(false);
            ICore::updateNewItemDialogState();
            s_reopenData.reopen();
        });
        s_inspectWizardAction->setEnabled(true);
        if (showWizard) {
            wizard->show();
            Core::ICore::registerWindow(wizard, Core::Context("Core.NewWizard"));
        }
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

    const QSet<Id> platforms = allAvailablePlatforms();
    for (const Id platform : platforms) {
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
    for (const IFeatureProvider *featureManager : std::as_const(s_providerList))
        platforms.unite(featureManager->availablePlatforms());

    return platforms;
}

QString IWizardFactory::displayNameForPlatform(Id i)
{
    for (const IFeatureProvider *featureManager : std::as_const(s_providerList)) {
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
                                          const FilePath &defaultLocation,
                                          const QVariantMap &extraVariables)
{
    s_reopenData.setData(title, factories, defaultLocation, extraVariables);
}

QIcon IWizardFactory::themedIcon(const Utils::FilePath &iconMaskPath)
{
    return Utils::Icon({{iconMaskPath, Theme::PanelTextColorDark}}, Icon::Tint).icon();
}

void IWizardFactory::destroyFeatureProvider()
{
    qDeleteAll(s_providerList);
    s_providerList.clear();
}

void IWizardFactory::clearWizardFactories()
{
    for (IWizardFactory *factory : std::as_const(s_allFactories))
        ActionManager::unregisterAction(factory->m_action, actionId(factory));

    qDeleteAll(s_allFactories);
    s_allFactories.clear();

    s_areFactoriesLoaded = false;
}

QSet<Id> IWizardFactory::pluginFeatures()
{
    static QSet<Id> plugins;
    if (plugins.isEmpty()) {
        // Implicitly create a feature for each plugin loaded:
        const QVector<ExtensionSystem::PluginSpec *> pluginVector = ExtensionSystem::PluginManager::plugins();
        for (const ExtensionSystem::PluginSpec *s : pluginVector) {
            if (s->state() == ExtensionSystem::PluginSpec::Running)
                plugins.insert(Id::fromString(s->name()));
        }
    }
    return plugins;
}

QSet<Id> IWizardFactory::availableFeatures(Id platformId)
{
    QSet<Id> availableFeatures;

    for (const IFeatureProvider *featureManager : std::as_const(s_providerList))
        availableFeatures.unite(featureManager->availableFeatures(platformId));

    return availableFeatures;
}

void IWizardFactory::initialize()
{
    connect(ICore::instance(), &ICore::coreAboutToClose, &IWizardFactory::clearWizardFactories);

    auto resetAction = new QAction(Tr::tr("Reload All Wizards"), ActionManager::instance());
    ActionManager::registerAction(resetAction, "Wizard.Factory.Reset");

    connect(resetAction, &QAction::triggered, &IWizardFactory::clearWizardFactories);
    connect(ICore::instance(), &ICore::newItemDialogStateChanged, resetAction,
            [resetAction] { resetAction->setEnabled(!ICore::isNewItemDialogRunning()); });

    s_inspectWizardAction = new QAction(Tr::tr("Inspect Wizard State"), ActionManager::instance());
    ActionManager::registerAction(s_inspectWizardAction, "Wizard.Inspect");
}

static QIcon iconWithText(const QIcon &icon, const QString &text)
{
    if (icon.isNull()) {
        static const QIcon fallBack =
                IWizardFactory::themedIcon(":/utils/images/wizardicon-file.png");
        return iconWithText(fallBack, text);
    }

    if (text.isEmpty())
        return icon;

    QIcon iconWithText;
    for (const QSize &pixmapSize : icon.availableSizes()) {
        QPixmap pixmap = icon.pixmap(pixmapSize);
        const qreal originalPixmapDpr = pixmap.devicePixelRatio();
        pixmap.setDevicePixelRatio(1); // Hack for QTCREATORBUG-26315
        const int fontSize = pixmap.height() / 4;
        const int margin = pixmap.height() / 8;
        QFont font;
        font.setPixelSize(fontSize);
        font.setStretch(85);
        QPainter p(&pixmap);
        p.setPen(Utils::creatorTheme()->color(Theme::PanelTextColorDark));
        p.setFont(font);
        QTextOption textOption(Qt::AlignHCenter | Qt::AlignBottom);
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        p.drawText(pixmap.rect().adjusted(margin, margin, -margin, -margin), text, textOption);
        p.end();
        pixmap.setDevicePixelRatio(originalPixmapDpr);
        iconWithText.addPixmap(pixmap);
    }
    return iconWithText;
}

void IWizardFactory::setIcon(const QIcon &icon, const QString &iconText)
{
    m_icon = iconWithText(icon, iconText);
}

void IWizardFactory::setDetailsPageQmlPath(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    if (filePath.startsWith(':')) {
        m_detailsPageQmlPath.setScheme(QLatin1String("qrc"));
        QString path = filePath;
        path.remove(0, 1);
        m_detailsPageQmlPath.setPath(path);
    } else {
        m_detailsPageQmlPath = QUrl::fromLocalFile(filePath);
    }
}
