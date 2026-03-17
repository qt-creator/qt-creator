// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "systemsettings.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "editormanager/editormanager_p.h"
#include "dialogs/ioptionspage.h"
#include "fileutils.h"
#include "icore.h"
#include "vcsmanager.h"

#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/checkablemessagebox.h>
#include <utils/crashreporting.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/guiutils.h>
#include <utils/hostosinfo.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/terminalcommand.h>
#include <utils/treemodel.h>

#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>

using namespace Utils;
using namespace Layouting;

namespace Core::Internal {

static constexpr bool hasDBusFileManager =
#ifdef QTC_SUPPORT_DBUSFILEMANAGER
    true;
#else
    false;
#endif

SystemSettings &systemSettings()
{
    static SystemSettings theSettings;
    return theSettings;
}

static NameValueDictionary defaultEnvVarSeparators()
{
    return NameValueDictionary(
        NameValuePairs{
            std::make_pair<QString, QString>("CFLAGS", " "),
            std::make_pair<QString, QString>("CXXFLAGS", " ")});
}

static QString fileBrowserHelpText()
{
    return Tr::tr(
        "<table border=1 cellspacing=0 cellpadding=3>"
        "<tr><th>Variable</th><th>Expands to</th></tr>"
        "<tr><td>%d</td><td>directory of current file</td></tr>"
        "<tr><td>%f</td><td>file name (with full path)</td></tr>"
        "<tr><td>%n</td><td>file name (without path)</td></tr>"
        "<tr><td>%%</td><td>%</td></tr>"
        "</table>");
}

void EnvChangeAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto label = createLabel();
    if (label)
        parent.addItem(label);

    auto changesLabel = new ElidingLabel();
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    changesLabel->setSizePolicy(sizePolicy);
    changesLabel->setElideMode(Qt::ElideRight);
    auto updateChangesLabel = [this, changesLabel]() {
        const EnvironmentItems items = volatileValue().itemsFromUser();
        changesLabel->setText(EnvironmentItem::toShortSummary(items, false));
    };
    updateChangesLabel();
    connect(this, &EnvChangeAspect::volatileValueChanged, this, updateChangesLabel);
    parent.addItem(changesLabel);

    QPushButton *changeButton = new QPushButton(Tr::tr("Change..."));
    changeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    parent.addItem(changeButton);
    connect(changeButton, &QPushButton::clicked, this, [changeButton, this]() {
        std::optional<EnvironmentChanges> changes
            = runEnvironmentItemsDialog(changeButton, volatileValue());
        if (changes)
            setVolatileValue(*changes);
    });
}

SystemSettings::SystemSettings()
{
    setAutoApply(false);

    environmentChangesAspect.setLabelText("Environment:");
    environmentChangesAspect.setSettingsKey(kEnvironmentChanges);

    terminalCommand.setLabelText(Tr::tr("Terminal:"));

    const auto updateSystemEnv = [this, startupEnv = Environment::systemEnvironment()] {
        Environment systemEnv = startupEnv;
        environmentChangesAspect.value().modifyEnvironment(systemEnv, nullptr);

        Environment::setSystemEnvironment(systemEnv);

        if (ICore::instance())
            ICore::instance()->systemEnvironmentChanged();
    };

    connect(&environmentChangesAspect, &EnvChangeAspect::changed, this, updateSystemEnv);

    Qt::Alignment lfa = Qt::Alignment(qApp->style()->styleHint(QStyle::SH_FormLayoutFormAlignment));

    BoolAspect::LabelPlacement boolAspectLabelPlacement
        = lfa & Qt::AlignHCenter ? BoolAspect::LabelPlacement::AtCheckBox
                                 : BoolAspect::LabelPlacement::Compact;

    envVarSeparatorAspect.setLabelText(Tr::tr("Variable separators:"));
    envVarSeparatorAspect.setSettingsKey(kEnvVarSeparators);
    envVarSeparatorAspect.setDefaultValue(defaultEnvVarSeparators().toStringList());

    useDbusFileManagers.setSettingsKey("General/SupportDbusFileManagers");
    useDbusFileManagers.setDefaultValue(true);
    useDbusFileManagers.setLabelPlacement(boolAspectLabelPlacement);
    useDbusFileManagers.setLabelText(Tr::tr("Use freedesktop.org file manager D-Bus interface"));
    useDbusFileManagers.setToolTip(
        Tr::tr(
            "Uses the <a href=\"%1\">freedesktop.org D-Bus interface</a> for <i>Open in File "
            "Manager</i>, if available. Otherwise falls back to the \"External file browser\" "
            "above.")
            .arg("https://freedesktop.org/wiki/Specifications/file-manager-interface"));

    externalFileBrowser.setSettingsKey("General/FileBrowser");
    externalFileBrowser.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
    externalFileBrowser.setDefaultValue("xdg-open %d");
    externalFileBrowser.setLabelText(Tr::tr("External file browser:"));
    externalFileBrowser.setToolTip(
        Tr::tr(
            "Command used for <i>Open in File Manager</i> if the freedesktop.org D-Bus interface "
            "is not available. The command can contain the following variables:\n")
        + fileBrowserHelpText());

    patchCommand.setSettingsKey("General/PatchCommand");
    patchCommand.setDefaultValue("patch");
    patchCommand.setExpectedKind(PathChooser::ExistingCommand);
    patchCommand.setHistoryCompleter("General.PatchCommand.History");
    patchCommand.setLabelText(Tr::tr("Patch command:"));
    patchCommand.setToolTip(Tr::tr("Command used for reverting diff chunks."));

    autoSaveModifiedFiles.setSettingsKey("EditorManager/AutoSaveEnabled");
    autoSaveModifiedFiles.setDefaultValue(true);
    autoSaveModifiedFiles.setLabelText(Tr::tr("Auto-save modified files"));
    autoSaveModifiedFiles.setLabelPlacement(boolAspectLabelPlacement);
    autoSaveModifiedFiles.setToolTip(
        Tr::tr("Automatically creates temporary copies of modified files. "
               "If %1 is restarted after a crash or power failure, it asks whether to "
               "recover the auto-saved content.")
            .arg(QGuiApplication::applicationDisplayName()));

    autoSaveInterval.setSettingsKey("EditorManager/AutoSaveInterval");
    autoSaveInterval.setSuffix(Tr::tr("min"));
    autoSaveInterval.setRange(1, 1000000);
    autoSaveInterval.setDefaultValue(5);
    autoSaveInterval.setLabelText(Tr::tr("Interval:"));

    autoSaveAfterRefactoring.setSettingsKey("EditorManager/AutoSaveAfterRefactoring");
    autoSaveAfterRefactoring.setDefaultValue(true);
    autoSaveAfterRefactoring.setLabelPlacement(boolAspectLabelPlacement);
    autoSaveAfterRefactoring.setLabelText(Tr::tr("Auto-save files after refactoring"));
    autoSaveAfterRefactoring.setToolTip(
        Tr::tr("Automatically saves all open files affected by a refactoring operation,\n"
               "provided they were unmodified before the refactoring."));

    autoSuspendEnabled.setSettingsKey("EditorManager/AutoSuspendEnabled");
    autoSuspendEnabled.setDefaultValue(true);
    autoSuspendEnabled.setLabelText(Tr::tr("Auto-suspend unmodified files"));
    autoSuspendEnabled.setLabelPlacement(boolAspectLabelPlacement);
    autoSuspendEnabled.setToolTip(
        Tr::tr("Automatically free resources of old documents that are not visible and not "
               "modified. They stay visible in the list of open documents."));

    enableCrashReports.setSettingsKey(Utils::crashReportSettingsKey());
    enableCrashReports.setLabelPlacement(boolAspectLabelPlacement);
    enableCrashReports.setLabelText(Tr::tr("Enable crash reporting"));
    enableCrashReports.setToolTip(
        "<p>"
        + Tr::tr(
            "Allow crashes to be automatically reported. Collected reports are "
            "used for the sole purpose of fixing bugs.")
        + "</p><p>"
        + Tr::tr("Crash reports are saved in \"%1\".").arg(appInfo().crashReports.toUserOutput()));
    enableCrashReports.setDefaultValue(false);
    enableCrashReports.setLabelPlacement(boolAspectLabelPlacement);

    autoSuspendMinDocumentCount.setSettingsKey("EditorManager/AutoSuspendMinDocuments");
    autoSuspendMinDocumentCount.setRange(1, 500);
    autoSuspendMinDocumentCount.setDefaultValue(10);
    autoSuspendMinDocumentCount.setLabelText(Tr::tr("Files to keep open:"));
    autoSuspendMinDocumentCount.setToolTip(
        Tr::tr("Minimum number of open documents that should be kept in memory. Increasing this "
           "number will lead to greater resource usage when not manually closing documents."));

    warnBeforeOpeningBigFiles.setSettingsKey("EditorManager/WarnBeforeOpeningBigTextFiles");
    warnBeforeOpeningBigFiles.setDefaultValue(true);
    warnBeforeOpeningBigFiles.setLabelPlacement(boolAspectLabelPlacement);
    warnBeforeOpeningBigFiles.setLabelText(Tr::tr("Warn before opening text files greater than"));

    bigFileSizeLimitInMB.setSettingsKey("EditorManager/BigTextFileSizeLimitInMB");
    bigFileSizeLimitInMB.setSuffix(Tr::tr("MB"));
    bigFileSizeLimitInMB.setRange(1, 500);
    bigFileSizeLimitInMB.setDefaultValue(5);

    maxRecentFiles.setSettingsKey("EditorManager/MaxRecentFiles");
    maxRecentFiles.setRange(1, 99);
    maxRecentFiles.setDefaultValue(8);
    maxRecentFiles.setLabelText(Tr::tr("Number of \"Recent Files\":"));

    reloadSetting.setSettingsKey("EditorManager/ReloadBehavior");
    reloadSetting.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    reloadSetting.addOption(Tr::tr("Always Ask"));
    reloadSetting.addOption(Tr::tr("Reload All Unchanged Editors"));
    reloadSetting.addOption(Tr::tr("Ignore Modifications"));
    reloadSetting.setDefaultValue(IDocument::ReloadUnmodified);
    reloadSetting.setLabelText(Tr::tr("When files are externally modified:"));

    askBeforeExit.setSettingsKey("AskBeforeExit");
    askBeforeExit.setLabelText(Tr::tr("Ask for confirmation before exiting"));
    askBeforeExit.setLabelPlacement(boolAspectLabelPlacement);

    readSettings();
    updateSystemEnv();

    autoSaveInterval.setEnabler(&autoSaveModifiedFiles);
    autoSuspendMinDocumentCount.setEnabler(&autoSuspendEnabled);
    bigFileSizeLimitInMB.setEnabler(&warnBeforeOpeningBigFiles);

    autoSaveModifiedFiles.addOnChanged(this, &EditorManagerPrivate::updateAutoSave);
    autoSaveInterval.addOnChanged(this, &EditorManagerPrivate::updateAutoSave);
    enableCrashReports.addOnChanged(this, [] {
        Utils::setCrashReportingEnabled(systemSettings().enableCrashReports());
    });

    setLayouter([this]() -> Layouting::Layout {
        using namespace Layouting;
        // clang-format off
        return Form {
            environmentChangesAspect, br,
            envVarSeparatorAspect, br,
            If (HostOsInfo::isAnyUnixHost()) >> Then {
                terminalCommand, br,
            },
            If (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()) >> Then {
                externalFileBrowser, br,
            },
            If (hasDBusFileManager) >> Then {
                useDbusFileManagers, br,
            },
            patchCommand, br,
            maxRecentFiles, br,
            reloadSetting, br,
            autoSaveModifiedFiles, Row { autoSaveInterval }, st, br,
            autoSaveAfterRefactoring, br,
            autoSuspendEnabled, Row { autoSuspendMinDocumentCount}, st, br,
            warnBeforeOpeningBigFiles, Row { bigFileSizeLimitInMB }, st, br,
            askBeforeExit, br,
            If (isCrashReportingAvailable()) >> Then {
                enableCrashReports, br,
            },
        };
        // clang-format on
    });
}

// SystemSettingsPage
class SystemSettingsPage final : public IOptionsPage
{
public:
    SystemSettingsPage()
    {
        setId(Constants::SETTINGS_ID_SYSTEM);
        setDisplayName(Tr::tr("System"));
        setCategory(Constants::SETTINGS_CATEGORY_CORE);
        setSettingsProvider([] { return &systemSettings(); });
    }
};

static void appendVscPathsToPatchCommandPath()
{
    Environment env;
    env.appendToPath(VcsManager::additionalToolsPath());
    systemSettings().patchCommand.setEnvironment(env);
}

void SystemSettings::delayedInitialize()
{
    // TODO: This is a bit of a hack to get the VSC paths into the patch command's environment,
    // but it needs to be done after all plugins have been loaded to ensure that the VCS plugin has
    // had a chance to add its additional tools path. We should look into a cleaner way to handle
    // this in the future.
    connect(
        VcsManager::instance(),
        &VcsManager::configurationChanged,
        this,
        appendVscPathsToPatchCommandPath);
    connect(
        ICore::instance(), &ICore::systemEnvironmentChanged, this, appendVscPathsToPatchCommandPath);
    appendVscPathsToPatchCommandPath();
}

const SystemSettingsPage settingsPage;

} // namespace Core::Internal
