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

#include "targetsetuppage.h"
#include "buildconfiguration.h"
#include "buildinfo.h"
#include "kit.h"
#include "kitmanager.h"
#include "importwidget.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "session.h"
#include "target.h"
#include "targetsetupwidget.h"

#include <coreplugin/icore.h>

#include <projectexplorer/ipotentialkit.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/wizard.h>
#include <utils/algorithm.h>
#include <utils/fancylineedit.h>

#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QCheckBox>

namespace ProjectExplorer {

static QList<IPotentialKit *> g_potentialKits;

IPotentialKit::IPotentialKit()
{
    g_potentialKits.append(this);
}

IPotentialKit::~IPotentialKit()
{
    g_potentialKits.removeOne(this);
}

namespace Internal {
static Utils::FileName importDirectory(const QString &projectPath)
{
    // Setup import widget:
    auto path = Utils::FileName::fromString(projectPath);
    path = path.parentDir(); // base dir
    path = path.parentDir(); // parent dir

    return path;
}

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
    Utils::FancyLineEdit *kitFilterLineEdit;

    void setupUi(TargetSetupPage *q)
    {
        auto setupTargetPage = new QWidget(q);
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

        kitFilterLineEdit = new Utils::FancyLineEdit(setupTargetPage);
        kitFilterLineEdit->setFiltering(true);
        kitFilterLineEdit->setPlaceholderText(TargetSetupPage::tr("Type to filter kits by name..."));

        centralWidget = new QWidget(setupTargetPage);
        QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        policy.setHorizontalStretch(0);
        policy.setVerticalStretch(0);
        policy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(policy);

        scrollAreaWidget = new QWidget(setupTargetPage);
        scrollArea = new QScrollArea(scrollAreaWidget);
        scrollArea->setWidgetResizable(true);

        auto scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 230, 81));
        scrollArea->setWidget(scrollAreaWidgetContents);

        auto verticalLayout = new QVBoxLayout(scrollAreaWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->addWidget(scrollArea);

        auto verticalLayout_2 = new QVBoxLayout(setupTargetPage);
        verticalLayout_2->addWidget(headerLabel);
        verticalLayout_2->addWidget(descriptionLabel);
        verticalLayout_2->addWidget(kitFilterLineEdit);
        verticalLayout_2->addWidget(noValidKitLabel);
        verticalLayout_2->addWidget(optionHintLabel);
        verticalLayout_2->addWidget(allKitsCheckBox);
        verticalLayout_2->addWidget(centralWidget);
        verticalLayout_2->addWidget(scrollAreaWidget);

        auto verticalLayout_3 = new QVBoxLayout(q);
        verticalLayout_3->setContentsMargins(0, 0, 0, -1);
        verticalLayout_3->addWidget(setupTargetPage);

        QObject::connect(optionHintLabel, &QLabel::linkActivated,
                         q, &TargetSetupPage::openOptions);

        QObject::connect(allKitsCheckBox, &QAbstractButton::clicked,
                         q, &TargetSetupPage::changeAllKitsSelections);

        QObject::connect(kitFilterLineEdit, &Utils::FancyLineEdit::filterChanged,
                         q, &TargetSetupPage::kitFilterChanged);
    }
};

} // namespace Internal

using namespace Internal;

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    Utils::WizardPage(parent),
    m_ui(new TargetSetupPageUi),
    m_importWidget(new ImportWidget(this)),
    m_spacer(new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding))
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

    auto centralWidget = new QWidget(this);
    m_ui->scrollArea->setWidget(centralWidget);
    centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->layout()->setMargin(0);

    setTitle(tr("Kit Selection"));

    for (IPotentialKit *pk : g_potentialKits)
        if (pk->isEnabled())
            m_potentialWidgets.append(pk->createWidget(this));

    setUseScrollArea(true);

    KitManager *km = KitManager::instance();
    // do note that those slots are triggered once *per* targetsetuppage
    // thus the same slot can be triggered multiple times on different instances!
    connect(km, &KitManager::kitAdded, this, &TargetSetupPage::handleKitAddition);
    connect(km, &KitManager::kitRemoved, this, &TargetSetupPage::handleKitRemoval);
    connect(km, &KitManager::kitUpdated, this, &TargetSetupPage::handleKitUpdate);
    connect(m_importWidget, &ImportWidget::importFrom,
            this, [this](const Utils::FileName &dir) { import(dir); });

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Kits"));
}

void TargetSetupPage::initializePage()
{
    reset();

    setupWidgets();
    setupImports();
    selectAtLeastOneKit();
}

void TargetSetupPage::setRequiredKitPredicate(const Kit::Predicate &predicate)
{
    m_requiredPredicate = predicate;
}

QList<Core::Id> TargetSetupPage::selectedKits() const
{
    QList<Core::Id> result;
    for (TargetSetupWidget *w : m_widgets) {
        if (w->isKitSelected())
            result.append(w->kit()->id());
    }
    return result;
}

void TargetSetupPage::setPreferredKitPredicate(const Kit::Predicate &predicate)
{
    m_preferredPredicate = predicate;
}

TargetSetupPage::~TargetSetupPage()
{
    disconnect();
    reset();
    delete m_ui;
}

bool TargetSetupPage::isComplete() const
{
    return Utils::anyOf(m_widgets, [](const TargetSetupWidget *w) {
        return w->isKitSelected();
    });
}

void TargetSetupPage::setupWidgets(const QString &filterText)
{
    // Known profiles:
    auto kitList = sortedKitList(m_requiredPredicate);

    foreach (Kit *k, kitList) {
        if (filterText.isEmpty() || k->displayName().contains(filterText, Qt::CaseInsensitive))
            addWidget(k);
    }

    // Setup import widget:
    m_importWidget->setCurrentDirectory(Internal::importDirectory(m_projectPath));

    updateVisibility();
    selectAtLeastOneKit();
}

void TargetSetupPage::reset()
{
    while (m_widgets.size() > 0) {
        TargetSetupWidget *w = m_widgets.back();

        Kit *k = w->kit();
        if (k && m_importer)
            m_importer->removeProject(k);

        removeWidget(w);
    }

    m_ui->allKitsCheckBox->setChecked(false);
}

TargetSetupWidget *TargetSetupPage::widget(const Core::Id kitId,
                                           TargetSetupWidget *fallback) const
{
    return Utils::findOr(m_widgets, fallback, [kitId](const TargetSetupWidget *w) {
        return w->kit() && w->kit()->id() == kitId;
    });
}

void TargetSetupPage::setProjectPath(const QString &path)
{
    m_projectPath = path;
    if (!m_projectPath.isEmpty()) {
        QFileInfo fileInfo(QDir::cleanPath(path));
        QStringList subDirsList = fileInfo.absolutePath().split('/');
        m_ui->headerLabel->setText(tr("The following kits can be used for project <b>%1</b>:",
                                      "%1: Project name").arg(subDirsList.last()));
    }
    m_ui->headerLabel->setVisible(!m_projectPath.isEmpty());

    if (m_widgetsWereSetUp)
        initializePage();
}

void TargetSetupPage::setProjectImporter(ProjectImporter *importer)
{
    if (importer == m_importer)
        return;

    if (m_widgetsWereSetUp)
        reset(); // Reset before changing the importer!

    m_importer = importer;
    m_importWidget->setVisible(m_importer);

    if (m_widgetsWereSetUp)
        initializePage();
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

    QStringList toImport = m_importer->importCandidates();
    foreach (const QString &path, toImport)
        import(Utils::FileName::fromString(path), true);
}

void TargetSetupPage::handleKitAddition(Kit *k)
{
    if (isUpdating())
        return;

    Q_ASSERT(!widget(k));
    addWidget(k);
    updateVisibility();
}

void TargetSetupPage::handleKitRemoval(Kit *k)
{
    if (isUpdating())
        return;

    if (m_importer)
        m_importer->cleanupKit(k);

    removeWidget(k);
    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPage::handleKitUpdate(Kit *k)
{
    if (isUpdating())
        return;

    if (m_importer)
        m_importer->makePersistent(k);

    bool acceptable = !m_requiredPredicate || m_requiredPredicate(k);

    if (!acceptable)
        removeWidget(k);
    else if (acceptable)
        addWidget(k);

    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPage::selectAtLeastOneKit()
{
    const bool atLeastOneKitSelected = Utils::anyOf(m_widgets, [](TargetSetupWidget *w) {
            return w->isKitSelected();
    });

    if (!atLeastOneKitSelected) {
        TargetSetupWidget *w = m_firstWidget;
        Kit *defaultKit = KitManager::defaultKit();
        if (defaultKit)
            w = widget(defaultKit->id(), m_firstWidget);
        if (w) {
            w->setKitSelected(true);
            kitSelectionChanged();
        }
        m_firstWidget = nullptr;
    }
    emit completeChanged(); // Is this necessary?
}

void TargetSetupPage::updateVisibility()
{
    // Always show the widgets, the import widget always makes sense to show.
    m_ui->scrollAreaWidget->setVisible(m_baseLayout == m_ui->scrollArea->widget()->layout());
    m_ui->centralWidget->setVisible(m_baseLayout == m_ui->centralWidget->layout());

    bool hasKits = m_widgets.size() > 0;
    m_ui->noValidKitLabel->setVisible(!hasKits);
    m_ui->optionHintLabel->setVisible(m_forceOptionHint || !hasKits);
    m_ui->allKitsCheckBox->setVisible(hasKits);

    emit completeChanged();
}

void TargetSetupPage::openOptions()
{
    Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, this);
}

void TargetSetupPage::kitSelectionChanged()
{
    int selected = 0;
    int deselected = 0;
    for (const TargetSetupWidget *widget : m_widgets) {
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

QList<Kit *> TargetSetupPage::sortedKitList(const Kit::Predicate &predicate)
{
    const auto kitList = KitManager::kits(predicate);

    return KitManager::sortKits(kitList);
}

void TargetSetupPage::kitFilterChanged(const QString &filterText)
{
    // Reset currently shown kits
    reset();
    setupWidgets(filterText);
}

void TargetSetupPage::changeAllKitsSelections()
{
    if (m_ui->allKitsCheckBox->checkState() == Qt::PartiallyChecked)
        m_ui->allKitsCheckBox->setCheckState(Qt::Checked);
    bool checked = m_ui->allKitsCheckBox->isChecked();
    for (TargetSetupWidget *widget : m_widgets)
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
        TargetSetupWidget *w = widget(info->kitId);
        if (!w) {
            Kit *k = KitManager::kit(info->kitId);
            Q_ASSERT(k);
            addWidget(k);
        }
        w = widget(info->kitId);
        if (!w) {
            delete info;
            continue;
        }

        w->addBuildInfo(info, true);
        w->setKitSelected(true);
        w->expandWidget();
        kitSelectionChanged();
    }
    emit completeChanged();
}

void TargetSetupPage::removeWidget(TargetSetupWidget *w)
{
    if (!w)
        return;
    if (w == m_firstWidget)
        m_firstWidget = nullptr;
    w->deleteLater();
    w->clearKit();
    m_widgets.erase(std::find(m_widgets.begin(), m_widgets.end(), w));
}

TargetSetupWidget *TargetSetupPage::addWidget(Kit *k)
{
    if (!k || (m_requiredPredicate && !m_requiredPredicate(k)))
        return nullptr;

    const IBuildConfigurationFactory *const factory
            = IBuildConfigurationFactory::find(k, m_projectPath);
    const QList<BuildInfo *> infoList = [this, k, factory]() {
        if (factory)
            return factory->availableSetups(k, m_projectPath);

        BuildInfo *info = new BuildInfo(nullptr);
        info->kitId = k->id();
        return QList<BuildInfo *>({info});
    }();

    // Not all projects have BuildConfigurations, that is perfectly fine.
    TargetSetupWidget *widget = new TargetSetupWidget(k, m_projectPath);

    m_baseLayout->removeWidget(m_importWidget);
    foreach (QWidget *potentialWidget, m_potentialWidgets)
        m_baseLayout->removeWidget(potentialWidget);
    m_baseLayout->removeItem(m_spacer);

    widget->setKitSelected(m_preferredPredicate && m_preferredPredicate(k));
    m_widgets.push_back(widget);
    connect(widget, &TargetSetupWidget::selectedToggled,
            this, &TargetSetupPage::kitSelectionChanged);
    m_baseLayout->addWidget(widget);

    m_baseLayout->addWidget(m_importWidget);
    foreach (QWidget *widget, m_potentialWidgets)
        m_baseLayout->addWidget(widget);
    m_baseLayout->addItem(m_spacer);

    connect(widget, &TargetSetupWidget::selectedToggled, this, &QWizardPage::completeChanged);

    if (!m_firstWidget)
        m_firstWidget = widget;

    return widget;
}

bool TargetSetupPage::setupProject(Project *project)
{
    QList<const BuildInfo *> toSetUp; // Pointers are managed by the widgets!
    for (TargetSetupWidget *widget : m_widgets) {
        if (!widget->isKitSelected())
            continue;

        Kit *k = widget->kit();

        if (k && m_importer)
            m_importer->makePersistent(k);
        toSetUp << widget->selectedBuildInfoList();
        widget->clearKit();
    }

    project->setup(toSetUp);
    toSetUp.clear();

    // Only reset now that toSetUp has been cleared!
    reset();

    Target *activeTarget = nullptr;
    if (m_importer)
        activeTarget = m_importer->preferredTarget(project->targets());
    if (activeTarget)
        SessionManager::setActiveTarget(project, activeTarget, SetActive::NoCascade);

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
