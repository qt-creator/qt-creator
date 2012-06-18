/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "targetsetuppage.h"

#include "ui_targetsetuppage.h"
#include "buildconfigurationinfo.h"
#include "qt4project.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "qt4target.h"
#include "qt4basetargetfactory.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtversionfactory.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QLabel>
#include <QLayout>

using namespace Qt4ProjectManager;

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    QWizardPage(parent),
    m_importSearch(false),
    m_useScrollArea(true),
    m_maximumQtVersionNumber(INT_MAX, INT_MAX, INT_MAX),
    m_spacer(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding)),
    m_ignoreQtVersionChange(false),
    m_ui(new Internal::Ui::TargetSetupPage)
{
    m_ui->setupUi(this);
    QWidget *centralWidget = new QWidget(this);
    m_ui->scrollArea->setWidget(centralWidget);
    centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->layout()->setMargin(0);

    setTitle(tr("Target Setup"));

    connect(m_ui->descriptionLabel, SIGNAL(linkActivated(QString)),
            this, SIGNAL(noteTextLinkActivated()));

    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(qtVersionsChanged(QList<int>,QList<int>,QList<int>)));
}

void TargetSetupPage::initializePage()
{
    cleanupImportInfos();
    deleteWidgets();

    setupImportInfos();
    setupWidgets();
}

TargetSetupPage::~TargetSetupPage()
{
    deleteWidgets();
    delete m_ui;
    cleanupImportInfos();
}

bool TargetSetupPage::isTargetSelected(Core::Id id) const
{
    Qt4TargetSetupWidget *widget = m_widgets.value(id);
    return widget && widget->isTargetSelected();
}

bool TargetSetupPage::isComplete() const
{
    foreach (Qt4TargetSetupWidget *widget, m_widgets)
        if (widget->isTargetSelected())
            return true;
    return false;
}

void TargetSetupPage::setPreferredFeatures(const QSet<QString> &featureIds)
{
    m_preferredFeatures = featureIds;
}

void TargetSetupPage::setRequiredTargetFeatures(const QSet<QString> &featureIds)
{
    m_requiredTargetFeatures = featureIds;
}

void TargetSetupPage::setRequiredQtFeatures(const Core::FeatureSet &features)
{
    m_requiredQtFeatures = features;
}

void TargetSetupPage::setSelectedPlatform(const QString &platform)
{
    m_selectedPlatform = platform;
}

void TargetSetupPage::setMinimumQtVersion(const QtSupport::QtVersionNumber &number)
{
    m_minimumQtVersionNumber = number;
}

void TargetSetupPage::setMaximumQtVersion(const QtSupport::QtVersionNumber &number)
{
    m_maximumQtVersionNumber = number;
}

void TargetSetupPage::setImportSearch(bool b)
{
    m_importSearch = b;
}

void TargetSetupPage::setupWidgets()
{
    QLayout *layout = 0;
    if (m_useScrollArea)
        layout = m_ui->scrollArea->widget()->layout();
    else
        layout = m_ui->centralWidget->layout();

    // Target Page setup
    QList<Qt4BaseTargetFactory *> factories = ExtensionSystem::PluginManager::getObjects<Qt4BaseTargetFactory>();
    bool atLeastOneTargetSelected = false;
    foreach (Qt4BaseTargetFactory *factory, factories) {
        QList<Core::Id> ids = factory->supportedTargetIds();
        foreach (Core::Id id, ids) {
            if (!factory->targetFeatures(id).contains(m_requiredTargetFeatures))
                continue;

            QList<BuildConfigurationInfo> infos = BuildConfigurationInfo::filterBuildConfigurationInfos(m_importInfos, id);
            const QList<BuildConfigurationInfo> platformFilteredInfos =
                    BuildConfigurationInfo::filterBuildConfigurationInfosByPlatform(factory->availableBuildConfigurations(id,
                                                                                                                          m_proFilePath,
                                                                                                                          m_minimumQtVersionNumber,
                                                                                                                          m_maximumQtVersionNumber,
                                                                                                                          m_requiredQtFeatures),
                                                                                    m_selectedPlatform);


            Qt4TargetSetupWidget *widget =
                    factory->createTargetSetupWidget(id, m_proFilePath,
                                                     m_minimumQtVersionNumber,
                                                     m_maximumQtVersionNumber,
                                                     m_requiredQtFeatures,
                                                     m_importSearch, infos);
            if (widget) {
                bool selectTarget = false;
                if (!m_importInfos.isEmpty()) {
                    selectTarget = !infos.isEmpty();
                } else {
                    if (!m_preferredFeatures.isEmpty()) {
                        selectTarget = factory->targetFeatures(id).contains(m_preferredFeatures)
                                && factory->selectByDefault(id);
                    }
                    if (!m_selectedPlatform.isEmpty()) {
                        selectTarget = !platformFilteredInfos.isEmpty();
                    }
                }
                widget->setTargetSelected(selectTarget);
                atLeastOneTargetSelected |= selectTarget;
                m_widgets.insert(id, widget);
                m_factories.insert(widget, factory);
                layout->addWidget(widget);
                connect(widget, SIGNAL(selectedToggled()),
                        this, SIGNAL(completeChanged()));
                connect(widget, SIGNAL(newImportBuildConfiguration(BuildConfigurationInfo)),
                        this, SLOT(newImportBuildConfiguration(BuildConfigurationInfo)));
            }
        }
    }
    if (!atLeastOneTargetSelected) {
        Qt4TargetSetupWidget *widget = m_widgets.value(Core::Id(Constants::DESKTOP_TARGET_ID));
        if (widget)
            widget->setTargetSelected(true);
    }

    if (m_useScrollArea)
        layout->addItem(m_spacer);
    if (m_widgets.isEmpty()) {
        // Oh no one can create any targets
        m_ui->scrollArea->setVisible(false);
        m_ui->centralWidget->setVisible(false);
        m_ui->descriptionLabel->setVisible(false);
        m_ui->noValidQtVersionsLabel->setVisible(true);
    } else {
        m_ui->scrollArea->setVisible(m_useScrollArea);
        m_ui->centralWidget->setVisible(!m_useScrollArea);
        m_ui->descriptionLabel->setVisible(true);
        m_ui->noValidQtVersionsLabel->setVisible(false);
    }
}

void TargetSetupPage::deleteWidgets()
{
    QLayout *layout = 0;
    if (m_useScrollArea)
        layout = m_ui->scrollArea->widget()->layout();
    else
        layout = m_ui->centralWidget->layout();
    foreach (Qt4TargetSetupWidget *widget, m_widgets)
        delete widget;
    m_widgets.clear();
    m_factories.clear();
    if (m_useScrollArea)
        layout->removeItem(m_spacer);
}

void TargetSetupPage::setProFilePath(const QString &path)
{
    m_proFilePath = path;
    if (!m_proFilePath.isEmpty()) {
        m_ui->descriptionLabel->setText(tr("Qt Creator can set up the following targets for project <b>%1</b>:",
                                           "%1: Project name").arg(QFileInfo(m_proFilePath).baseName()));
    }

    deleteWidgets();
    setupWidgets();
}

void TargetSetupPage::setNoteText(const QString &text)
{
    m_ui->descriptionLabel->setText(text);
}

void TargetSetupPage::setupImportInfos()
{
    if (m_importSearch)
        m_importInfos = BuildConfigurationInfo::importBuildConfigurations(m_proFilePath);
}

void TargetSetupPage::cleanupImportInfos()
{
    // The same qt version can be twice in the list, for the following case
    // A Project with two import build using the same already existing qt version
    // If that qt version is deleted, it is replaced by ONE temporary qt version
    // So two entries in m_importInfos refer to the same qt version
    QSet<QtSupport::BaseQtVersion *> alreadyDeleted;
    foreach (const BuildConfigurationInfo &info, m_importInfos) {
        if (info.temporaryQtVersion) {
            if (!alreadyDeleted.contains(info.temporaryQtVersion)) {
                alreadyDeleted << info.temporaryQtVersion;
                delete info.temporaryQtVersion;
            }
        }
    }
    m_importInfos.clear();
}

void TargetSetupPage::newImportBuildConfiguration(const BuildConfigurationInfo &info)
{
    m_importInfos.append(info);
}

void TargetSetupPage::qtVersionsChanged(const QList<int> &added, const QList<int> &removed, const QList<int> &changed)
{
    Q_UNUSED(added)
    if (m_ignoreQtVersionChange)
        return;
    QMap<Core::Id, Qt4TargetSetupWidget *>::iterator it, end;
    end = m_widgets.end();
    it = m_widgets.begin();
    for ( ; it != end; ++it) {
        Qt4BaseTargetFactory *factory = m_factories.value(it.value());
        it.value()->updateBuildConfigurationInfos(factory->availableBuildConfigurations(it.key(),
                                                                                        m_proFilePath,
                                                                                        m_minimumQtVersionNumber,
                                                                                        m_maximumQtVersionNumber,
                                                                                        m_requiredQtFeatures));
    }

    QtSupport::QtVersionManager *mgr = QtSupport::QtVersionManager::instance();
    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (QtSupport::BaseQtVersion *tmpVersion = m_importInfos[i].temporaryQtVersion) {
            // Check whether we have a qt version now
            QtSupport::BaseQtVersion *version =
                    mgr->qtVersionForQMakeBinary(tmpVersion->qmakeCommand());
            if (version)
                replaceTemporaryQtVersion(tmpVersion, version->uniqueId());
        } else {
            // Check whether we need to replace the qt version id
            int oldId = m_importInfos[i].qtVersionId;
            if (removed.contains(oldId) || changed.contains(oldId)) {
                QString makefile = m_importInfos[i].directory + QLatin1Char('/') + m_importInfos[i].makefile;
                Utils::FileName qmakeBinary = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
                QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->qtVersionForQMakeBinary(qmakeBinary);
                if (version) {
                    replaceQtVersionWithQtVersion(oldId, version->uniqueId());
                } else {
                    version = QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmakeBinary);
                    replaceQtVersionWithTemporaryQtVersion(oldId, version);
                }
            }
        }
    }
}

void TargetSetupPage::replaceQtVersionWithQtVersion(int oldId, int newId)
{
    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importInfos[i].qtVersionId == oldId) {
            m_importInfos[i].qtVersionId = newId;
        }
    }
    QMap<Core::Id, Qt4TargetSetupWidget *>::const_iterator it, end;
    it = m_widgets.constBegin();
    end = m_widgets.constEnd();
    for ( ; it != end; ++it)
        (*it)->replaceQtVersionWithQtVersion(oldId, newId);
}

void TargetSetupPage::replaceQtVersionWithTemporaryQtVersion(int id, QtSupport::BaseQtVersion *version)
{
    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importInfos[i].qtVersionId == id) {
            m_importInfos[i].temporaryQtVersion = version;
            m_importInfos[i].qtVersionId = -1;
        }
    }
    QMap<Core::Id, Qt4TargetSetupWidget *>::const_iterator it, end;
    it = m_widgets.constBegin();
    end = m_widgets.constEnd();
    for ( ; it != end; ++it)
        (*it)->replaceQtVersionWithTemporaryQtVersion(id, version);
}

void TargetSetupPage::replaceTemporaryQtVersion(QtSupport::BaseQtVersion *version, int id)
{
    for (int i = 0; i < m_importInfos.size(); ++i) {
        if (m_importInfos[i].temporaryQtVersion == version) {
            m_importInfos[i].temporaryQtVersion = 0;
            m_importInfos[i].qtVersionId = id;
        }
    }
    QMap<Core::Id, Qt4TargetSetupWidget *>::const_iterator it, end;
    it = m_widgets.constBegin();
    end = m_widgets.constEnd();
    for ( ; it != end; ++it)
        (*it)->replaceTemporaryQtVersionWithQtVersion(version, id);
}

bool TargetSetupPage::setupProject(Qt4ProjectManager::Qt4Project *project)
{
    m_ignoreQtVersionChange = true;
    QtSupport::QtVersionManager *mgr = QtSupport::QtVersionManager::instance();
    QMap<Core::Id, Qt4TargetSetupWidget *>::const_iterator it, end;
    end = m_widgets.constEnd();
    it = m_widgets.constBegin();

    QSet<QtSupport::BaseQtVersion *> temporaryQtVersions;
    for ( ; it != end; ++it)
        foreach (QtSupport::BaseQtVersion *tempVersion, it.value()->usedTemporaryQtVersions())
            temporaryQtVersions.insert(tempVersion);

    foreach (QtSupport::BaseQtVersion *tempVersion, temporaryQtVersions) {
        QtSupport::BaseQtVersion *version = mgr->qtVersionForQMakeBinary(tempVersion->qmakeCommand());
        if (version) {
            replaceTemporaryQtVersion(tempVersion, version->uniqueId());
            delete tempVersion;
        } else {
            mgr->addVersion(tempVersion);
            replaceTemporaryQtVersion(tempVersion, tempVersion->uniqueId());
        }
    }

    m_ignoreQtVersionChange = false;

    it = m_widgets.constBegin();
    for ( ; it != end; ++it) {
        Qt4BaseTargetFactory *factory = m_factories.value(it.value());
        if (ProjectExplorer::Target *target = factory->create(project, it.key(), it.value()))
            project->addTarget(target);
    }

    // Select active target
    // a) Simulator target
    // b) Desktop target
    // c) the first target
    ProjectExplorer::Target *activeTarget = 0;
    QList<ProjectExplorer::Target *> targets = project->targets();
    foreach (ProjectExplorer::Target *t, targets) {
        if (t->id() == Core::Id(Constants::QT_SIMULATOR_TARGET_ID))
            activeTarget = t;
        else if (!activeTarget && t->id() == Core::Id(Constants::DESKTOP_TARGET_ID))
            activeTarget = t;
    }
    if (!activeTarget && !targets.isEmpty())
        activeTarget = targets.first();
    if (activeTarget)
        project->setActiveTarget(activeTarget);

    return true;
}

void TargetSetupPage::setUseScrollArea(bool b)
{
    m_useScrollArea = b;
}
