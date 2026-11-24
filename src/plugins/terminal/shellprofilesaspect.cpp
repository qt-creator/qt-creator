// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "shellprofilesaspect.h"

#include "terminaltr.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stylehelper.h>

#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QUuid>

using namespace Utils;

namespace Terminal::Internal {

const char profilesKey[] = "Profiles";
const char hiddenKey[] = "Hidden";
const char detectedOrderKey[] = "DetectedOrder";
const char defaultKey[] = "Default";

const char legacyShellPathKey[] = "ShellPath";
const char legacyShellArgumentsKey[] = "ShellArguments";

const char idKey[] = "Id";
const char nameKey[] = "Name";
const char executableKey[] = "Executable";
const char argumentsKey[] = "Arguments";
const char environmentKey[] = "Environment";
const char workingDirectoryKey[] = "WorkingDirectory";
const char iconPathKey[] = "IconPath";

/*!
    Returns every profile: the detected ones first, in the order the user gave
    them, followed by the manual ones.

    Detected profiles the user never moved keep their detection order and come
    after the ones that were.
*/
QList<ShellProfile> orderedShellProfiles(const ShellProfileSettings &settings,
                                         const QList<ShellProfile> &detected)
{
    QList<ShellProfile> sortedDetected = detected;
    const auto rank = [&settings](const ShellProfile &profile) {
        const qsizetype index = settings.detectedOrder.indexOf(profile.id);
        return index < 0 ? settings.detectedOrder.size() : index;
    };
    std::stable_sort(sortedDetected.begin(),
                     sortedDetected.end(),
                     [&rank](const ShellProfile &a, const ShellProfile &b) {
                         return rank(a) < rank(b);
                     });

    QList<ShellProfile> result = sortedDetected;
    for (ShellProfile profile : settings.manualProfiles) {
        profile.isDetected = false;
        result << profile;
    }
    return result;
}

QList<ShellProfile> visibleShellProfiles(const ShellProfileSettings &settings,
                                         const QList<ShellProfile> &detected)
{
    return Utils::filtered(orderedShellProfiles(settings, detected),
                           [&settings](const ShellProfile &profile) {
                               return !profile.isDetected
                                      || !settings.hiddenProfiles.contains(profile.id);
                           });
}

/*!
    Returns the profile a new terminal starts with: the one the user chose, or,
    when that is gone or hidden, the first visible one. With nothing visible it
    falls back to \a system, the first of the \a detected profiles.

    Looking for the installed shells can take long, so \a detected is only
    called when neither a manual profile nor \a system is the answer.
*/
ShellProfile defaultShellProfile(const ShellProfileSettings &settings,
                                 const ShellProfile &system,
                                 const std::function<QList<ShellProfile>()> &detected)
{
    for (ShellProfile profile : settings.manualProfiles) {
        if (profile.id == settings.defaultProfile) {
            profile.isDetected = false;
            return profile;
        }
    }
    if (!settings.hiddenProfiles.contains(system.id)) {
        if (settings.defaultProfile == system.id)
            return system;
        const bool systemIsFirst = settings.detectedOrder.isEmpty()
                                   || settings.detectedOrder.first() == system.id;
        if (settings.defaultProfile.isEmpty() && systemIsFirst)
            return system;
    }

    const QList<ShellProfile> visible = visibleShellProfiles(settings, detected());
    for (const ShellProfile &profile : visible) {
        if (profile.id == settings.defaultProfile)
            return profile;
    }
    if (!visible.isEmpty())
        return visible.first();
    return system;
}

QVariant shellProfileSettingsToVariant(const ShellProfileSettings &settings)
{
    QVariantList profiles;
    for (const ShellProfile &profile : settings.manualProfiles) {
        profiles << QVariantMap{
            {idKey, profile.id},
            {nameKey, profile.name},
            {executableKey, profile.executable.toSettings()},
            {argumentsKey, profile.arguments},
            {environmentKey, profile.environmentChanges.toVariant()},
            {workingDirectoryKey, profile.workingDirectory.toSettings()},
            {iconPathKey, profile.iconPath.toSettings()},
        };
    }
    return QVariantMap{
        {profilesKey, profiles},
        {hiddenKey, settings.hiddenProfiles},
        {detectedOrderKey, settings.detectedOrder},
        {defaultKey, settings.defaultProfile},
    };
}

ShellProfileSettings shellProfileSettingsFromVariant(const QVariant &variant)
{
    const QVariantMap map = variant.toMap();
    ShellProfileSettings settings;
    for (const QVariant &entry : map.value(profilesKey).toList()) {
        const QVariantMap profileMap = entry.toMap();
        ShellProfile profile;
        profile.id = profileMap.value(idKey).toString();
        if (profile.id.isEmpty())
            continue;
        profile.name = profileMap.value(nameKey).toString();
        profile.executable = FilePath::fromSettings(profileMap.value(executableKey));
        profile.arguments = profileMap.value(argumentsKey).toString();
        profile.environmentChanges = EnvironmentChanges::createFromVariant(
            profileMap.value(environmentKey));
        profile.workingDirectory = FilePath::fromSettings(profileMap.value(workingDirectoryKey));
        profile.iconPath = FilePath::fromSettings(profileMap.value(iconPathKey));
        settings.manualProfiles << profile;
    }
    settings.hiddenProfiles = map.value(hiddenKey).toStringList();
    settings.detectedOrder = map.value(detectedOrderKey).toStringList();
    settings.defaultProfile = map.value(defaultKey).toString();
    return settings;
}

// Does not read the file: for a script, that would mean running it.
static QString environmentSummary(const EnvironmentChanges &changes)
{
    QStringList parts;
    if (!changes.itemsFromUser().isEmpty())
        parts << EnvironmentItem::toShortSummary(changes.itemsFromUser(), false);
    if (!changes.file().isEmpty())
        parts << Tr::tr("Variables from %1").arg(changes.file().toUserOutput());
    return parts.isEmpty() ? Tr::tr("No changes to apply.") : parts.join("; ");
}

class ShellProfilesWidget final : public QWidget
{
public:
    explicit ShellProfilesWidget(ShellProfilesAspect *aspect);

private:
    enum { IdRole = Qt::UserRole };

    void rebuild(const QString &idToSelect);
    void refreshTexts();
    void updateDetails();
    void updateButtons();
    void push();

    QString currentId() const;
    std::optional<ShellProfile> currentProfile() const;
    ShellProfile *currentManualProfile();
    QTreeWidgetItem *itemForId(const QString &id) const;

    void addProfile();
    void cloneProfile();
    void removeProfile();
    void makeDefault();
    void move(int offset);
    void setHidden(const QString &id, bool hidden);
    void editEnvironment();

    ShellProfilesAspect *m_aspect;
    ShellProfileSettings m_settings;
    Guard m_updating;

    QTreeWidget *m_tree;
    QTreeWidgetItem *m_detectedRoot = nullptr;
    QTreeWidgetItem *m_manualRoot = nullptr;

    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_removeButton;
    QPushButton *m_defaultButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;

    QWidget *m_details;
    QLineEdit *m_name;
    PathChooser *m_executable;
    QLineEdit *m_arguments;
    PathChooser *m_workingDirectory;
    QLabel *m_environment;
    QPushButton *m_environmentButton;
};

ShellProfilesWidget::ShellProfilesWidget(ShellProfilesAspect *aspect)
    : m_aspect(aspect)
    , m_settings(aspect->volatileValue())
    , m_tree(new QTreeWidget)
    , m_addButton(new QPushButton(Tr::tr("Add")))
    , m_cloneButton(new QPushButton(Tr::tr("Clone")))
    , m_removeButton(new QPushButton(Tr::tr("Remove")))
    , m_defaultButton(new QPushButton(Tr::tr("Make Default")))
    , m_upButton(new QPushButton(Tr::tr("Move Up")))
    , m_downButton(new QPushButton(Tr::tr("Move Down")))
    , m_details(new QWidget)
    , m_name(new QLineEdit)
    , m_executable(new PathChooser)
    , m_arguments(new QLineEdit)
    , m_workingDirectory(new PathChooser)
    , m_environment(new QLabel)
    , m_environmentButton(new QPushButton(Tr::tr("Change...")))
{
    m_tree->setHeaderLabels({Tr::tr("Name"), Tr::tr("Command"), Tr::tr("Working Directory")});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);

    m_executable->setExpectedKind(PathChooserKind::ExistingCommand);
    m_executable->setHistoryCompleter("Terminal.Shell.History");
    m_workingDirectory->setExpectedKind(PathChooserKind::ExistingDirectory);
    m_workingDirectory->setPlaceholderText(Tr::tr("Project or current directory"));
    m_workingDirectory->setToolTip(
        Tr::tr("The directory the shell starts in. Leave empty to start in the directory of "
               "the startup project."));
    m_environment->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_environment->setWordWrap(true);
    m_environment->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_defaultButton->setToolTip(Tr::tr("Uses this shell for new terminals."));

    using namespace Layouting;
    // clang-format off
    Form {
        Tr::tr("Name:"), m_name, br,
        Tr::tr("Executable:"), m_executable, br,
        Tr::tr("Arguments:"), m_arguments, br,
        Tr::tr("Working directory:"), m_workingDirectory, br,
        Tr::tr("Environment:"), Row { m_environment, m_environmentButton }, br,
        noMargin,
    }.attachTo(m_details);

    Column {
        Row {
            m_tree,
            Column {
                m_addButton,
                m_cloneButton,
                m_removeButton,
                Space(StyleHelper::SpacingTokens::GapVL),
                m_defaultButton,
                Space(StyleHelper::SpacingTokens::GapVL),
                m_upButton,
                m_downButton,
                st,
            },
        },
        m_details,
        noMargin,
    }.attachTo(this);
    // clang-format on

    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this] {
        updateDetails();
        updateButtons();
    });
    connect(m_tree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column) {
        if (m_updating.isLocked() || column != 0 || !(item->flags() & Qt::ItemIsUserCheckable))
            return;
        setHidden(item->data(0, IdRole).toString(), item->checkState(0) != Qt::Checked);
    });

    connect(m_addButton, &QPushButton::clicked, this, &ShellProfilesWidget::addProfile);
    connect(m_cloneButton, &QPushButton::clicked, this, &ShellProfilesWidget::cloneProfile);
    connect(m_removeButton, &QPushButton::clicked, this, &ShellProfilesWidget::removeProfile);
    connect(m_defaultButton, &QPushButton::clicked, this, &ShellProfilesWidget::makeDefault);
    connect(m_upButton, &QPushButton::clicked, this, [this] { move(-1); });
    connect(m_downButton, &QPushButton::clicked, this, [this] { move(1); });
    connect(m_environmentButton, &QPushButton::clicked, this, &ShellProfilesWidget::editEnvironment);

    const auto editField = [this](const std::function<void(ShellProfile &)> &edit) {
        if (m_updating.isLocked())
            return;
        ShellProfile *profile = currentManualProfile();
        QTC_ASSERT(profile, return);
        edit(*profile);
        push();
        refreshTexts();
    };
    connect(m_name, &QLineEdit::textEdited, this, [editField](const QString &text) {
        editField([&text](ShellProfile &profile) { profile.name = text; });
    });
    connect(m_executable, &PathChooser::textChanged, this, [this, editField] {
        editField([this](ShellProfile &profile) {
            profile.executable = m_executable->unexpandedFilePath();
        });
    });
    connect(m_arguments, &QLineEdit::textEdited, this, [editField](const QString &text) {
        editField([&text](ShellProfile &profile) { profile.arguments = text; });
    });
    connect(m_workingDirectory, &PathChooser::textChanged, this, [this, editField] {
        editField([this](ShellProfile &profile) {
            profile.workingDirectory = m_workingDirectory->unexpandedFilePath();
        });
    });

    m_aspect->addOnVolatileValueChanged(this, [this] {
        if (m_updating.isLocked())
            return;
        m_settings = m_aspect->volatileValue();
        rebuild(currentId());
    });

    rebuild({});
}

void ShellProfilesWidget::push()
{
    const GuardLocker locker(m_updating);
    m_aspect->setVolatileValue(m_settings);
}

QString ShellProfilesWidget::currentId() const
{
    const QTreeWidgetItem *item = m_tree->currentItem();
    return item ? item->data(0, IdRole).toString() : QString();
}

std::optional<ShellProfile> ShellProfilesWidget::currentProfile() const
{
    const QString id = currentId();
    if (id.isEmpty())
        return {};
    for (const ShellProfile &profile : orderedShellProfiles(m_settings, detectedShellProfiles())) {
        if (profile.id == id)
            return profile;
    }
    return {};
}

ShellProfile *ShellProfilesWidget::currentManualProfile()
{
    const QString id = currentId();
    for (ShellProfile &profile : m_settings.manualProfiles) {
        if (profile.id == id)
            return &profile;
    }
    return nullptr;
}

QTreeWidgetItem *ShellProfilesWidget::itemForId(const QString &id) const
{
    for (QTreeWidgetItem *root : {m_detectedRoot, m_manualRoot}) {
        for (int i = 0; i < root->childCount(); ++i) {
            if (root->child(i)->data(0, IdRole).toString() == id)
                return root->child(i);
        }
    }
    return nullptr;
}

void ShellProfilesWidget::rebuild(const QString &idToSelect)
{
    {
        const GuardLocker locker(m_updating);
        m_tree->clear();

        m_detectedRoot = new QTreeWidgetItem(m_tree, {Tr::tr("Auto-detected")});
        m_manualRoot = new QTreeWidgetItem(m_tree, {Tr::tr("Manual")});
        for (QTreeWidgetItem *root : {m_detectedRoot, m_manualRoot}) {
            root->setFlags(Qt::ItemIsEnabled);
            root->setFirstColumnSpanned(true);
        }

        for (const ShellProfile &profile :
             orderedShellProfiles(m_settings, detectedShellProfiles())) {
            auto item = new QTreeWidgetItem(profile.isDetected ? m_detectedRoot : m_manualRoot);
            item->setData(0, IdRole, profile.id);
            Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            if (profile.isDetected) {
                flags |= Qt::ItemIsUserCheckable;
                item->setToolTip(0, Tr::tr("Uncheck to hide this shell from the menu of the "
                                           "New Terminal button."));
            }
            item->setFlags(flags);
        }
        m_tree->expandAll();
        refreshTexts();

        m_tree->setCurrentItem(itemForId(idToSelect));
    }
    updateDetails();
    updateButtons();
}

void ShellProfilesWidget::refreshTexts()
{
    const GuardLocker locker(m_updating);
    const QList<ShellProfile> profiles = orderedShellProfiles(m_settings, detectedShellProfiles());
    const QString defaultId
        = defaultShellProfile(m_settings, systemShellProfile(), &detectedShellProfiles).id;
    for (const ShellProfile &profile : profiles) {
        QTreeWidgetItem *item = itemForId(profile.id);
        QTC_ASSERT(item, continue);
        item->setText(0,
                      profile.id == defaultId ? Tr::tr("%1 (default)").arg(profile.name)
                                              : profile.name);
        item->setText(1, profile.unexpandedCommandLine().toUserOutput());
        item->setText(2, profile.workingDirectory.toUserOutput());
        if (profile.isDetected) {
            item->setCheckState(0,
                                m_settings.hiddenProfiles.contains(profile.id) ? Qt::Unchecked
                                                                               : Qt::Checked);
        }
    }
}

void ShellProfilesWidget::updateDetails()
{
    const GuardLocker locker(m_updating);
    const std::optional<ShellProfile> profile = currentProfile();
    m_details->setEnabled(profile.has_value());

    const ShellProfile shown = profile.value_or(ShellProfile{});
    m_name->setText(shown.name);
    m_executable->setFilePath(shown.executable);
    m_arguments->setText(shown.arguments);
    m_workingDirectory->setFilePath(shown.workingDirectory);
    m_environment->setText(environmentSummary(shown.environmentChanges));

    const bool editable = profile && !profile->isDetected;
    m_name->setReadOnly(!editable);
    m_executable->setReadOnly(!editable);
    m_arguments->setReadOnly(!editable);
    m_workingDirectory->setReadOnly(!editable);
    m_environmentButton->setEnabled(editable);
}

void ShellProfilesWidget::updateButtons()
{
    const std::optional<ShellProfile> profile = currentProfile();
    QTreeWidgetItem *item = m_tree->currentItem();
    const QTreeWidgetItem *root = item ? item->parent() : nullptr;
    const int row = root ? root->indexOfChild(item) : -1;

    m_cloneButton->setEnabled(profile.has_value());
    m_removeButton->setEnabled(profile && !profile->isDetected);
    m_defaultButton->setEnabled(
        profile
        && profile->id
               != defaultShellProfile(m_settings, systemShellProfile(), &detectedShellProfiles).id);
    m_upButton->setEnabled(root && row > 0);
    m_downButton->setEnabled(root && row >= 0 && row < root->childCount() - 1);
}

void ShellProfilesWidget::addProfile()
{
    const ShellProfile system = systemShellProfile();
    ShellProfile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = Tr::tr("New Shell");
    profile.executable = system.executable;
    profile.arguments = system.arguments;
    m_settings.manualProfiles << profile;
    push();
    rebuild(profile.id);
    m_name->setFocus();
    m_name->selectAll();
}

void ShellProfilesWidget::cloneProfile()
{
    std::optional<ShellProfile> profile = currentProfile();
    QTC_ASSERT(profile, return);
    profile->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile->name = Tr::tr("Clone of %1").arg(profile->name);
    profile->isDetected = false;
    m_settings.manualProfiles << *profile;
    push();
    rebuild(profile->id);
    m_name->setFocus();
    m_name->selectAll();
}

void ShellProfilesWidget::removeProfile()
{
    const QString id = currentId();
    const qsizetype index = Utils::indexOf(m_settings.manualProfiles,
                                           Utils::equal(&ShellProfile::id, id));
    QTC_ASSERT(index >= 0, return);
    m_settings.manualProfiles.removeAt(index);
    if (m_settings.defaultProfile == id)
        m_settings.defaultProfile.clear();
    push();

    const QList<ShellProfile> &remaining = m_settings.manualProfiles;
    rebuild(remaining.isEmpty() ? QString() : remaining.at(std::min(index, remaining.size() - 1)).id);
}

void ShellProfilesWidget::makeDefault()
{
    const QString id = currentId();
    QTC_ASSERT(!id.isEmpty(), return);
    m_settings.defaultProfile = id;
    m_settings.hiddenProfiles.removeAll(id);
    push();
    refreshTexts();
    updateButtons();
}

void ShellProfilesWidget::move(int offset)
{
    const std::optional<ShellProfile> profile = currentProfile();
    QTC_ASSERT(profile, return);

    if (profile->isDetected) {
        QStringList order = Utils::transform(
            Utils::filtered(orderedShellProfiles(m_settings, detectedShellProfiles()),
                            &ShellProfile::isDetected),
            &ShellProfile::id);
        const qsizetype from = order.indexOf(profile->id);
        const qsizetype to = from + offset;
        QTC_ASSERT(from >= 0 && to >= 0 && to < order.size(), return);
        order.move(from, to);
        m_settings.detectedOrder = order;
    } else {
        const qsizetype from = Utils::indexOf(m_settings.manualProfiles,
                                              Utils::equal(&ShellProfile::id, profile->id));
        const qsizetype to = from + offset;
        QTC_ASSERT(from >= 0 && to >= 0 && to < m_settings.manualProfiles.size(), return);
        m_settings.manualProfiles.move(from, to);
    }
    push();
    rebuild(profile->id);
}

void ShellProfilesWidget::setHidden(const QString &id, bool hidden)
{
    m_settings.hiddenProfiles.removeAll(id);
    if (hidden)
        m_settings.hiddenProfiles << id;
    push();
    refreshTexts();
    updateButtons();
}

void ShellProfilesWidget::editEnvironment()
{
    ShellProfile *profile = currentManualProfile();
    QTC_ASSERT(profile, return);
    const std::optional<EnvironmentChanges> changes
        = runEnvironmentItemsDialog(this,
                                    profile->environmentChanges,
                                    {},
                                    {},
                                    Tr::tr("Edit Shell Environment"));
    if (!changes)
        return;
    profile->environmentChanges = *changes;
    push();
    updateDetails();
}

ShellProfilesAspect::ShellProfilesAspect(AspectContainer *container)
    : TypedAspect(container)
{}

/*!
    Returns the profiles to offer in the menu of the New Terminal button, in
    the order the user gave them.
*/
QList<ShellProfile> ShellProfilesAspect::profiles() const
{
    return visibleShellProfiles(value(), detectedShellProfiles());
}

ShellProfile ShellProfilesAspect::defaultProfile() const
{
    return defaultShellProfile(value(), systemShellProfile(), &detectedShellProfiles);
}

void ShellProfilesAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    parent.addItem(createSubWidget<ShellProfilesWidget>(this));
}

void ShellProfilesAspect::writeSettings() const
{
    TypedAspect::writeSettings();
    // Once the profiles are saved, the single shell they replaced is gone,
    // even when the profiles equal the default and are therefore not stored.
    if (!settingsKey().isEmpty()) {
        userSettings().remove(legacyShellPathKey);
        userSettings().remove(legacyShellArgumentsKey);
    }
}

QVariant ShellProfilesAspect::toSettingsValue(const QVariant &valueToSave) const
{
    return shellProfileSettingsToVariant(valueToSave.value<ShellProfileSettings>());
}

QVariant ShellProfilesAspect::fromSettingsValue(const QVariant &savedValue) const
{
    return QVariant::fromValue(shellProfileSettingsFromVariant(savedValue));
}

/*!
    Before shell profiles, a single shell was configured by "ShellPath" and
    "ShellArguments" in \a group. Turns such a shell into a manual profile of
    \a shellProfiles that is the default.

    The old keys stay until the profiles are saved, so that an older version
    still finds the shell.
*/
void migrateLegacyShellSetting(ShellProfilesAspect &shellProfiles, const Key &group)
{
    const QtcSettings &settings = userSettings();
    if (settings.contains(group + '/' + shellProfiles.settingsKey()))
        return;
    const QVariant path = settings.value(group + '/' + Key(legacyShellPathKey));
    const QVariant arguments = settings.value(group + '/' + Key(legacyShellArgumentsKey));
    if (!path.isValid() && !arguments.isValid())
        return;

    const ShellProfile system = systemShellProfile();
    ShellProfile profile;
    profile.id = "Migrated";
    profile.name = Tr::tr("Configured Shell");
    profile.executable = path.isValid() ? FilePath::fromSettings(path) : system.executable;
    profile.arguments = arguments.isValid() ? arguments.toString() : system.arguments;

    ShellProfileSettings value;
    value.manualProfiles << profile;
    value.defaultProfile = profile.id;
    shellProfiles.setValue(value, BaseAspect::BeQuiet);
}

} // namespace Terminal::Internal
