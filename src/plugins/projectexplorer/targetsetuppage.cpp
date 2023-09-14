// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "targetsetuppage.h"

#include "buildinfo.h"
#include "importwidget.h"
#include "ipotentialkit.h"
#include "kit.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectimporter.h"
#include "target.h"
#include "targetsetupwidget.h"
#include "task.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/wizard.h>

#include <QApplication>
#include <QCheckBox>
#include <QFileInfo>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

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

static TasksGenerator defaultTasksGenerator(const TasksGenerator &childGenerator)
{
    return [childGenerator](const Kit *k) -> Tasks {
        if (!k->isValid())
            return {CompileTask(Task::Error, Tr::tr("Kit is not valid."))};
        if (childGenerator)
            return childGenerator(k);
        return {};
    };
}

class TargetSetupPagePrivate : public QObject
{
public:
    explicit TargetSetupPagePrivate(TargetSetupPage *parent)
        : q(parent)
    {
        m_tasksGenerator = defaultTasksGenerator({});

        m_importWidget = new ImportWidget(q);
        m_importWidget->setVisible(false);

        m_spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        auto setupTargetPage = new QWidget(q);

        headerLabel = new QLabel(setupTargetPage);
        headerLabel->setWordWrap(true);
        headerLabel->setVisible(false);

        noValidKitLabel = new QLabel(setupTargetPage);
        noValidKitLabel->setWordWrap(true);
        noValidKitLabel->setText("<span style=\" font-weight:600;\">"
                                 + Tr::tr("No suitable kits found.") + "</span><br/>"
                                 + Tr::tr("Add a kit in the <a href=\"buildandrun\">"
                                          "options</a> or via the maintenance tool of"
                                          " the SDK."));
        noValidKitLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        noValidKitLabel->setVisible(false);

        allKitsCheckBox = new QCheckBox(setupTargetPage);
        allKitsCheckBox->setTristate(true);
        allKitsCheckBox->setText(Tr::tr("Select all kits"));

        kitFilterLineEdit = new FancyLineEdit(setupTargetPage);
        kitFilterLineEdit->setFiltering(true);
        kitFilterLineEdit->setPlaceholderText(Tr::tr("Type to filter kits by name..."));

        m_centralWidget = new QWidget(setupTargetPage);
        QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        policy.setHorizontalStretch(0);
        policy.setVerticalStretch(0);
        policy.setHeightForWidth(m_centralWidget->sizePolicy().hasHeightForWidth());
        m_centralWidget->setSizePolicy(policy);

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

        auto horizontalLayout = new QHBoxLayout;
        horizontalLayout->addWidget(allKitsCheckBox);
        horizontalLayout->addSpacing(10);
        horizontalLayout->addWidget(kitFilterLineEdit);

        auto verticalLayout_2 = new QVBoxLayout(setupTargetPage);
        verticalLayout_2->addWidget(headerLabel);
        verticalLayout_2->addLayout(horizontalLayout);
        verticalLayout_2->addWidget(noValidKitLabel);
        verticalLayout_2->addWidget(m_centralWidget);
        verticalLayout_2->addWidget(scrollAreaWidget);

        auto verticalLayout_3 = new QVBoxLayout(q);
        verticalLayout_3->setContentsMargins(0, 0, 0, -1);
        verticalLayout_3->addWidget(setupTargetPage);

        auto centralWidget = new QWidget(q);
        scrollArea->setWidget(centralWidget);
        centralWidget->setLayout(new QVBoxLayout);

        m_centralWidget->setLayout(new QVBoxLayout);
        m_centralWidget->layout()->setContentsMargins(0, 0, 0, 0);

        QObject::connect(noValidKitLabel, &QLabel::linkActivated,
                         q, &TargetSetupPage::openOptions);

        QObject::connect(allKitsCheckBox, &QAbstractButton::clicked,
                         q, &TargetSetupPage::changeAllKitsSelections);

        QObject::connect(kitFilterLineEdit, &FancyLineEdit::filterChanged,
                         this, &TargetSetupPagePrivate::kitFilterChanged);

        for (IPotentialKit *pk : std::as_const(g_potentialKits)) {
            if (pk->isEnabled())
                m_potentialWidgets.append(pk->createWidget(q));
        }

        setUseScrollArea(true);

        KitManager *km = KitManager::instance();
        // do note that those slots are triggered once *per* targetsetuppage
        // thus the same slot can be triggered multiple times on different instances!
        connect(km, &KitManager::kitAdded, this, &TargetSetupPagePrivate::handleKitAddition);
        connect(km, &KitManager::kitRemoved, this, &TargetSetupPagePrivate::handleKitRemoval);
        connect(km, &KitManager::kitUpdated, this, &TargetSetupPagePrivate::handleKitUpdate);
        connect(m_importWidget, &ImportWidget::importFrom,
                this, [this](const FilePath &dir) { import(dir); });
        connect(KitManager::instance(), &KitManager::kitsChanged,
                this, &TargetSetupPagePrivate::updateVisibility);

    }

    void doInitializePage();
    void handleKitAddition(Kit *k);
    void handleKitRemoval(Kit *k);
    void handleKitUpdate(Kit *k);
    void updateVisibility();

    void reLayout();
    static bool compareKits(const Kit *k1, const Kit *k2);
    std::vector<Internal::TargetSetupWidget *> sortedWidgetList() const;

    void kitSelectionChanged();

    bool isUpdating() const;
    void selectAtLeastOneEnabledKit();
    void removeWidget(Kit *k) { removeWidget(widget(k)); }
    void removeWidget(Internal::TargetSetupWidget *w);
    Internal::TargetSetupWidget *addWidget(Kit *k);
    void addAdditionalWidgets();
    void removeAdditionalWidgets(QLayout *layout);
    void removeAdditionalWidgets();
    void updateWidget(Internal::TargetSetupWidget *widget);
    bool isUsable(const Kit *kit) const;

    void setupImports();
    void import(const FilePath &path, bool silent = false);

    void setupWidgets(const QString &filterText = QString());
    void reset();

    TargetSetupWidget *widget(const Id kitId, TargetSetupWidget *fallback = nullptr) const;
    TargetSetupWidget *widget(const Kit *k, TargetSetupWidget *fallback = nullptr) const
    {
        return k ? widget(k->id(), fallback) : fallback;
    }

    void setUseScrollArea(bool b);
    void kitFilterChanged(const QString &filterText);

    TargetSetupPage *q;
    QWidget *m_centralWidget;
    QWidget *scrollAreaWidget;
    QScrollArea *scrollArea;
    QLabel *headerLabel;
    QLabel *noValidKitLabel;
    QCheckBox *allKitsCheckBox;
    FancyLineEdit *kitFilterLineEdit;

    TasksGenerator m_tasksGenerator;
    QPointer<ProjectImporter> m_importer;
    QLayout *m_baseLayout = nullptr;
    FilePath m_projectPath;
    QString m_defaultShadowBuildLocation;
    std::vector<Internal::TargetSetupWidget *> m_widgets;

    Internal::ImportWidget *m_importWidget = nullptr;
    QSpacerItem *m_spacer;
    QList<QWidget *> m_potentialWidgets;

    bool m_widgetsWereSetUp = false;
};

} // namespace Internal

using namespace Internal;

TargetSetupPage::TargetSetupPage(QWidget *parent)
    : WizardPage(parent)
    , d(new TargetSetupPagePrivate(this))
{
    setObjectName(QLatin1String("TargetSetupPage"));
    setWindowTitle(Tr::tr("Select Kits for Your Project"));
    setTitle(Tr::tr("Kit Selection"));

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHorizontalStretch(0);
    policy.setVerticalStretch(0);
    setSizePolicy(policy);

    setProperty(SHORT_TITLE_PROPERTY, Tr::tr("Kits"));
}

void TargetSetupPage::initializePage()
{
    if (KitManager::isLoaded()) {
        d->doInitializePage();
    } else {
        connect(KitManager::instance(), &KitManager::kitsLoaded,
                d, &TargetSetupPagePrivate::doInitializePage);
    }
}

void TargetSetupPage::setTasksGenerator(const TasksGenerator &tasksGenerator)
{
    d->m_tasksGenerator = defaultTasksGenerator(tasksGenerator);
}

QList<Id> TargetSetupPage::selectedKits() const
{
    QList<Id> result;
    for (TargetSetupWidget *w : d->m_widgets) {
        if (w->isKitSelected())
            result.append(w->kit()->id());
    }
    return result;
}

TargetSetupPage::~TargetSetupPage()
{
    disconnect();
    d->reset();
    delete d->m_spacer;
    delete d;
}

bool TargetSetupPage::isComplete() const
{
    return anyOf(d->m_widgets, [](const TargetSetupWidget *w) {
        return w->isKitSelected();
    });
}

void TargetSetupPagePrivate::setupWidgets(const QString &filterText)
{
    for (Kit *k : KitManager::sortedKits()) {
        if (!filterText.isEmpty() && !k->displayName().contains(filterText, Qt::CaseInsensitive))
            continue;
        const auto widget = new TargetSetupWidget(k, m_projectPath);
        connect(widget, &TargetSetupWidget::selectedToggled,
                this, &TargetSetupPagePrivate::kitSelectionChanged);
        connect(widget, &TargetSetupWidget::selectedToggled,
                q, &QWizardPage::completeChanged);
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

void TargetSetupPagePrivate::reset()
{
    removeAdditionalWidgets();
    while (m_widgets.size() > 0) {
        TargetSetupWidget *w = m_widgets.back();

        Kit *k = w->kit();
        if (k && m_importer)
            m_importer->removeProject(k);

        removeWidget(w);
    }

    allKitsCheckBox->setChecked(false);
}

TargetSetupWidget *TargetSetupPagePrivate::widget(const Id kitId, TargetSetupWidget *fallback) const
{
    return findOr(m_widgets, fallback, [kitId](const TargetSetupWidget *w) {
        return w->kit() && w->kit()->id() == kitId;
    });
}

void TargetSetupPage::setProjectPath(const FilePath &path)
{
    d->m_projectPath = path;
    if (!d->m_projectPath.isEmpty()) {
        QFileInfo fileInfo(QDir::cleanPath(path.toString()));
        QStringList subDirsList = fileInfo.absolutePath().split('/');
        d->headerLabel->setText(Tr::tr("The following kits can be used for project <b>%1</b>:",
                                      "%1: Project name").arg(subDirsList.last()));
    }
    d->headerLabel->setVisible(!d->m_projectPath.isEmpty());

    if (d->m_widgetsWereSetUp)
        initializePage();
}

void TargetSetupPage::setProjectImporter(ProjectImporter *importer)
{
    if (importer == d->m_importer)
        return;

    if (d->m_widgetsWereSetUp)
        d->reset(); // Reset before changing the importer!

    d->m_importer = importer;
    d->m_importWidget->setVisible(d->m_importer);

    if (d->m_widgetsWereSetUp)
        initializePage();
}

bool TargetSetupPage::importLineEditHasFocus() const
{
    return d->m_importWidget->ownsReturnKey();
}

void TargetSetupPagePrivate::setupImports()
{
    if (!m_importer || m_projectPath.isEmpty())
        return;

    const FilePaths toImport = m_importer->importCandidates();
    for (const FilePath &path : toImport)
        import(path, true);
}

void TargetSetupPagePrivate::handleKitAddition(Kit *k)
{
    if (isUpdating())
        return;

    QTC_ASSERT(!widget(k), return);
    addWidget(k);
    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPagePrivate::handleKitRemoval(Kit *k)
{
    if (isUpdating())
        return;

    if (m_importer)
        m_importer->cleanupKit(k);

    removeWidget(k);
    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPagePrivate::handleKitUpdate(Kit *k)
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

void TargetSetupPagePrivate::selectAtLeastOneEnabledKit()
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

        emit q->completeChanged(); // FIXME: Is this necessary?
    }
}

void TargetSetupPagePrivate::updateVisibility()
{
    // Always show the widgets, the import widget always makes sense to show.
    scrollAreaWidget->setVisible(m_baseLayout == scrollArea->widget()->layout());
    m_centralWidget->setVisible(m_baseLayout == m_centralWidget->layout());

    const bool hasUsableKits = KitManager::kit([this](const Kit *k) { return isUsable(k); });
    noValidKitLabel->setVisible(!hasUsableKits);
    allKitsCheckBox->setVisible(hasUsableKits);

    emit q->completeChanged();
}

void TargetSetupPagePrivate::reLayout()
{
    removeAdditionalWidgets();
    for (TargetSetupWidget * const w : std::as_const(m_widgets))
        m_baseLayout->removeWidget(w);
    for (TargetSetupWidget * const w : std::as_const(m_widgets))
        m_baseLayout->addWidget(w);
    addAdditionalWidgets();
}

bool TargetSetupPagePrivate::compareKits(const Kit *k1, const Kit *k2)
{
    const QString name1 = k1->displayName();
    const QString name2 = k2->displayName();
    if (name1 < name2)
        return true;
    if (name1 > name2)
        return false;
    return k1 < k2;
}

std::vector<TargetSetupWidget *> TargetSetupPagePrivate::sortedWidgetList() const
{
    return sorted(m_widgets, [](const TargetSetupWidget *w1, const TargetSetupWidget *w2) {
        return compareKits(w1->kit(), w2->kit());
    });
}

void TargetSetupPage::openOptions()
{
    Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, this);
}

void TargetSetupPagePrivate::kitSelectionChanged()
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
        allKitsCheckBox->setCheckState(Qt::PartiallyChecked);
    else if (selected > 0 && deselected == 0)
        allKitsCheckBox->setCheckState(Qt::Checked);
    else
        allKitsCheckBox->setCheckState(Qt::Unchecked);
}

void TargetSetupPagePrivate::kitFilterChanged(const QString &filterText)
{
    for (TargetSetupWidget *widget : m_widgets) {
        Kit *kit = widget->kit();
        widget->setVisible(filterText.isEmpty() || kit->displayName().contains(filterText, Qt::CaseInsensitive));
    }
}

void TargetSetupPagePrivate::doInitializePage()
{
    reset();
    setupWidgets();
    setupImports();

    selectAtLeastOneEnabledKit();

    updateVisibility();
}

void TargetSetupPage::showEvent(QShowEvent *event)
{
    WizardPage::showEvent(event);
    d->kitFilterLineEdit->setFocus(); // Ensure "Configure Project" gets triggered on <Return>
}

void TargetSetupPage::changeAllKitsSelections()
{
    if (d->allKitsCheckBox->checkState() == Qt::PartiallyChecked)
        d->allKitsCheckBox->setCheckState(Qt::Checked);
    bool checked = d->allKitsCheckBox->isChecked();
    for (TargetSetupWidget *widget : d->m_widgets)
        widget->setKitSelected(checked);
    emit completeChanged();
}

bool TargetSetupPagePrivate::isUpdating() const
{
    return m_importer && m_importer->isUpdating();
}

void TargetSetupPagePrivate::import(const FilePath &path, bool silent)
{
    if (!m_importer)
        return;

    for (const BuildInfo &info : m_importer->import(path, silent)) {
        TargetSetupWidget *w = widget(info.kitId);
        if (!w) {
            Kit *k = KitManager::kit(info.kitId);
            QTC_CHECK(k);
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
    emit q->completeChanged();
}

void TargetSetupPagePrivate::removeWidget(TargetSetupWidget *w)
{
    if (!w)
        return;
    w->deleteLater();
    w->clearKit();
    m_widgets.erase(std::find(m_widgets.begin(), m_widgets.end(), w));
}

TargetSetupWidget *TargetSetupPagePrivate::addWidget(Kit *k)
{
    const auto widget = new TargetSetupWidget(k, m_projectPath);
    updateWidget(widget);
    connect(widget, &TargetSetupWidget::selectedToggled,
            this, &TargetSetupPagePrivate::kitSelectionChanged);
    connect(widget, &TargetSetupWidget::selectedToggled,
            q, &QWizardPage::completeChanged);


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

void TargetSetupPagePrivate::addAdditionalWidgets()
{
    m_baseLayout->addWidget(m_importWidget);
    for (QWidget * const widget : std::as_const(m_potentialWidgets))
        m_baseLayout->addWidget(widget);
    m_baseLayout->addItem(m_spacer);
}

void TargetSetupPagePrivate::removeAdditionalWidgets(QLayout *layout)
{
    layout->removeWidget(m_importWidget);
    for (QWidget * const potentialWidget : std::as_const(m_potentialWidgets))
        layout->removeWidget(potentialWidget);
    layout->removeItem(m_spacer);
}

void TargetSetupPagePrivate::removeAdditionalWidgets()
{
    removeAdditionalWidgets(m_baseLayout);
}

void TargetSetupPagePrivate::updateWidget(TargetSetupWidget *widget)
{
    QTC_ASSERT(widget, return );
    widget->update(m_tasksGenerator);
}

bool TargetSetupPagePrivate::isUsable(const Kit *kit) const
{
    return !containsType(m_tasksGenerator(kit), Task::Error);
}

bool TargetSetupPage::setupProject(Project *project)
{
    QList<BuildInfo> toSetUp;
    for (TargetSetupWidget *widget : d->m_widgets) {
        if (!widget->isKitSelected())
            continue;

        Kit *k = widget->kit();

        if (k && d->m_importer)
            d->m_importer->makePersistent(k);
        toSetUp << widget->selectedBuildInfoList();
        widget->clearKit();
    }

    project->setup(toSetUp);
    toSetUp.clear();

    // Only reset now that toSetUp has been cleared!
    d->reset();

    Target *activeTarget = nullptr;
    if (d->m_importer)
        activeTarget = d->m_importer->preferredTarget(project->targets());
    if (activeTarget)
        project->setActiveTarget(activeTarget, SetActive::NoCascade);

    return true;
}

void TargetSetupPage::setUseScrollArea(bool b)
{
    d->setUseScrollArea(b);
}

void TargetSetupPagePrivate::setUseScrollArea(bool b)
{
    QLayout *oldBaseLayout = m_baseLayout;
    m_baseLayout = b ? scrollArea->widget()->layout() : m_centralWidget->layout();
    if (oldBaseLayout == m_baseLayout)
        return;
    scrollAreaWidget->setVisible(b);
    m_centralWidget->setVisible(!b);

    if (oldBaseLayout)
        removeAdditionalWidgets(oldBaseLayout);
    addAdditionalWidgets();
}

} // namespace ProjectExplorer
