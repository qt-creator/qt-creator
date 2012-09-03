/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "targetsetuppage.h"
#include "importwidget.h"

#include "buildconfigurationinfo.h"
#include "qt4buildconfiguration.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qmakekitinformation.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>
#include <utils/qtcprocess.h>

#include <QLabel>
#include <QMessageBox>
#include <QVariant>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace Qt4ProjectManager {
namespace Internal {

static const Core::Id QT_IS_TEMPORARY("Qt4PM.TempQt");
static const Core::Id PROFILE_IS_TEMPORARY("Qt4PM.TempProfile");
static const Core::Id TEMPORARY_OF_PROJECTS("Qt4PM.TempProject");

class TargetSetupPageUi
{
public:
    QWidget *centralWidget;
    QWidget *scrollAreaWidget;
    QScrollArea *scrollArea;
    QLabel *descriptionLabel;

    void setupUi(QWidget *q)
    {
        QWidget *setupTargetPage = new QWidget(q);

        descriptionLabel = new QLabel(setupTargetPage);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        descriptionLabel->setText(TargetSetupPage::tr("Qt Creator can set up the following targets:"));

#ifdef Q_OS_MAC
        QString hint = TargetSetupPage::tr(
            "<html><head/><body><p><span style=\" font-weight:600;\">"
            "No valid kits found.</span></p>"
            "<p>Please add a kit in <a href=\"buildandrun\"><span style=\" text-decoration: underline; color:#0000ff;\">"
            "Qt Creator &gt; Preferences &gt; Build &amp; Run</span></a>"
            " or via the maintenance tool of the SDK.</p></body></html>");
#else
        QString hint = TargetSetupPage::tr(
            "<html><head/><body><p><span style=\" font-weight:600;\">"
            "No valid kits found.</span></p>"
            "<p>Please add a kit in <a href=\"buildandrun\"><span style=\" text-decoration: underline; color:#0000ff;\">"
            "Tools &gt; Options &gt; Build &amp; Run</span></a>"
            " or via the maintenance tool of the SDK.</p></body></html>");
#endif

        QLabel *noValidProfileLabel = new QLabel(setupTargetPage);
        noValidProfileLabel->setWordWrap(true);
        noValidProfileLabel->setText(hint);
        noValidProfileLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

        centralWidget = new QWidget(setupTargetPage);
        QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        policy.setHorizontalStretch(0);
        policy.setVerticalStretch(0);
        policy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(policy);

        scrollAreaWidget = new QWidget(setupTargetPage);
        scrollArea = new QScrollArea(scrollAreaWidget);
        scrollArea->setWidgetResizable(true);

        QWidget *scrollAreaWidgetContents;
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 230, 81));
        scrollArea->setWidget(scrollAreaWidgetContents);

        QVBoxLayout *verticalLayout = new QVBoxLayout(scrollAreaWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->addWidget(scrollArea);

        QVBoxLayout *verticalLayout_2 = new QVBoxLayout(setupTargetPage);
        verticalLayout_2->addWidget(descriptionLabel);
        verticalLayout_2->addWidget(noValidProfileLabel);
        verticalLayout_2->addWidget(centralWidget);
        verticalLayout_2->addWidget(scrollAreaWidget);

        QVBoxLayout *verticalLayout_3 = new QVBoxLayout(q);
        verticalLayout_3->setContentsMargins(0, 0, 0, -1);
        verticalLayout_3->addWidget(setupTargetPage);

        QObject::connect(noValidProfileLabel, SIGNAL(linkActivated(QString)),
            q, SIGNAL(noteTextLinkActivated()));
        QObject::connect(descriptionLabel, SIGNAL(linkActivated(QString)),
            q, SIGNAL(noteTextLinkActivated()));
    }
};

} // namespace Internal

using namespace Internal;

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    QWizardPage(parent),
    m_requiredMatcher(0),
    m_preferredMatcher(0),
    m_baseLayout(0),
    m_importSearch(false),
    m_ignoreUpdates(false),
    m_firstWidget(0),
    m_ui(new TargetSetupPageUi),
    m_importWidget(new Internal::ImportWidget(this)),
    m_spacer(new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding))
{
    setObjectName(QLatin1String("TargetSetupPage"));
    setWindowTitle(tr("Set up Targets for Your Project"));
    m_ui->setupUi(this);

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHorizontalStretch(0);
    policy.setVerticalStretch(0);
    policy.setHeightForWidth(sizePolicy().hasHeightForWidth());
    setSizePolicy(policy);

    QWidget *centralWidget = new QWidget(this);
    m_ui->scrollArea->setWidget(centralWidget);
    centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->layout()->setMargin(0);

    setUseScrollArea(true);
    setImportSearch(false);

    setTitle(tr("Target Setup"));

    ProjectExplorer::KitManager *km = ProjectExplorer::KitManager::instance();
    connect(km, SIGNAL(kitAdded(ProjectExplorer::Kit*)),
            this, SLOT(handleKitAddition(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this, SLOT(handleKitRemoval(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(handleKitUpdate(ProjectExplorer::Kit*)));
    connect(m_importWidget, SIGNAL(importFrom(Utils::FileName)),
            this, SLOT(import(Utils::FileName)));
}

void TargetSetupPage::initializePage()
{
    reset();

    setupWidgets();
    setupImports();
    selectAtLeastOneTarget();
}

void TargetSetupPage::setRequiredKitMatcher(ProjectExplorer::KitMatcher *matcher)
{
    m_requiredMatcher = matcher;
}

QList<Core::Id> TargetSetupPage::selectedKits() const
{
    QList<Core::Id> result;
    QMap<Core::Id, Qt4TargetSetupWidget *>::const_iterator it, end;
    it = m_widgets.constBegin();
    end = m_widgets.constEnd();

    for ( ; it != end; ++it) {
        if (isKitSelected(it.key()))
            result << it.key();
    }
    return result;
}

void TargetSetupPage::setPreferredKitMatcher(ProjectExplorer::KitMatcher *matcher)
{
    m_preferredMatcher = matcher;
}

TargetSetupPage::~TargetSetupPage()
{
    reset();
    delete m_ui;
    delete m_preferredMatcher;
    delete m_requiredMatcher;
}

bool TargetSetupPage::isKitSelected(Core::Id id) const
{
    Qt4TargetSetupWidget *widget = m_widgets.value(id);
    return widget && widget->isKitSelected();
}

void TargetSetupPage::setKitSelected(Core::Id id, bool selected)
{
    Qt4TargetSetupWidget *widget = m_widgets.value(id);
    if (widget)
        widget->setKitSelected(selected);
}

bool TargetSetupPage::isComplete() const
{
    foreach (Qt4TargetSetupWidget *widget, m_widgets.values())
        if (widget->isKitSelected())
            return true;
    return false;
}

void TargetSetupPage::setImportSearch(bool b)
{
    m_importSearch = b;
    m_importWidget->setVisible(b);
}

void TargetSetupPage::setupWidgets()
{
    // Known profiles:
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::instance()->kits(m_requiredMatcher)) {
        cleanProfile(k); // clean up broken profiles added by some development versions of QtC
        addWidget(k);
    }

    // Setup import widget:
    m_baseLayout->addWidget(m_importWidget);
    Utils::FileName path = Utils::FileName::fromString(m_proFilePath);
    path = path.parentDir(); // base dir
    path = path.parentDir(); // parent dir
    m_importWidget->setCurrentDirectory(path);

    updateVisibility();
}

void TargetSetupPage::reset()
{
    foreach (Qt4TargetSetupWidget *widget, m_widgets.values()) {
        ProjectExplorer::Kit *p = widget->profile();
        if (!p)
            continue;
        removeProject(p, m_proFilePath);
        delete widget;
    }

    m_widgets.clear();
    m_firstWidget = 0;
}

ProjectExplorer::Kit *TargetSetupPage::createTemporaryProfile(QtSupport::BaseQtVersion *version,
                                                              bool temporaryVersion,
                                                              const Utils::FileName &parsedSpec)
{
    ProjectExplorer::Kit *k = new ProjectExplorer::Kit;
    QtSupport::QtKitInformation::setQtVersion(k, version);
    ProjectExplorer::ToolChainKitInformation::setToolChain(k, version->preferredToolChain(parsedSpec));
    QmakeKitInformation::setMkspec(k, parsedSpec);

    k->setDisplayName(version->displayName());
    k->setValue(PROFILE_IS_TEMPORARY, true);
    k->setValue(TEMPORARY_OF_PROJECTS, QStringList() << m_proFilePath);
    if (temporaryVersion)
        k->setValue(QT_IS_TEMPORARY, version->uniqueId());

    m_ignoreUpdates = true;
    ProjectExplorer::KitManager::instance()->registerKit(k);
    m_ignoreUpdates = false;

    return k;
}

void TargetSetupPage::cleanProfile(ProjectExplorer::Kit *k)
{
    m_ignoreUpdates = true;
    k->removeKey(PROFILE_IS_TEMPORARY);
    k->removeKey(QT_IS_TEMPORARY);
    k->removeKey(TEMPORARY_OF_PROJECTS);
    m_ignoreUpdates = false;
}

void TargetSetupPage::makeQtPersistent(ProjectExplorer::Kit *k)
{
    m_ignoreUpdates = true;
    k->removeKey(QT_IS_TEMPORARY);
    m_ignoreUpdates = false;
}

void TargetSetupPage::addProject(ProjectExplorer::Kit *k, const QString &path)
{
    if (!k->hasValue(PROFILE_IS_TEMPORARY))
        return;

    QStringList profiles = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    profiles.append(path);
    m_ignoreUpdates = true;
    k->setValue(PROFILE_IS_TEMPORARY, profiles);
    m_ignoreUpdates = false;
}

void TargetSetupPage::removeProject(ProjectExplorer::Kit *k, const QString &path)
{
    if (!k->hasValue(PROFILE_IS_TEMPORARY) || path.isEmpty())
        return;

    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    if (projects.contains(path)) {
        projects.removeOne(path);
        m_ignoreUpdates = true;
        k->setValue(TEMPORARY_OF_PROJECTS, projects);
        if (projects.isEmpty())
            ProjectExplorer::KitManager::instance()->deregisterKit(k);
        m_ignoreUpdates = false;
    }
}

void TargetSetupPage::setProFilePath(const QString &path)
{
    m_proFilePath = path;
    if (!m_proFilePath.isEmpty()) {
        m_ui->descriptionLabel->setText(tr("Qt Creator can set up the following targets for project <b>%1</b>:",
                                           "%1: Project name").arg(QFileInfo(m_proFilePath).baseName()));
    }

    if (m_widgets.isEmpty())
        return;

    reset();
    setupWidgets();
}

void TargetSetupPage::setNoteText(const QString &text)
{
    m_ui->descriptionLabel->setText(text);
}

void TargetSetupPage::import(const Utils::FileName &path)
{
    import(path, false);
}

void TargetSetupPage::import(const Utils::FileName &path, const bool silent)
{
    QFileInfo fi = path.toFileInfo();
    if (!fi.exists() && !fi.isDir())
        return;

    QStringList makefiles = QDir(path.toString()).entryList(QStringList(QLatin1String("Makefile*")));

    QtSupport::BaseQtVersion *version = 0;
    bool temporaryVersion = false;
    ProjectExplorer::Kit *kit = 0;

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    ProjectExplorer::KitManager *km = ProjectExplorer::KitManager::instance();
    bool found = false;

    foreach (const QString &file, makefiles) {
        // find interesting makefiles
        QString makefile = path.toString() + QLatin1Char('/') + file;
        Utils::FileName qmakeBinary = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        if (qmakeBinary.isEmpty())
            continue;
        if (QtSupport::QtVersionManager::makefileIsFor(makefile, m_proFilePath) != QtSupport::QtVersionManager::SameProject)
            continue;

        // Find version:
        version = vm->qtVersionForQMakeBinary(qmakeBinary);
        if (!version) {
            version = QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmakeBinary);
            if (!version)
                continue;

            vm->addVersion(version);
            temporaryVersion = true;
        }

        // find qmake arguments and mkspec
        QPair<QtSupport::BaseQtVersion::QmakeBuildConfigs, QString> makefileBuildConfig =
                QtSupport::QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());

        QString additionalArguments = makefileBuildConfig.second;
        Utils::FileName parsedSpec =
                Qt4BuildConfiguration::extractSpecFromArguments(&additionalArguments, path.toString(), version);
        Utils::FileName versionSpec = version->mkspec();
        if (parsedSpec.isEmpty() || parsedSpec == Utils::FileName::fromString(QLatin1String("default")))
            parsedSpec = versionSpec;

        QString specArgument;
        // Compare mkspecs and add to additional arguments
        if (parsedSpec != versionSpec)
            specArgument = QLatin1String("-spec ") + Utils::QtcProcess::quoteArg(parsedSpec.toUserOutput());
        Utils::QtcProcess::addArgs(&specArgument, additionalArguments);

        // Find profile:
        foreach (ProjectExplorer::Kit *k, km->kits()) {
            QtSupport::BaseQtVersion *profileVersion = QtSupport::QtKitInformation::qtVersion(k);
            Utils::FileName profileSpec = QmakeKitInformation::mkspec(k);
            ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
            if (profileSpec.isEmpty() && profileVersion)
                profileSpec = profileVersion->mkspecFor(tc);

            if (profileVersion == version
                    && profileSpec == parsedSpec)
                kit = k;
        }
        if (!kit)
            kit = createTemporaryProfile(version, temporaryVersion, parsedSpec);
        else
            addProject(kit, m_proFilePath);

        // Create widget:
        Qt4TargetSetupWidget *widget = m_widgets.value(kit->id(), 0);
        if (!widget)
            addWidget(kit);
        widget = m_widgets.value(kit->id(), 0);
        if (!widget)
            continue;

        // create info:
        BuildConfigurationInfo info = BuildConfigurationInfo(makefileBuildConfig.first,
                                                             specArgument,
                                                             path.toString(),
                                                             true,
                                                             file);

        widget->addBuildConfigurationInfo(info, true);
        widget->setKitSelected(true);
        found = true;
    }

    updateVisibility();

    if (!found && !silent)
        QMessageBox::critical(this,
                              tr("No Build Found"),
                              tr("No build found in %1 matching project %2.").arg(path.toUserOutput()).arg(m_proFilePath));
}

void TargetSetupPage::handleQtUpdate(const QList<int> &add, const QList<int> &rm, const QList<int> &mod)
{
    Q_UNUSED(add);
    // Update Profile to no longer claim a Qt version is temporary once it is modified/removed.
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::instance()->kits()) {
        if (!k->hasValue(QT_IS_TEMPORARY))
            continue;
        int qtVersion = k->value(QT_IS_TEMPORARY, -1).toInt();
        if (rm.contains(qtVersion) || mod.contains(qtVersion))
            makeQtPersistent(k);
    }
}

void TargetSetupPage::setupImports()
{
    if (!m_importSearch || m_proFilePath.isEmpty())
        return;

    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();
    import(Utils::FileName::fromString(sourceDir), true);

    QList<ProjectExplorer::Kit *> kitList = ProjectExplorer::KitManager::instance()->kits();
    foreach (ProjectExplorer::Kit *p, kitList) {
        QFileInfo fi(Qt4Project::shadowBuildDirectory(m_proFilePath, p, QString()));
        const QString baseDir = fi.absolutePath();
        const QString prefix = fi.baseName();

        foreach (const QString &dir, QDir(baseDir).entryList()) {
            if (dir.startsWith(prefix))
                import(Utils::FileName::fromString(baseDir + QLatin1Char('/') + dir), true);
        }
    }
}

void TargetSetupPage::handleKitAddition(ProjectExplorer::Kit *k)
{
    if (m_ignoreUpdates)
        return;

    Q_ASSERT(!m_widgets.contains(k->id()));
    addWidget(k);
    updateVisibility();
}

void TargetSetupPage::handleKitRemoval(ProjectExplorer::Kit *k)
{
    if (m_ignoreUpdates)
        return;

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QtSupport::BaseQtVersion *version = vm->version(k->value(QT_IS_TEMPORARY, -1).toInt());
    if (version)
        vm->removeVersion(version);

    removeWidget(k);
    updateVisibility();
}

void TargetSetupPage::handleKitUpdate(ProjectExplorer::Kit *k)
{
    if (m_ignoreUpdates)
        return;

    cleanProfile(k);
    Qt4TargetSetupWidget *widget = m_widgets.value(k->id());

    bool acceptable = true;
    if (m_requiredMatcher && !m_requiredMatcher->matches(k))
        acceptable = false;

    if (widget && !acceptable)
        removeWidget(k);
    else if (!widget && acceptable)
        addWidget(k);

    updateVisibility();
}

void TargetSetupPage::selectAtLeastOneTarget()
{
    bool atLeastOneTargetSelected = false;
    foreach (Qt4TargetSetupWidget *w, m_widgets.values()) {
        if (w->isKitSelected()) {
            atLeastOneTargetSelected = true;
            break;
        }
    }

    if (!atLeastOneTargetSelected) {
        Qt4TargetSetupWidget *widget = m_firstWidget;
        ProjectExplorer::Kit *defaultKit = ProjectExplorer::KitManager::instance()->defaultKit();
        if (defaultKit)
            widget = m_widgets.value(defaultKit->id(), m_firstWidget);
        if (widget)
            widget->setKitSelected(true);
        m_firstWidget = 0;
    }
    emit completeChanged(); // Is this necessary?
}

void TargetSetupPage::updateVisibility()
{
    // Always show the widgets, the import widget always makes sense to show.
    m_ui->scrollAreaWidget->setVisible(m_baseLayout == m_ui->scrollArea->widget()->layout());
    m_ui->centralWidget->setVisible(m_baseLayout == m_ui->centralWidget->layout());

    emit completeChanged();
}

void TargetSetupPage::removeWidget(ProjectExplorer::Kit *k)
{
    Qt4TargetSetupWidget *widget = m_widgets.value(k->id());
    if (!widget)
        return;
    if (widget == m_firstWidget)
        m_firstWidget = 0;
    widget->deleteLater();
    m_widgets.remove(k->id());
}

Qt4TargetSetupWidget *TargetSetupPage::addWidget(ProjectExplorer::Kit *k)
{
    if (m_requiredMatcher && !m_requiredMatcher->matches(k))
        return 0;

    QList<BuildConfigurationInfo> infoList = Qt4BuildConfigurationFactory::availableBuildConfigurations(k, m_proFilePath);
    Qt4TargetSetupWidget *widget = infoList.isEmpty() ? 0 : new Qt4TargetSetupWidget(k, m_proFilePath, infoList);
    if (!widget)
        return 0;

    m_baseLayout->removeWidget(m_importWidget);
    m_baseLayout->removeItem(m_spacer);

    widget->setKitSelected(m_preferredMatcher && m_preferredMatcher->matches(k));
    m_widgets.insert(k->id(), widget);
    m_baseLayout->addWidget(widget);

    m_baseLayout->addWidget(m_importWidget);
    m_baseLayout->addItem(m_spacer);

    connect(widget, SIGNAL(selectedToggled()),
            this, SIGNAL(completeChanged()));

    if (!m_firstWidget)
        m_firstWidget = widget;

    return widget;
}

struct ProfileBuildInfo
{
    ProfileBuildInfo(ProjectExplorer::Kit *p, const QList<BuildConfigurationInfo> &il) :
        profile(p), infoList(il)
    { }

    ProjectExplorer::Kit *profile;
    QList<BuildConfigurationInfo> infoList;
};

bool TargetSetupPage::setupProject(Qt4ProjectManager::Qt4Project *project)
{
    QList<ProfileBuildInfo> toRegister;
    foreach (Qt4TargetSetupWidget *widget, m_widgets.values()) {
        if (!widget->isKitSelected())
            continue;

        ProjectExplorer::Kit *p = widget->profile();
        cleanProfile(p);
        toRegister.append(ProfileBuildInfo(p, widget->selectedBuildConfigurationInfoList()));
        widget->clearProfile();
    }
    reset();

    // only register targets after we are done cleaning up
    foreach (const ProfileBuildInfo &data, toRegister)
        project->addTarget(project->createTarget(data.profile, data.infoList));

    // Select active target
    // a) Simulator target
    // b) Desktop target
    // c) the first target
    ProjectExplorer::Target *activeTarget = 0;
    QList<ProjectExplorer::Target *> targets = project->targets();
    foreach (ProjectExplorer::Target *t, targets) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(t->kit());
        if (version && version->type() == QLatin1String(QtSupport::Constants::SIMULATORQT))
            activeTarget = t;
        else if (!activeTarget && version && version->type() == QLatin1String(QtSupport::Constants::DESKTOPQT))
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
    m_baseLayout = b ? m_ui->scrollArea->widget()->layout() : m_ui->centralWidget->layout();
    m_ui->scrollAreaWidget->setVisible(b);
    m_ui->centralWidget->setVisible(!b);
}

} // namespace Qt4ProjectManager
