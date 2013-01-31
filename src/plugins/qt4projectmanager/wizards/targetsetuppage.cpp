/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
static const Core::Id KIT_IS_TEMPORARY("Qt4PM.TempKit");
static const Core::Id KIT_TEMPORARY_NAME("Qt4PM.TempName");
static const Core::Id KIT_FINAL_NAME("Qt4PM.FinalName");
static const Core::Id TEMPORARY_OF_PROJECTS("Qt4PM.TempProject");

class TargetSetupPageUi
{
public:
    QWidget *centralWidget;
    QWidget *scrollAreaWidget;
    QScrollArea *scrollArea;
    QLabel *headerLabel;
    QLabel *descriptionLabel;
    QLabel *noValidKitLabel;
    QLabel *optionHintLabel;

    void setupUi(QWidget *q)
    {
        QWidget *setupTargetPage = new QWidget(q);
        descriptionLabel = new QLabel(setupTargetPage);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setVisible(false);

        headerLabel = new QLabel(setupTargetPage);
        headerLabel->setWordWrap(true);
        headerLabel->setVisible(false);

        noValidKitLabel = new QLabel(setupTargetPage);
        noValidKitLabel->setWordWrap(true);
        noValidKitLabel->setText(TargetSetupPage::tr("<span style=\" font-weight:600;\">No valid kits found.</span>"));


        optionHintLabel = new QLabel(setupTargetPage);
        optionHintLabel->setWordWrap(true);
        optionHintLabel->setText(TargetSetupPage::tr(
                                     "Please add a kit in the <a href=\"buildandrun\">options</a> "
                                     "or via the maintenance tool of the SDK."));
        optionHintLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        optionHintLabel->setVisible(false);

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
        verticalLayout_2->addWidget(headerLabel);
        verticalLayout_2->addWidget(noValidKitLabel);
        verticalLayout_2->addWidget(descriptionLabel);
        verticalLayout_2->addWidget(optionHintLabel);
        verticalLayout_2->addWidget(centralWidget);
        verticalLayout_2->addWidget(scrollAreaWidget);

        QVBoxLayout *verticalLayout_3 = new QVBoxLayout(q);
        verticalLayout_3->setContentsMargins(0, 0, 0, -1);
        verticalLayout_3->addWidget(setupTargetPage);

        QObject::connect(optionHintLabel, SIGNAL(linkActivated(QString)),
                         q, SLOT(openOptions()));
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
    m_spacer(new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding)),
    m_forceOptionHint(false)
{
    setObjectName(QLatin1String("TargetSetupPage"));
    setWindowTitle(tr("Select Kits for Your Project"));
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

    setTitle(tr("Kit Selection"));

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
    selectAtLeastOneKit();
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
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::instance()->kits(m_requiredMatcher))
        addWidget(k);

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
        ProjectExplorer::Kit *k = widget->kit();
        if (!k)
            continue;
        removeProject(k, m_proFilePath);
        delete widget;
    }

    m_widgets.clear();
    m_firstWidget = 0;
}

ProjectExplorer::Kit *TargetSetupPage::createTemporaryKit(QtSupport::BaseQtVersion *version,
                                                          bool temporaryVersion,
                                                          const Utils::FileName &parsedSpec)
{
    ProjectExplorer::Kit *k = new ProjectExplorer::Kit;
    QtSupport::QtKitInformation::setQtVersion(k, version);
    ProjectExplorer::ToolChainKitInformation::setToolChain(k, version->preferredToolChain(parsedSpec));
    QmakeKitInformation::setMkspec(k, parsedSpec);

    k->setDisplayName(tr("%1 - temporary").arg(version->displayName()));
    k->setValue(KIT_TEMPORARY_NAME, k->displayName());
    k->setValue(KIT_FINAL_NAME, version->displayName());
    k->setValue(KIT_IS_TEMPORARY, true);
    if (temporaryVersion)
        k->setValue(QT_IS_TEMPORARY, version->uniqueId());

    m_ignoreUpdates = true;
    ProjectExplorer::KitManager::instance()->registerKit(k);
    m_ignoreUpdates = false;

    return k;
}

void TargetSetupPage::cleanKit(ProjectExplorer::Kit *k)
{
    m_ignoreUpdates = true;
    k->removeKey(KIT_IS_TEMPORARY);
    k->removeKey(QT_IS_TEMPORARY);
    k->removeKey(TEMPORARY_OF_PROJECTS);
    const QString tempName = k->value(KIT_TEMPORARY_NAME).toString();
    if (!tempName.isNull() && k->displayName() == tempName)
        k->setDisplayName(k->value(KIT_FINAL_NAME).toString());
    k->removeKey(KIT_TEMPORARY_NAME);
    k->removeKey(KIT_FINAL_NAME);
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
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    if (!projects.contains(path)) {
        projects.append(path);
        m_ignoreUpdates = true;
        k->setValue(TEMPORARY_OF_PROJECTS, projects);
        m_ignoreUpdates = false;
    }
}

void TargetSetupPage::removeProject(ProjectExplorer::Kit *k, const QString &path)
{
    if (!k->hasValue(KIT_IS_TEMPORARY) || path.isEmpty())
        return;

    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    if (projects.contains(path)) {
        projects.removeOne(path);
        m_ignoreUpdates = true;
        if (projects.isEmpty())
            ProjectExplorer::KitManager::instance()->deregisterKit(k);
        else
            k->setValue(TEMPORARY_OF_PROJECTS, projects);
        m_ignoreUpdates = false;
    }
}

void TargetSetupPage::setProFilePath(const QString &path)
{
    m_proFilePath = path;
    if (!m_proFilePath.isEmpty())
        m_ui->headerLabel->setText(tr("Qt Creator can use the following kits for project <b>%1</b>:",
                                      "%1: Project name").arg(QFileInfo(m_proFilePath).baseName()));
    m_ui->headerLabel->setVisible(!m_proFilePath.isEmpty());

    if (m_widgets.isEmpty())
        return;

    reset();
    setupWidgets();
}

void TargetSetupPage::setNoteText(const QString &text)
{
    m_ui->descriptionLabel->setText(text);
    m_ui->descriptionLabel->setVisible(!text.isEmpty());
}

void TargetSetupPage::showOptionsHint(bool show)
{
    m_forceOptionHint = show;
    updateVisibility();
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

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    ProjectExplorer::KitManager *km = ProjectExplorer::KitManager::instance();
    bool found = false;

    foreach (const QString &file, makefiles) {
        // find interesting makefiles
        QString makefile = path.toString() + QLatin1Char('/') + file;
        Utils::FileName qmakeBinary = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        QFileInfo fi = qmakeBinary.toFileInfo();
        Utils::FileName canonicalQmakeBinary = Utils::FileName::fromString(fi.canonicalFilePath());
        if (canonicalQmakeBinary.isEmpty())
            continue;
        if (QtSupport::QtVersionManager::makefileIsFor(makefile, m_proFilePath) != QtSupport::QtVersionManager::SameProject)
            continue;

        // Find version:
        foreach (QtSupport::BaseQtVersion *v, vm->versions()) {
            QFileInfo vfi = v->qmakeCommand().toFileInfo();
            Utils::FileName current = Utils::FileName::fromString(vfi.canonicalFilePath());
            if (current == canonicalQmakeBinary) {
                version = v;
                break;
            }
        }

        // Create a new version if not found:
        if (!version) {
            // Do not use the canonical path here...
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

        // Find profiles (can be more than one, e.g. (Linux-)Desktop and embedded linux):
        QList<ProjectExplorer::Kit *> kitList;
        foreach (ProjectExplorer::Kit *k, km->kits()) {
            QtSupport::BaseQtVersion *profileVersion = QtSupport::QtKitInformation::qtVersion(k);
            Utils::FileName profileSpec = QmakeKitInformation::mkspec(k);
            ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
            if (profileSpec.isEmpty() && profileVersion)
                profileSpec = profileVersion->mkspecFor(tc);

            if (profileVersion == version
                    && profileSpec == parsedSpec)
                kitList.append(k);
        }
        if (kitList.isEmpty())
            kitList.append(createTemporaryKit(version, temporaryVersion, parsedSpec));

        foreach (ProjectExplorer::Kit *k, kitList) {
            addProject(k, m_proFilePath);

            // Create widget:
            Qt4TargetSetupWidget *widget = m_widgets.value(k->id(), 0);
            if (!widget)
                addWidget(k);
            widget = m_widgets.value(k->id(), 0);
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
    // Update kit to no longer claim a Qt version is temporary once it is modified/removed.
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

    QFileInfo pfi(m_proFilePath);
    const QString prefix = pfi.baseName();
    QStringList toImport;
    toImport << pfi.absolutePath();

    QList<ProjectExplorer::Kit *> kitList = ProjectExplorer::KitManager::instance()->kits();
    foreach (ProjectExplorer::Kit *k, kitList) {
        QFileInfo fi(Qt4Project::shadowBuildDirectory(m_proFilePath, k, QString()));
        const QString baseDir = fi.absolutePath();

        foreach (const QString &dir, QDir(baseDir).entryList()) {
            const QString path = baseDir + QLatin1Char('/') + dir;
            if (dir.startsWith(prefix) && !toImport.contains(path))
                toImport << path;

        }
    }
    foreach (const QString &path, toImport)
        import(Utils::FileName::fromString(path), true);
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
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QtSupport::BaseQtVersion *version = vm->version(k->value(QT_IS_TEMPORARY, -1).toInt());
    if (version)
        vm->removeVersion(version);

    if (m_ignoreUpdates)
        return;

    removeWidget(k);
    updateVisibility();
}

void TargetSetupPage::handleKitUpdate(ProjectExplorer::Kit *k)
{
    if (m_ignoreUpdates)
        return;

    cleanKit(k);
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

void TargetSetupPage::selectAtLeastOneKit()
{
    bool atLeastOneKitSelected = false;
    foreach (Qt4TargetSetupWidget *w, m_widgets.values()) {
        if (w->isKitSelected()) {
            atLeastOneKitSelected = true;
            break;
        }
    }

    if (!atLeastOneKitSelected) {
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

    bool hasKits = !m_widgets.isEmpty();
    m_ui->noValidKitLabel->setVisible(!hasKits);
    m_ui->optionHintLabel->setVisible(m_forceOptionHint || !hasKits);

    emit completeChanged();
}

void TargetSetupPage::openOptions()
{
    Core::ICore::instance()->showOptionsDialog(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY,
                                               ProjectExplorer::Constants::KITS_SETTINGS_PAGE_ID,
                                               this);
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

class KitBuildInfo
{
public:
    KitBuildInfo(ProjectExplorer::Kit *k, const QList<BuildConfigurationInfo> &il) :
        kit(k), infoList(il)
    { }

    ProjectExplorer::Kit *kit;
    QList<BuildConfigurationInfo> infoList;
};

bool TargetSetupPage::setupProject(Qt4ProjectManager::Qt4Project *project)
{
    QList<KitBuildInfo> toRegister;
    foreach (Qt4TargetSetupWidget *widget, m_widgets.values()) {
        if (!widget->isKitSelected())
            continue;

        ProjectExplorer::Kit *k = widget->kit();
        cleanKit(k);
        toRegister.append(KitBuildInfo(k, widget->selectedBuildConfigurationInfoList()));
        widget->clearKit();
    }
    reset();

    // only register kits after we are done cleaning up
    foreach (const KitBuildInfo &data, toRegister)
        project->addTarget(project->createTarget(data.kit, data.infoList));

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
