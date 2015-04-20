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

#include "targetsetuppage.h"
#include "buildconfiguration.h"
#include "buildinfo.h"
#include "kit.h"
#include "kitmanager.h"
#include "importwidget.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "target.h"
#include "targetsetupwidget.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/ipotentialkit.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/wizard.h>
#include <utils/algorithm.h>

#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QCheckBox>

namespace ProjectExplorer {
namespace Internal {

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
    QCheckBox *allKitsCheckBox;

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

        allKitsCheckBox = new QCheckBox(setupTargetPage);
        allKitsCheckBox->setTristate(true);
        allKitsCheckBox->setText(TargetSetupPage::tr("Select all kits"));

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
        verticalLayout_2->addWidget(allKitsCheckBox);
        verticalLayout_2->addWidget(centralWidget);
        verticalLayout_2->addWidget(scrollAreaWidget);

        QVBoxLayout *verticalLayout_3 = new QVBoxLayout(q);
        verticalLayout_3->setContentsMargins(0, 0, 0, -1);
        verticalLayout_3->addWidget(setupTargetPage);

        QObject::connect(optionHintLabel, SIGNAL(linkActivated(QString)),
                         q, SLOT(openOptions()));

        QObject::connect(allKitsCheckBox, SIGNAL(clicked()),
                         q, SLOT(changeAllKitsSelections()));
    }
};

} // namespace Internal

using namespace Internal;

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    Utils::WizardPage(parent),
    m_importer(0),
    m_baseLayout(0),
    m_firstWidget(0),
    m_ui(new TargetSetupPageUi),
    m_importWidget(new ImportWidget(this)),
    m_spacer(new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding)),
    m_forceOptionHint(false)
{
    m_importWidget->setVisible(false);

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

    setTitle(tr("Kit Selection"));

    QList<IPotentialKit *> potentialKits =
            ExtensionSystem::PluginManager::instance()->getObjects<IPotentialKit>();
    foreach (IPotentialKit *pk, potentialKits)
        if (pk->isEnabled())
            m_potentialWidgets.append(pk->createWidget(this));

    setUseScrollArea(true);

    QObject *km = KitManager::instance();
    // do note that those slots are triggered once *per* targetsetuppage
    // thus the same slot can be triggered multiple times on different instances!
    connect(km, SIGNAL(kitAdded(ProjectExplorer::Kit*)),
            this, SLOT(handleKitAddition(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this, SLOT(handleKitRemoval(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(handleKitUpdate(ProjectExplorer::Kit*)));
    connect(m_importWidget, SIGNAL(importFrom(Utils::FileName)),
            this, SLOT(import(Utils::FileName)));

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Kits"));
}

void TargetSetupPage::initializePage()
{
    reset();

    setupWidgets();
    setupImports();
    selectAtLeastOneKit();
}

void TargetSetupPage::setRequiredKitMatcher(const KitMatcher &matcher)
{
    m_requiredMatcher = matcher;
}

QList<Core::Id> TargetSetupPage::selectedKits() const
{
    QList<Core::Id> result;
    QMap<Core::Id, TargetSetupWidget *>::const_iterator it, end;
    it = m_widgets.constBegin();
    end = m_widgets.constEnd();

    for ( ; it != end; ++it) {
        if (isKitSelected(it.key()))
            result << it.key();
    }
    return result;
}

void TargetSetupPage::setPreferredKitMatcher(const KitMatcher &matcher)
{
    m_preferredMatcher = matcher;
}

TargetSetupPage::~TargetSetupPage()
{
    reset();
    delete m_ui;
    delete m_importer;
}

bool TargetSetupPage::isKitSelected(Core::Id id) const
{
    TargetSetupWidget *widget = m_widgets.value(id);
    return widget && widget->isKitSelected();
}

void TargetSetupPage::setKitSelected(Core::Id id, bool selected)
{
    TargetSetupWidget *widget = m_widgets.value(id);
    if (widget) {
        widget->setKitSelected(selected);
        kitSelectionChanged();
    }
}

bool TargetSetupPage::isComplete() const
{
    return Utils::anyOf(m_widgets, &TargetSetupWidget::isKitSelected);
}

void TargetSetupPage::setupWidgets()
{
    // Known profiles:
    QList<Kit *> kitList;
    if (m_requiredMatcher.isValid())
        kitList = KitManager::matchingKits(m_requiredMatcher);
    else
        kitList = KitManager::kits();

    kitList = KitManager::sortKits(kitList);

    foreach (Kit *k, kitList)
        addWidget(k);

    // Setup import widget:
    Utils::FileName path = Utils::FileName::fromString(m_projectPath);
    path = path.parentDir(); // base dir
    path = path.parentDir(); // parent dir
    m_importWidget->setCurrentDirectory(path);

    updateVisibility();
}

void TargetSetupPage::reset()
{
    foreach (TargetSetupWidget *widget, m_widgets) {
        Kit *k = widget->kit();
        if (!k)
            continue;
        if (m_importer)
            m_importer->removeProject(k, m_projectPath);
        delete widget;
    }

    m_widgets.clear();
    m_firstWidget = 0;

    m_ui->allKitsCheckBox->setChecked(false);
}

void TargetSetupPage::setProjectPath(const QString &path)
{
    m_projectPath = path;
    if (!m_projectPath.isEmpty())
        m_ui->headerLabel->setText(tr("Qt Creator can use the following kits for project <b>%1</b>:",
                                      "%1: Project name").arg(QFileInfo(m_projectPath).baseName()));
    m_ui->headerLabel->setVisible(!m_projectPath.isEmpty());

    if (m_widgets.isEmpty())
        return;

    reset();
    setupWidgets();
}

void TargetSetupPage::setProjectImporter(ProjectImporter *importer)
{
    if (importer == m_importer)
        return;
    if (m_importer)
        delete m_importer;
    m_importer = importer;

    m_importWidget->setVisible(m_importer);

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

void TargetSetupPage::setupImports()
{
    if (!m_importer || m_projectPath.isEmpty())
        return;

    QStringList toImport = m_importer->importCandidates(Utils::FileName::fromString(m_projectPath));
    foreach (const QString &path, toImport)
        import(Utils::FileName::fromString(path), true);
}

void TargetSetupPage::handleKitAddition(Kit *k)
{
    if (isUpdating())
        return;

    Q_ASSERT(!m_widgets.contains(k->id()));
    addWidget(k);
    updateVisibility();
}

void TargetSetupPage::handleKitRemoval(Kit *k)
{
    if (m_importer)
        m_importer->cleanupKit(k);

    if (isUpdating())
        return;

    removeWidget(k);
    updateVisibility();
}

void TargetSetupPage::handleKitUpdate(Kit *k)
{
    if (isUpdating())
        return;

    if (m_importer)
        m_importer->makePermanent(k);

    TargetSetupWidget *widget = m_widgets.value(k->id());

    bool acceptable = !m_requiredMatcher.isValid() || m_requiredMatcher.matches(k);

    if (widget && !acceptable)
        removeWidget(k);
    else if (!widget && acceptable)
        addWidget(k);

    updateVisibility();
}

void TargetSetupPage::selectAtLeastOneKit()
{
    bool atLeastOneKitSelected = false;
    foreach (TargetSetupWidget *w, m_widgets) {
        if (w->isKitSelected()) {
            atLeastOneKitSelected = true;
            break;
        }
    }

    if (!atLeastOneKitSelected) {
        TargetSetupWidget *widget = m_firstWidget;
        Kit *defaultKit = KitManager::defaultKit();
        if (defaultKit)
            widget = m_widgets.value(defaultKit->id(), m_firstWidget);
        if (widget) {
            widget->setKitSelected(true);
            kitSelectionChanged();
        }
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
    m_ui->allKitsCheckBox->setVisible(hasKits);

    emit completeChanged();
}

void TargetSetupPage::openOptions()
{
    Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, this);
}

void TargetSetupPage::import(const Utils::FileName &path)
{
    import(path, false);
}

void TargetSetupPage::kitSelectionChanged()
{
    int selected = 0;
    int deselected = 0;
    foreach (TargetSetupWidget *widget, m_widgets) {
        if (widget->isKitSelected())
            ++selected;
        else
            ++deselected;
    }
    if (selected > 0 && deselected > 0)
        m_ui->allKitsCheckBox->setCheckState(Qt::PartiallyChecked);
    else if (selected > 0 && deselected == 0)
        m_ui->allKitsCheckBox->setCheckState(Qt::Checked);
    else
        m_ui->allKitsCheckBox->setCheckState(Qt::Unchecked);
}

void TargetSetupPage::changeAllKitsSelections()
{
    if (m_ui->allKitsCheckBox->checkState() == Qt::PartiallyChecked)
        m_ui->allKitsCheckBox->setCheckState(Qt::Checked);
    bool checked = m_ui->allKitsCheckBox->isChecked();
    foreach (TargetSetupWidget *widget, m_widgets)
        widget->setKitSelected(checked);
    emit completeChanged();
}

bool TargetSetupPage::isUpdating() const
{
    if (m_importer)
        return m_importer->isUpdating();
    return false;
}

void TargetSetupPage::import(const Utils::FileName &path, bool silent)
{
    if (!m_importer)
        return;

    QList<BuildInfo *> toImport = m_importer->import(path, silent);
    foreach (BuildInfo *info, toImport) {
        TargetSetupWidget *widget = m_widgets.value(info->kitId, 0);
        if (!widget) {
            Kit *k = KitManager::find(info->kitId);
            Q_ASSERT(k);
            addWidget(k);
        }
        widget = m_widgets.value(info->kitId, 0);
        if (!widget) {
            delete info;
            continue;
        }

        widget->addBuildInfo(info, true);
        widget->setKitSelected(true);
        widget->expandWidget();
        kitSelectionChanged();
    }
    emit completeChanged();
}

void TargetSetupPage::removeWidget(Kit *k)
{
    TargetSetupWidget *widget = m_widgets.value(k->id());
    if (!widget)
        return;
    if (widget == m_firstWidget)
        m_firstWidget = 0;
    widget->deleteLater();
    m_widgets.remove(k->id());
    kitSelectionChanged();
}

TargetSetupWidget *TargetSetupPage::addWidget(Kit *k)
{
    if (!k || (m_requiredMatcher.isValid() && !m_requiredMatcher.matches(k)))
        return 0;

    IBuildConfigurationFactory *factory
            = IBuildConfigurationFactory::find(k, m_projectPath);
    if (!factory)
        return 0;

    QList<BuildInfo *> infoList = factory->availableSetups(k, m_projectPath);
    TargetSetupWidget *widget = infoList.isEmpty() ? 0 : new TargetSetupWidget(k, m_projectPath, infoList);
    if (!widget)
        return 0;

    m_baseLayout->removeWidget(m_importWidget);
    foreach (QWidget *widget, m_potentialWidgets)
        m_baseLayout->removeWidget(widget);
    m_baseLayout->removeItem(m_spacer);

    widget->setKitSelected(m_preferredMatcher.isValid() && m_preferredMatcher.matches(k));
    m_widgets.insert(k->id(), widget);
    connect(widget, SIGNAL(selectedToggled()),
            this, SLOT(kitSelectionChanged()));
    m_baseLayout->addWidget(widget);

    m_baseLayout->addWidget(m_importWidget);
    foreach (QWidget *widget, m_potentialWidgets)
        m_baseLayout->addWidget(widget);
    m_baseLayout->addItem(m_spacer);

    connect(widget, SIGNAL(selectedToggled()),
            this, SIGNAL(completeChanged()));

    if (!m_firstWidget)
        m_firstWidget = widget;

    kitSelectionChanged();

    return widget;
}

bool TargetSetupPage::setupProject(Project *project)
{
    QList<const BuildInfo *> toSetUp; // Pointers are managed by the widgets!
    foreach (TargetSetupWidget *widget, m_widgets) {
        if (!widget->isKitSelected())
            continue;

        Kit *k = widget->kit();
        if (m_importer)
            m_importer->makePermanent(k);
        toSetUp << widget->selectedBuildInfoList();
        widget->clearKit();
    }

    reset();
    project->setup(toSetUp);

    toSetUp.clear();

    Target *activeTarget = 0;
    if (m_importer)
        activeTarget = m_importer->preferredTarget(project->targets());
    if (activeTarget)
        project->setActiveTarget(activeTarget);

    return true;
}

void TargetSetupPage::setUseScrollArea(bool b)
{
    QLayout *oldBaseLayout = m_baseLayout;
    m_baseLayout = b ? m_ui->scrollArea->widget()->layout() : m_ui->centralWidget->layout();
    if (oldBaseLayout == m_baseLayout)
        return;
    m_ui->scrollAreaWidget->setVisible(b);
    m_ui->centralWidget->setVisible(!b);

    if (oldBaseLayout) {
        oldBaseLayout->removeWidget(m_importWidget);
        foreach (QWidget *widget, m_potentialWidgets)
            oldBaseLayout->removeWidget(widget);
        oldBaseLayout->removeItem(m_spacer);
    }

    m_baseLayout->addWidget(m_importWidget);
    foreach (QWidget *widget, m_potentialWidgets)
        m_baseLayout->addWidget(widget);
    m_baseLayout->addItem(m_spacer);
}

} // namespace ProjectExplorer
