// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "targetsetuppage.h"

#include "buildinfo.h"
#include "importwidget.h"
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
#include <utils/qtcprocess.h>
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
        tasksGenerator = defaultTasksGenerator({});

        importWidget = new ImportWidget(q);
        importWidget->setVisible(false);

        spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

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

        hideUnsuitableKitsCheckBox = new QCheckBox(Tr::tr("Hide unsuitable kits"), setupTargetPage);

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

        auto horizontalLayout = new QHBoxLayout;
        horizontalLayout->addWidget(allKitsCheckBox);
        horizontalLayout->addSpacing(10);
        horizontalLayout->addWidget(kitFilterLineEdit);

        auto verticalLayout_2 = new QVBoxLayout(setupTargetPage);
        verticalLayout_2->addWidget(headerLabel);
        verticalLayout_2->addLayout(horizontalLayout);
        verticalLayout_2->addWidget(hideUnsuitableKitsCheckBox);
        verticalLayout_2->addWidget(noValidKitLabel);
        verticalLayout_2->addWidget(centralWidget);
        verticalLayout_2->addWidget(scrollAreaWidget);

        auto verticalLayout_3 = new QVBoxLayout(q);
        verticalLayout_3->setContentsMargins(0, 0, 0, -1);
        verticalLayout_3->addWidget(setupTargetPage);

        auto centralWidget = new QWidget(q);
        scrollArea->setWidget(centralWidget);
        centralWidget->setLayout(new QVBoxLayout);

        this->centralWidget->setLayout(new QVBoxLayout);
        this->centralWidget->layout()->setContentsMargins(0, 0, 0, 0);

        QObject::connect(noValidKitLabel, &QLabel::linkActivated,
                         q, &TargetSetupPage::openOptions);

        QObject::connect(allKitsCheckBox, &QAbstractButton::clicked,
                         q, &TargetSetupPage::changeAllKitsSelections);

        const auto toggleTargetWidgetVisibility = [this] {
            for (TargetSetupWidget *widget : widgets)
                toggleVisibility(widget);
        };
        QObject::connect(hideUnsuitableKitsCheckBox, &QCheckBox::toggled,
                         this, toggleTargetWidgetVisibility);

        QObject::connect(kitFilterLineEdit, &FancyLineEdit::filterChanged,
                         this, toggleTargetWidgetVisibility);

        setUseScrollArea(true);

        KitManager *km = KitManager::instance();
        // do note that those slots are triggered once *per* targetsetuppage
        // thus the same slot can be triggered multiple times on different instances!
        connect(km, &KitManager::kitAdded, this, &TargetSetupPagePrivate::handleKitAddition);
        connect(km, &KitManager::kitRemoved, this, &TargetSetupPagePrivate::handleKitRemoval);
        connect(km, &KitManager::kitUpdated, this, &TargetSetupPagePrivate::handleKitUpdate);
        connect(importWidget, &ImportWidget::importFrom,
                this, [this](const FilePath &dir) { import(dir); });
        connect(KitManager::instance(), &KitManager::kitsChanged,
                this, &TargetSetupPagePrivate::updateVisibility);

        toggleTargetWidgetVisibility();
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
    TargetSetupWidget *addWidget(Kit *k);
    void connectWidget(TargetSetupWidget *w);
    void toggleVisibility(TargetSetupWidget *w);
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

    TargetSetupPage *q;
    QWidget *centralWidget;
    QWidget *scrollAreaWidget;
    QScrollArea *scrollArea;
    QLabel *headerLabel;
    QLabel *noValidKitLabel;
    QCheckBox *allKitsCheckBox;
    FancyLineEdit *kitFilterLineEdit;
    QCheckBox *hideUnsuitableKitsCheckBox;

    TasksGenerator tasksGenerator;
    QPointer<ProjectImporter> importer;
    QLayout *baseLayout = nullptr;
    FilePath projectPath;
    QString defaultShadowBuildLocation;
    std::vector<Internal::TargetSetupWidget *> widgets;

    Internal::ImportWidget *importWidget = nullptr;
    QSpacerItem *spacer;

    bool widgetsWereSetUp = false;
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
    d->tasksGenerator = defaultTasksGenerator(tasksGenerator);
}

QList<Id> TargetSetupPage::selectedKits() const
{
    QList<Id> result;
    for (TargetSetupWidget *w : d->widgets) {
        if (w->isKitSelected())
            result.append(w->kit()->id());
    }
    return result;
}

TargetSetupPage::~TargetSetupPage()
{
    disconnect();
    d->reset();
    delete d->spacer;
    delete d;
}

bool TargetSetupPage::isComplete() const
{
    return anyOf(d->widgets, [](const TargetSetupWidget *w) {
        return w->isKitSelected();
    });
}

void TargetSetupPagePrivate::setupWidgets(const QString &filterText)
{
    for (Kit *k : KitManager::sortedKits()) {
        if (!filterText.isEmpty() && !k->displayName().contains(filterText, Qt::CaseInsensitive))
            continue;
        if (importer && !importer->filter(k))
            continue;
        const auto widget = new TargetSetupWidget(k, projectPath);
        updateWidget(widget);
        widgets.push_back(widget);
        baseLayout->addWidget(widget);
    }
    addAdditionalWidgets();

    // Setup import widget:
    importWidget->setCurrentDirectory(Internal::importDirectory(projectPath));

    kitSelectionChanged();
    updateVisibility();
    for (TargetSetupWidget * const w : widgets) {
        connectWidget(w);
        toggleVisibility(w);
    }
}

void TargetSetupPagePrivate::reset()
{
    removeAdditionalWidgets();
    while (widgets.size() > 0) {
        TargetSetupWidget *w = widgets.back();

        Kit *k = w->kit();
        if (k && importer)
            importer->removeProject(k);

        removeWidget(w);
    }

    allKitsCheckBox->setChecked(false);
    hideUnsuitableKitsCheckBox->setChecked(true);
}

TargetSetupWidget *TargetSetupPagePrivate::widget(const Id kitId, TargetSetupWidget *fallback) const
{
    return findOr(widgets, fallback, [kitId](const TargetSetupWidget *w) {
        return w->kit() && w->kit()->id() == kitId;
    });
}

void TargetSetupPage::setProjectPath(const FilePath &path)
{
    d->projectPath = path;
    if (!d->projectPath.isEmpty()) {
        QFileInfo fileInfo(QDir::cleanPath(path.toString()));
        QStringList subDirsList = fileInfo.absolutePath().split('/');
        d->headerLabel->setText(Tr::tr("The following kits can be used for project <b>%1</b>:",
                                      "%1: Project name").arg(subDirsList.last()));
    }
    d->headerLabel->setVisible(!d->projectPath.isEmpty());

    if (d->widgetsWereSetUp)
        initializePage();
}

void TargetSetupPage::setProjectImporter(ProjectImporter *importer)
{
    if (importer == d->importer)
        return;

    if (d->widgetsWereSetUp)
        d->reset(); // Reset before changing the importer!

    if (d->importer) {
        disconnect(d->importer, &ProjectImporter::cmakePresetsUpdated,
                   this, &TargetSetupPage::initializePage);
    }


    d->importer = importer;
    d->importWidget->setVisible(d->importer);

    if (d->importer) {
        // FIXME: Needed for the refresh of CMake preset kits created by
        // CMakeProjectImporter
        connect(d->importer, &ProjectImporter::cmakePresetsUpdated,
                this, &TargetSetupPage::initializePage);
    }

    if (d->widgetsWereSetUp)
        initializePage();
}

bool TargetSetupPage::importLineEditHasFocus() const
{
    return d->importWidget->ownsReturnKey();
}

void TargetSetupPagePrivate::setupImports()
{
    if (!importer || projectPath.isEmpty())
        return;

    const FilePaths toImport = importer->importCandidates();
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

    if (importer)
        importer->cleanupKit(k);

    removeWidget(k);
    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPagePrivate::handleKitUpdate(Kit *k)
{
    if (isUpdating())
        return;

    if (importer)
        importer->makePersistent(k);

    const auto newWidgetList = sortedWidgetList();
    if (newWidgetList != widgets) { // Sorting has changed.
        widgets = newWidgetList;
        reLayout();
    }
    updateWidget(widget(k));
    kitSelectionChanged();
    updateVisibility();
}

void TargetSetupPagePrivate::selectAtLeastOneEnabledKit()
{
    if (anyOf(widgets, [](const TargetSetupWidget *w) { return w->isKitSelected(); })) {
        // Something is already selected, we are done.
        return;
    }

    TargetSetupWidget *toCheckWidget = nullptr;

    const Kit *defaultKit = KitManager::defaultKit();

    auto isPreferred = [this](const TargetSetupWidget *w) {
        const Tasks tasks = tasksGenerator(w->kit());
        return w->isValid() && tasks.isEmpty();
    };

    // Use default kit if that is preferred:
    toCheckWidget = findOrDefault(widgets, [defaultKit, isPreferred](const TargetSetupWidget *w) {
        return isPreferred(w) && w->kit() == defaultKit;
    });

    if (!toCheckWidget) {
        // Use the first preferred widget:
        toCheckWidget = findOrDefault(widgets, isPreferred);
    }

    if (!toCheckWidget) {
        // Use default kit if it is enabled:
        toCheckWidget = findOrDefault(widgets, [defaultKit](const TargetSetupWidget *w) {
            return w->isValid() && w->kit() == defaultKit;
        });
    }

    if (!toCheckWidget) {
        // Use the first enabled widget:
        toCheckWidget = findOrDefault(widgets,
                                      [](const TargetSetupWidget *w) { return w->isValid(); });
    }

    if (toCheckWidget) {
        toCheckWidget->setKitSelected(true);

        emit q->completeChanged(); // FIXME: Is this necessary?
    }
}

void TargetSetupPagePrivate::updateVisibility()
{
    // Always show the widgets, the import widget always makes sense to show.
    scrollAreaWidget->setVisible(baseLayout == scrollArea->widget()->layout());
    centralWidget->setVisible(baseLayout == centralWidget->layout());

    const bool hasUsableKits = KitManager::kit([this](const Kit *k) { return isUsable(k); });
    noValidKitLabel->setVisible(!hasUsableKits);
    allKitsCheckBox->setVisible(hasUsableKits);

    emit q->completeChanged();
}

void TargetSetupPagePrivate::reLayout()
{
    removeAdditionalWidgets();
    for (TargetSetupWidget * const w : std::as_const(widgets))
        baseLayout->removeWidget(w);
    for (TargetSetupWidget * const w : std::as_const(widgets))
        baseLayout->addWidget(w);
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
    return sorted(widgets, [](const TargetSetupWidget *w1, const TargetSetupWidget *w2) {
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
    for (const TargetSetupWidget *widget : widgets) {
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
    for (TargetSetupWidget *widget : d->widgets) {
        if (!checked || widget->isValid())
            widget->setKitSelected(checked);
    }
    emit completeChanged();
}

bool TargetSetupPagePrivate::isUpdating() const
{
    return importer && importer->isUpdating();
}

void TargetSetupPagePrivate::import(const FilePath &path, bool silent)
{
    if (!importer)
        return;

    for (const BuildInfo &info : importer->import(path, silent)) {
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
    widgets.erase(std::find(widgets.begin(), widgets.end(), w));
}

TargetSetupWidget *TargetSetupPagePrivate::addWidget(Kit *k)
{
    const auto widget = new TargetSetupWidget(k, projectPath);
    updateWidget(widget);
    connectWidget(widget);
    toggleVisibility(widget);

    // Insert widget, sorted.
    const auto insertionPos = std::find_if(widgets.begin(), widgets.end(),
                                           [k](const TargetSetupWidget *w) {
        return compareKits(k, w->kit());
    });
    const bool addedToEnd = insertionPos == widgets.end();
    widgets.insert(insertionPos, widget);
    if (addedToEnd) {
        removeAdditionalWidgets();
        baseLayout->addWidget(widget);
        addAdditionalWidgets();
    } else {
        reLayout();
    }

    return widget;
}

void TargetSetupPagePrivate::connectWidget(TargetSetupWidget *w)
{
    connect(w, &TargetSetupWidget::selectedToggled,
            this, &TargetSetupPagePrivate::kitSelectionChanged);
    connect(w, &TargetSetupWidget::selectedToggled,
            q, &QWizardPage::completeChanged);
    connect(w, &TargetSetupWidget::validToggled, this, [w, this] { toggleVisibility(w); });
}

void TargetSetupPagePrivate::toggleVisibility(TargetSetupWidget *w)
{
    const bool shouldBeVisible = [w, this] {
        if (!w->isValid() && hideUnsuitableKitsCheckBox->isChecked())
            return false;
        const QString filterText = kitFilterLineEdit->text();
        return filterText.isEmpty()
               || w->kit()->displayName().contains(filterText, Qt::CaseInsensitive);
    }();
    if (shouldBeVisible) {
        if (!w->isVisible()) // Prevent flickering.
            w->show();
    } else {
        w->hide();
    }
}

void TargetSetupPagePrivate::addAdditionalWidgets()
{
    baseLayout->addWidget(importWidget);
    baseLayout->addItem(spacer);
}

void TargetSetupPagePrivate::removeAdditionalWidgets(QLayout *layout)
{
    layout->removeWidget(importWidget);
    layout->removeItem(spacer);
}

void TargetSetupPagePrivate::removeAdditionalWidgets()
{
    removeAdditionalWidgets(baseLayout);
}

void TargetSetupPagePrivate::updateWidget(TargetSetupWidget *widget)
{
    QTC_ASSERT(widget, return );
    widget->update(tasksGenerator);
}

bool TargetSetupPagePrivate::isUsable(const Kit *kit) const
{
    return !containsType(tasksGenerator(kit), Task::Error);
}

bool TargetSetupPage::setupProject(Project *project)
{
    QList<BuildInfo> toSetUp;
    for (TargetSetupWidget *widget : d->widgets) {
        if (!widget->isKitSelected())
            continue;

        Kit *k = widget->kit();

        if (k && d->importer)
            d->importer->makePersistent(k);
        toSetUp << widget->selectedBuildInfoList();
        widget->clearKit();
    }

    project->setup(toSetUp);
    toSetUp.clear();

    // Only reset now that toSetUp has been cleared!
    d->reset();

    Target *activeTarget = nullptr;
    if (d->importer)
        activeTarget = d->importer->preferredTarget(project->targets());
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
    QLayout *oldBaseLayout = baseLayout;
    baseLayout = b ? scrollArea->widget()->layout() : centralWidget->layout();
    if (oldBaseLayout == baseLayout)
        return;
    scrollAreaWidget->setVisible(b);
    centralWidget->setVisible(!b);

    if (oldBaseLayout)
        removeAdditionalWidgets(oldBaseLayout);
    addAdditionalWidgets();
}

} // namespace ProjectExplorer
