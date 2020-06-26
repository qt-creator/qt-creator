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
#include "importwidget.h"
#include "kit.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "session.h"
#include "target.h"
#include "targetsetupwidget.h"
#include "task.h"

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

using namespace Utils;

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
static FilePath importDirectory(const FilePath &projectPath)
{
    // Setup import widget:
    auto path = projectPath;
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
    QLabel *noValidKitLabel;
    QCheckBox *allKitsCheckBox;
    FancyLineEdit *kitFilterLineEdit;

    void setupUi(TargetSetupPage *q)
    {
        auto setupTargetPage = new QWidget(q);

        headerLabel = new QLabel(setupTargetPage);
        headerLabel->setWordWrap(true);
        headerLabel->setVisible(false);

        noValidKitLabel = new QLabel(setupTargetPage);
        noValidKitLabel->setWordWrap(true);
        noValidKitLabel->setText("<span style=\" font-weight:600;\">"
                                 + TargetSetupPage::tr("No suitable kits found.") + "</span><br/>"
                                 + TargetSetupPage::tr("Add a kit in the <a href=\"buildandrun\">"
                                                       "options</a> or via the maintenance tool of"
                                                       " the SDK."));
        noValidKitLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        noValidKitLabel->setVisible(false);

        allKitsCheckBox = new QCheckBox(setupTargetPage);
        allKitsCheckBox->setTristate(true);
        allKitsCheckBox->setText(TargetSetupPage::tr("Select all kits"));

        kitFilterLineEdit = new FancyLineEdit(setupTargetPage);
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
        verticalLayout_2->addWidget(kitFilterLineEdit);
        verticalLayout_2->addWidget(noValidKitLabel);
        verticalLayout_2->addWidget(allKitsCheckBox);
        verticalLayout_2->addWidget(centralWidget);
        verticalLayout_2->addWidget(scrollAreaWidget);

        auto verticalLayout_3 = new QVBoxLayout(q);
        verticalLayout_3->setContentsMargins(0, 0, 0, -1);
        verticalLayout_3->addWidget(setupTargetPage);

        QObject::connect(noValidKitLabel, &QLabel::linkActivated,
                         q, &TargetSetupPage::openOptions);

        QObject::connect(allKitsCheckBox, &QAbstractButton::clicked,
                         q, &TargetSetupPage::changeAllKitsSelections);

        QObject::connect(kitFilterLineEdit, &FancyLineEdit::filterChanged,
                         q, &TargetSetupPage::kitFilterChanged);
    }
};

} // namespace Internal

static TasksGenerator defaultTasksGenerator(const TasksGenerator &childGenerator)
{
    return [childGenerator](const Kit *k) -> Tasks {
        if (!k->isValid())
            return {
                CompileTask(Task::Error,
                            QCoreApplication::translate("ProjectExplorer", "Kit is not valid."))};
        if (childGenerator)
            return childGenerator(k);
        return {};
    };
}

using namespace Internal;

TargetSetupPage::TargetSetupPage(QWidget *parent)
    : WizardPage(parent)
    , m_tasksGenerator(defaultTasksGenerator({}))
    , m_ui(new TargetSetupPageUi)
    , m_importWidget(new ImportWidget(this))
    , m_spacer(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding))
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
    m_ui->centralWidget->layout()->setContentsMargins(0, 0, 0, 0);

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
            this, [this](const FilePath &dir) { import(dir); });
    connect(KitManager::instance(), &KitManager::kitsChanged,
            this, &TargetSetupPage::updateVisibility);

    setProperty(SHORT_TITLE_PROPERTY, tr("Kits"));
}

void TargetSetupPage::initializePage()
{
    if (KitManager::isLoaded()) {
        doInitializePage();
    } else {
        connect(KitManager::instance(), &KitManager::kitsLoaded,
                this, &TargetSetupPage::doInitializePage);
    }
}

void TargetSetupPage::setTasksGenerator(const TasksGenerator &tasksGenerator)
{
    m_tasksGenerator = defaultTasksGenerator(tasksGenerator);
}

QList<Utils::Id> TargetSetupPage::selectedKits() const
{
    QList<Utils::Id> result;
    for (TargetSetupWidget *w : m_widgets) {
        if (w->isKitSelected())
            result.append(w->kit()->id());
    }
    return result;
}

TargetSetupPage::~TargetSetupPage()
{
    disconnect();
    reset();
    delete m_spacer;
    delete m_ui;
}

bool TargetSetupPage::isComplete() const
{
    return anyOf(m_widgets, [](const TargetSetupWidget *w) {
        return w->isKitSelected();
    });
}

void TargetSetupPage::setupWidgets(const QString &filterText)
{
    const auto kitList = KitManager::sortKits(KitManager::kits());
    for (Kit *k : kitList) {
        if (!filterText.isEmpty() && !k->displayName().contains(filterText, Qt::CaseInsensitive))
            continue;
        const auto widget = new TargetSetupWidget(k, m_projectPath);
        connect(widget, &TargetSetupWidget::selectedToggled,
                this, &TargetSetupPage::kitSelectionChanged);
        connect(widget, &TargetSetupWidget::selectedToggled, this, &QWizardPage::completeChanged);
        updateWidget(widget);
        m_widgets.push_back(widget);
        m_baseLayout->addWidget(widget);
    }
    addAdditionalWidgets();

    // Setup import widget:
    m_importWidget->setCurrentDirectory(Internal::importDirectory(m_projectPath));

    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPage::reset()
{
    removeAdditionalWidgets();
    while (m_widgets.size() > 0) {
        TargetSetupWidget *w = m_widgets.back();

        Kit *k = w->kit();
        if (k && m_importer)
            m_importer->removeProject(k);

        removeWidget(w);
    }

    m_ui->allKitsCheckBox->setChecked(false);
}

TargetSetupWidget *TargetSetupPage::widget(const Utils::Id kitId,
                                           TargetSetupWidget *fallback) const
{
    return findOr(m_widgets, fallback, [kitId](const TargetSetupWidget *w) {
        return w->kit() && w->kit()->id() == kitId;
    });
}

void TargetSetupPage::setProjectPath(const FilePath &path)
{
    m_projectPath = path;
    if (!m_projectPath.isEmpty()) {
        QFileInfo fileInfo(QDir::cleanPath(path.toString()));
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

bool TargetSetupPage::importLineEditHasFocus() const
{
    return m_importWidget->ownsReturnKey();
}

void TargetSetupPage::setupImports()
{
    if (!m_importer || m_projectPath.isEmpty())
        return;

    const QStringList toImport = m_importer->importCandidates();
    for (const QString &path : toImport)
        import(FilePath::fromString(path), true);
}

void TargetSetupPage::handleKitAddition(Kit *k)
{
    if (isUpdating())
        return;

    Q_ASSERT(!widget(k));
    addWidget(k);
    kitSelectionChanged();
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

    const auto newWidgetList = sortedWidgetList();
    if (newWidgetList != m_widgets) { // Sorting has changed.
        m_widgets = newWidgetList;
        reLayout();
    }
    updateWidget(widget(k));
    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPage::selectAtLeastOneEnabledKit()
{
    if (anyOf(m_widgets, [](const TargetSetupWidget *w) { return w->isKitSelected(); })) {
        // Something is already selected, we are done.
        return;
    }

    TargetSetupWidget *toCheckWidget = nullptr;

    const Kit *defaultKit = KitManager::defaultKit();

    auto isPreferred = [this](const TargetSetupWidget *w) {
        const Tasks tasks = m_tasksGenerator(w->kit());
        return w->isEnabled() && tasks.isEmpty();
    };

    // Use default kit if that is preferred:
    toCheckWidget = findOrDefault(m_widgets, [defaultKit, isPreferred](const TargetSetupWidget *w) {
        return isPreferred(w) && w->kit() == defaultKit;
    });

    if (!toCheckWidget) {
        // Use the first preferred widget:
        toCheckWidget = findOrDefault(m_widgets, isPreferred);
    }

    if (!toCheckWidget) {
        // Use default kit if it is enabled:
        toCheckWidget = findOrDefault(m_widgets, [defaultKit](const TargetSetupWidget *w) {
            return w->isEnabled() && w->kit() == defaultKit;
        });
    }

    if (!toCheckWidget) {
        // Use the first enabled widget:
        toCheckWidget = findOrDefault(m_widgets,
                                      [](const TargetSetupWidget *w) { return w->isEnabled(); });
    }

    if (toCheckWidget) {
        toCheckWidget->setKitSelected(true);

        emit completeChanged(); // Is this necessary?
    }
}

void TargetSetupPage::updateVisibility()
{
    // Always show the widgets, the import widget always makes sense to show.
    m_ui->scrollAreaWidget->setVisible(m_baseLayout == m_ui->scrollArea->widget()->layout());
    m_ui->centralWidget->setVisible(m_baseLayout == m_ui->centralWidget->layout());

    const bool hasUsableKits = KitManager::kit([this](const Kit *k) { return isUsable(k); });
    m_ui->noValidKitLabel->setVisible(!hasUsableKits);
    m_ui->allKitsCheckBox->setVisible(hasUsableKits);

    emit completeChanged();
}

void TargetSetupPage::reLayout()
{
    removeAdditionalWidgets();
    for (TargetSetupWidget * const w : qAsConst(m_widgets))
        m_baseLayout->removeWidget(w);
    for (TargetSetupWidget * const w : qAsConst(m_widgets))
        m_baseLayout->addWidget(w);
    addAdditionalWidgets();
}

bool TargetSetupPage::compareKits(const Kit *k1, const Kit *k2)
{
    const QString name1 = k1->displayName();
    const QString name2 = k2->displayName();
    if (name1 < name2)
        return true;
    if (name1 > name2)
        return false;
    return k1 < k2;
}

std::vector<TargetSetupWidget *> TargetSetupPage::sortedWidgetList() const
{
    std::vector<TargetSetupWidget *> list = m_widgets;
    sort(list, [](const TargetSetupWidget *w1, const TargetSetupWidget *w2) {
        return compareKits(w1->kit(), w2->kit());
    });
    return list;
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

void TargetSetupPage::kitFilterChanged(const QString &filterText)
{
    // Remember selected kits:
    const std::vector<TargetSetupWidget *> selectedWidgets
        = filtered(m_widgets, &TargetSetupWidget::isKitSelected);
    const QVector<Utils::Id> selectedKitIds = transform<QVector>(selectedWidgets,
                                                                [](const TargetSetupWidget *w) {
                                                                    return w->kit()->id();
                                                                });

    // Reset currently shown kits
    reset();
    setupWidgets(filterText);

    // Re-select kits:
    for (TargetSetupWidget *w : qAsConst(m_widgets))
        w->setKitSelected(selectedKitIds.contains(w->kit()->id()));
}

void TargetSetupPage::doInitializePage()
{
    reset();
    setupWidgets();
    setupImports();

    selectAtLeastOneEnabledKit();

    updateVisibility();
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
    return m_importer && m_importer->isUpdating();
}

void TargetSetupPage::import(const FilePath &path, bool silent)
{
    if (!m_importer)
        return;

    for (const BuildInfo &info : m_importer->import(path, silent)) {
        TargetSetupWidget *w = widget(info.kitId);
        if (!w) {
            Kit *k = KitManager::kit(info.kitId);
            Q_ASSERT(k);
            addWidget(k);
        }
        w = widget(info.kitId);
        if (!w)
            continue;

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
    w->deleteLater();
    w->clearKit();
    m_widgets.erase(std::find(m_widgets.begin(), m_widgets.end(), w));
}

TargetSetupWidget *TargetSetupPage::addWidget(Kit *k)
{
    const auto widget = new TargetSetupWidget(k, m_projectPath);
    updateWidget(widget);
    connect(widget, &TargetSetupWidget::selectedToggled,
            this, &TargetSetupPage::kitSelectionChanged);
    connect(widget, &TargetSetupWidget::selectedToggled, this, &QWizardPage::completeChanged);


    // Insert widget, sorted.
    const auto insertionPos = std::find_if(m_widgets.begin(), m_widgets.end(),
                                           [k](const TargetSetupWidget *w) {
        return compareKits(k, w->kit());
    });
    const bool addedToEnd = insertionPos == m_widgets.end();
    m_widgets.insert(insertionPos, widget);
    if (addedToEnd) {
        removeAdditionalWidgets();
        m_baseLayout->addWidget(widget);
        addAdditionalWidgets();
    } else {
        reLayout();
    }

    return widget;
}

void TargetSetupPage::addAdditionalWidgets()
{
    m_baseLayout->addWidget(m_importWidget);
    for (QWidget * const widget : qAsConst(m_potentialWidgets))
        m_baseLayout->addWidget(widget);
    m_baseLayout->addItem(m_spacer);
}

void TargetSetupPage::removeAdditionalWidgets(QLayout *layout)
{
    layout->removeWidget(m_importWidget);
    for (QWidget * const potentialWidget : qAsConst(m_potentialWidgets))
        layout->removeWidget(potentialWidget);
    layout->removeItem(m_spacer);
}

void TargetSetupPage::updateWidget(TargetSetupWidget *widget)
{
    QTC_ASSERT(widget, return );
    widget->update(m_tasksGenerator);
}

bool TargetSetupPage::isUsable(const Kit *kit) const
{
    return !containsType(m_tasksGenerator(kit), Task::Error);
}

bool TargetSetupPage::setupProject(Project *project)
{
    QList<BuildInfo> toSetUp;
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

    if (oldBaseLayout)
        removeAdditionalWidgets(oldBaseLayout);
    addAdditionalWidgets();
}

} // namespace ProjectExplorer
