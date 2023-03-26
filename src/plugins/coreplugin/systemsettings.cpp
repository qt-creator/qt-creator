// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "systemsettings.h"

#include "coreconstants.h"
#include "coreplugin.h"
#include "coreplugintr.h"
#include "editormanager/editormanager_p.h"
#include "dialogs/restartdialog.h"
#include "fileutils.h"
#include "icore.h"
#include "iversioncontrol.h"
#include "mainwindow.h"
#include "patchtool.h"
#include "vcsmanager.h"

#include <app/app_version.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/terminalcommand.h>
#include <utils/unixutils.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QToolButton>

using namespace Utils;
using namespace Layouting;

namespace Core {
namespace Internal {

#ifdef ENABLE_CRASHPAD
const char crashReportingEnabledKey[] = "CrashReportingEnabled";
const char showCrashButtonKey[] = "ShowCrashButton";

// TODO: move to somewhere in Utils
static QString formatSize(qint64 size)
{
    QStringList units {Tr::tr("Bytes"), Tr::tr("KB"), Tr::tr("MB"),
                       Tr::tr("GB"), Tr::tr("TB")};
    double outputSize = size;
    int i;
    for (i = 0; i < units.size() - 1; ++i) {
        if (outputSize < 1024)
            break;
        outputSize /= 1024;
    }
    return i == 0 ? QString("%0 %1").arg(outputSize).arg(units[i]) // Bytes
                  : QString("%0 %1").arg(outputSize, 0, 'f', 2).arg(units[i]); // KB, MB, GB, TB
}
#endif // ENABLE_CRASHPAD

class SystemSettingsWidget : public IOptionsPageWidget
{
public:
    SystemSettingsWidget()
        : m_fileSystemCaseSensitivityChooser(new QComboBox)
        , m_autoSuspendMinDocumentCount(new QSpinBox)
        , m_externalFileBrowserEdit(new QLineEdit)
        , m_autoSuspendCheckBox(new QCheckBox(Tr::tr("Auto-suspend unmodified files")))
        , m_maxRecentFilesSpinBox(new QSpinBox)
        , m_enableCrashReportingCheckBox(new QCheckBox(Tr::tr("Enable crash reporting")))
        , m_warnBeforeOpeningBigFiles(
              new QCheckBox(Tr::tr("Warn before opening text files greater than")))
        , m_bigFilesLimitSpinBox(new QSpinBox)
        , m_terminalComboBox(new QComboBox)
        , m_terminalOpenArgs(new QLineEdit)
        , m_terminalExecuteArgs(new QLineEdit)
        , m_patchChooser(new Utils::PathChooser)
        , m_environmentChangesLabel(new Utils::ElidingLabel)
        , m_askBeforeExitCheckBox(new QCheckBox(Tr::tr("Ask for confirmation before exiting")))
        , m_reloadBehavior(new QComboBox)
        , m_autoSaveCheckBox(new QCheckBox(Tr::tr("Auto-save modified files")))
        , m_clearCrashReportsButton(new QPushButton(Tr::tr("Clear Local Crash Reports")))
        , m_crashReportsSizeText(new QLabel)
        , m_autoSaveInterval(new QSpinBox)
        , m_autoSaveRefactoringCheckBox(new QCheckBox(Tr::tr("Auto-save files after refactoring")))

    {
        m_autoSuspendCheckBox->setToolTip(
            Tr::tr("Automatically free resources of old documents that are not visible and not "
               "modified. They stay visible in the list of open documents."));
        m_autoSuspendMinDocumentCount->setMinimum(1);
        m_autoSuspendMinDocumentCount->setMaximum(500);
        m_autoSuspendMinDocumentCount->setValue(30);
        m_enableCrashReportingCheckBox->setToolTip(
            Tr::tr("Allow crashes to be automatically reported. Collected reports are "
               "used for the sole purpose of fixing bugs."));
        m_bigFilesLimitSpinBox->setSuffix(Tr::tr("MB"));
        m_bigFilesLimitSpinBox->setMinimum(1);
        m_bigFilesLimitSpinBox->setMaximum(500);
        m_bigFilesLimitSpinBox->setValue(5);
        m_terminalExecuteArgs->setToolTip(
            Tr::tr("Command line arguments used for \"Run in terminal\"."));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(5);
        m_environmentChangesLabel->setSizePolicy(sizePolicy);
        m_reloadBehavior->addItem(Tr::tr("Always Ask"));
        m_reloadBehavior->addItem(Tr::tr("Reload All Unchanged Editors"));
        m_reloadBehavior->addItem(Tr::tr("Ignore Modifications"));
        m_autoSaveInterval->setSuffix(Tr::tr("min"));
        QSizePolicy termSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        termSizePolicy.setHorizontalStretch(3);
        m_terminalComboBox->setSizePolicy(termSizePolicy);
        m_terminalComboBox->setMinimumSize(QSize(100, 0));
        m_terminalComboBox->setEditable(true);
        m_terminalOpenArgs->setToolTip(
            Tr::tr("Command line arguments used for \"%1\".").arg(FileUtils::msgTerminalHereAction()));
        m_autoSaveInterval->setMinimum(1);

        auto fileSystemCaseSensitivityLabel = new QLabel(Tr::tr("File system case sensitivity:"));
        fileSystemCaseSensitivityLabel->setToolTip(
            Tr::tr("Influences how file names are matched to decide if they are the same."));
        auto autoSuspendLabel = new QLabel(Tr::tr("Files to keep open:"));
        autoSuspendLabel->setToolTip(
            Tr::tr("Minimum number of open documents that should be kept in memory. Increasing this "
               "number will lead to greater resource usage when not manually closing documents."));
        auto resetFileBrowserButton = new QPushButton(Tr::tr("Reset"));
        resetFileBrowserButton->setToolTip(Tr::tr("Reset to default."));
        auto helpExternalFileBrowserButton = new QToolButton;
        helpExternalFileBrowserButton->setText(Tr::tr("?"));
        auto helpCrashReportingButton = new QToolButton;
        helpCrashReportingButton->setText(Tr::tr("?"));
        auto resetTerminalButton = new QPushButton(Tr::tr("Reset"));
        resetTerminalButton->setToolTip(Tr::tr("Reset to default.", "Terminal"));
        auto patchCommandLabel = new QLabel(Tr::tr("Patch command:"));
        auto environmentButton = new QPushButton(Tr::tr("Change..."));
        environmentButton->setSizePolicy(QSizePolicy::Fixed,
                                         environmentButton->sizePolicy().verticalPolicy());

        Grid form;
        form.addRow(
            {Tr::tr("Environment:"), Span(2, Row{m_environmentChangesLabel, environmentButton})});
        if (HostOsInfo::isAnyUnixHost()) {
            form.addRow({Tr::tr("Terminal:"),
                         Span(2,
                              Row{m_terminalComboBox,
                                  m_terminalOpenArgs,
                                  m_terminalExecuteArgs,
                                  resetTerminalButton})});
        }
        if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()) {
            form.addRow({Tr::tr("External file browser:"),
                         Span(2,
                              Row{m_externalFileBrowserEdit,
                                  resetFileBrowserButton,
                                  helpExternalFileBrowserButton})});
        }
        form.addRow({patchCommandLabel, Span(2, m_patchChooser)});
        if (HostOsInfo::isMacHost()) {
            form.addRow({fileSystemCaseSensitivityLabel,
                         Span(2, Row{m_fileSystemCaseSensitivityChooser, st})});
        }
        form.addRow(
            {Tr::tr("When files are externally modified:"), Span(2, Row{m_reloadBehavior, st})});
        form.addRow(
            {m_autoSaveCheckBox, Span(2, Row{Tr::tr("Interval:"), m_autoSaveInterval, st})});
        form.addRow(Span(3, m_autoSaveRefactoringCheckBox));
        form.addRow({m_autoSuspendCheckBox,
                     Span(2, Row{autoSuspendLabel, m_autoSuspendMinDocumentCount, st})});
        form.addRow(Span(3, Row{m_warnBeforeOpeningBigFiles, m_bigFilesLimitSpinBox, st}));
        form.addRow(Span(3,
                         Row{Tr::tr("Maximum number of entries in \"Recent Files\":"),
                             m_maxRecentFilesSpinBox,
                             st}));
        form.addRow(m_askBeforeExitCheckBox);
#ifdef ENABLE_CRASHPAD
        form.addRow(
            Span(3, Row{m_enableCrashReportingCheckBox, helpCrashReportingButton, st}));
        form.addRow(Span(3, Row{m_clearCrashReportsButton, m_crashReportsSizeText, st}));
#endif

        Column {
            Group {
                title(Tr::tr("System")),
                Column { form, st }
            }
        }.attachTo(this);

        m_reloadBehavior->setCurrentIndex(EditorManager::reloadSetting());
        if (HostOsInfo::isAnyUnixHost()) {
            const QVector<TerminalCommand> availableTerminals
                = TerminalCommand::availableTerminalEmulators();
            for (const TerminalCommand &term : availableTerminals)
                m_terminalComboBox->addItem(term.command.toUserOutput(), QVariant::fromValue(term));
            updateTerminalUi(TerminalCommand::terminalEmulator());
            connect(m_terminalComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
                updateTerminalUi(m_terminalComboBox->itemData(index).value<TerminalCommand>());
            });
        }

        if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()) {
            m_externalFileBrowserEdit->setText(UnixUtils::fileBrowser(ICore::settings()));
        }

        const QString patchToolTip = Tr::tr("Command used for reverting diff chunks.");
        patchCommandLabel->setToolTip(patchToolTip);
        m_patchChooser->setToolTip(patchToolTip);
        m_patchChooser->setExpectedKind(PathChooser::ExistingCommand);
        m_patchChooser->setHistoryCompleter(QLatin1String("General.PatchCommand.History"));
        m_patchChooser->setFilePath(PatchTool::patchCommand());
        m_autoSaveCheckBox->setChecked(EditorManagerPrivate::autoSaveEnabled());
        m_autoSaveCheckBox->setToolTip(Tr::tr("Automatically creates temporary copies of "
                                          "modified files. If %1 is restarted after "
                                          "a crash or power failure, it asks whether to "
                                          "recover the auto-saved content.")
                                           .arg(Constants::IDE_DISPLAY_NAME));
        m_autoSaveRefactoringCheckBox->setChecked(EditorManager::autoSaveAfterRefactoring());
        m_autoSaveRefactoringCheckBox->setToolTip(
            Tr::tr("Automatically saves all open files "
               "affected by a refactoring operation,\nprovided they were unmodified before the "
               "refactoring."));
        m_autoSaveInterval->setValue(EditorManagerPrivate::autoSaveInterval());
        m_autoSuspendCheckBox->setChecked(EditorManagerPrivate::autoSuspendEnabled());
        m_autoSuspendMinDocumentCount->setValue(EditorManagerPrivate::autoSuspendMinDocumentCount());
        m_warnBeforeOpeningBigFiles->setChecked(
            EditorManagerPrivate::warnBeforeOpeningBigFilesEnabled());
        m_bigFilesLimitSpinBox->setValue(EditorManagerPrivate::bigFileSizeLimit());
        m_maxRecentFilesSpinBox->setMinimum(1);
        m_maxRecentFilesSpinBox->setMaximum(99);
        m_maxRecentFilesSpinBox->setValue(EditorManagerPrivate::maxRecentFiles());
#ifdef ENABLE_CRASHPAD
        if (ICore::settings()->value(showCrashButtonKey).toBool()) {
            auto crashButton = new QPushButton("CRASH!!!");
            crashButton->show();
            connect(crashButton, &QPushButton::clicked, [] {
                // do a real crash
                volatile int* a = reinterpret_cast<volatile int *>(NULL); *a = 1;
            });
        }

        m_enableCrashReportingCheckBox->setChecked(
            ICore::settings()->value(crashReportingEnabledKey).toBool());
        connect(helpCrashReportingButton, &QAbstractButton::clicked, this, [this] {
            showHelpDialog(Tr::tr("Crash Reporting"), CorePlugin::msgCrashpadInformation());
        });
        connect(m_enableCrashReportingCheckBox, &QCheckBox::stateChanged, this, [this] {
            const QString restartText = Tr::tr("The change will take effect after restart.");
            Core::RestartDialog restartDialog(Core::ICore::dialogParent(), restartText);
            restartDialog.exec();
            if (restartDialog.result() == QDialog::Accepted)
                apply();
        });

        updateClearCrashWidgets();
        connect(m_clearCrashReportsButton, &QPushButton::clicked, this, [&] {
            const FilePaths &crashFiles = ICore::crashReportsPath().dirEntries(QDir::Files);
            for (const FilePath &file : crashFiles)
                file.removeFile();
            updateClearCrashWidgets();
        });
#endif

        m_askBeforeExitCheckBox->setChecked(
            static_cast<Core::Internal::MainWindow *>(ICore::mainWindow())
                ->askConfirmationBeforeExit());

        if (HostOsInfo::isAnyUnixHost()) {
            connect(resetTerminalButton,
                    &QAbstractButton::clicked,
                    this,
                    &SystemSettingsWidget::resetTerminal);
            if (!HostOsInfo::isMacHost()) {
                connect(resetFileBrowserButton,
                        &QAbstractButton::clicked,
                        this,
                        &SystemSettingsWidget::resetFileBrowser);
                connect(helpExternalFileBrowserButton,
                        &QAbstractButton::clicked,
                        this,
                        &SystemSettingsWidget::showHelpForFileBrowser);
            }
        }

        if (HostOsInfo::isMacHost()) {
            Qt::CaseSensitivity defaultSensitivity
                    = OsSpecificAspects::fileNameCaseSensitivity(HostOsInfo::hostOs());
            if (defaultSensitivity == Qt::CaseSensitive) {
                m_fileSystemCaseSensitivityChooser->addItem(Tr::tr("Case Sensitive (Default)"),
                                                            Qt::CaseSensitive);
            } else {
                m_fileSystemCaseSensitivityChooser->addItem(Tr::tr("Case Sensitive"), Qt::CaseSensitive);
            }
            if (defaultSensitivity == Qt::CaseInsensitive) {
                m_fileSystemCaseSensitivityChooser->addItem(Tr::tr("Case Insensitive (Default)"),
                                                            Qt::CaseInsensitive);
            } else {
                m_fileSystemCaseSensitivityChooser->addItem(Tr::tr("Case Insensitive"),
                                                            Qt::CaseInsensitive);
            }
            const Qt::CaseSensitivity sensitivity = EditorManagerPrivate::readFileSystemSensitivity(
                ICore::settings());
            if (sensitivity == Qt::CaseSensitive)
                m_fileSystemCaseSensitivityChooser->setCurrentIndex(0);
            else
                m_fileSystemCaseSensitivityChooser->setCurrentIndex(1);
        }

        updatePath();

        m_environmentChangesLabel->setElideMode(Qt::ElideRight);
        m_environmentChanges = CorePlugin::environmentChanges();
        updateEnvironmentChangesLabel();
        connect(environmentButton, &QPushButton::clicked, this, [this, environmentButton] {
            std::optional<EnvironmentItems> changes
                = Utils::EnvironmentDialog::getEnvironmentItems(environmentButton,
                                                                m_environmentChanges);
            if (!changes)
                return;
            m_environmentChanges = *changes;
            updateEnvironmentChangesLabel();
            updatePath();
        });

        connect(VcsManager::instance(), &VcsManager::configurationChanged,
                this, &SystemSettingsWidget::updatePath);
    }

private:
    void apply() final;

    void showHelpForFileBrowser();
    void resetFileBrowser();
    void resetTerminal();
    void updateTerminalUi(const Utils::TerminalCommand &term);
    void updatePath();
    void updateEnvironmentChangesLabel();
    void updateClearCrashWidgets();

    void showHelpDialog(const QString &title, const QString &helpText);

    QComboBox *m_fileSystemCaseSensitivityChooser;
    QSpinBox *m_autoSuspendMinDocumentCount;
    QLineEdit *m_externalFileBrowserEdit;
    QCheckBox *m_autoSuspendCheckBox;
    QSpinBox *m_maxRecentFilesSpinBox;
    QCheckBox *m_enableCrashReportingCheckBox;
    QCheckBox *m_warnBeforeOpeningBigFiles;
    QSpinBox *m_bigFilesLimitSpinBox;
    QComboBox *m_terminalComboBox;
    QLineEdit *m_terminalOpenArgs;
    QLineEdit *m_terminalExecuteArgs;
    Utils::PathChooser *m_patchChooser;
    Utils::ElidingLabel *m_environmentChangesLabel;
    QCheckBox *m_askBeforeExitCheckBox;
    QComboBox *m_reloadBehavior;
    QCheckBox *m_autoSaveCheckBox;
    QPushButton *m_clearCrashReportsButton;
    QLabel *m_crashReportsSizeText;
    QSpinBox *m_autoSaveInterval;
    QCheckBox *m_autoSaveRefactoringCheckBox;
    QPointer<QMessageBox> m_dialog;
    EnvironmentItems m_environmentChanges;
};

void SystemSettingsWidget::apply()
{
    QtcSettings *settings = ICore::settings();
    EditorManager::setReloadSetting(IDocument::ReloadSetting(m_reloadBehavior->currentIndex()));
    if (HostOsInfo::isAnyUnixHost()) {
        TerminalCommand::setTerminalEmulator({
            FilePath::fromUserInput(m_terminalComboBox->lineEdit()->text()),
            m_terminalOpenArgs->text(),
            m_terminalExecuteArgs->text()
        });
        if (!HostOsInfo::isMacHost()) {
            UnixUtils::setFileBrowser(settings, m_externalFileBrowserEdit->text());
        }
    }
    PatchTool::setPatchCommand(m_patchChooser->filePath());
    EditorManagerPrivate::setAutoSaveEnabled(m_autoSaveCheckBox->isChecked());
    EditorManagerPrivate::setAutoSaveInterval(m_autoSaveInterval->value());
    EditorManagerPrivate::setAutoSaveAfterRefactoring(m_autoSaveRefactoringCheckBox->isChecked());
    EditorManagerPrivate::setAutoSuspendEnabled(m_autoSuspendCheckBox->isChecked());
    EditorManagerPrivate::setAutoSuspendMinDocumentCount(m_autoSuspendMinDocumentCount->value());
    EditorManagerPrivate::setWarnBeforeOpeningBigFilesEnabled(
        m_warnBeforeOpeningBigFiles->isChecked());
    EditorManagerPrivate::setBigFileSizeLimit(m_bigFilesLimitSpinBox->value());
    EditorManagerPrivate::setMaxRecentFiles(m_maxRecentFilesSpinBox->value());
#ifdef ENABLE_CRASHPAD
    ICore::settings()->setValue(crashReportingEnabledKey,
                                m_enableCrashReportingCheckBox->isChecked());
#endif

    static_cast<Core::Internal::MainWindow *>(ICore::mainWindow())
        ->setAskConfirmationBeforeExit(m_askBeforeExitCheckBox->isChecked());

    if (HostOsInfo::isMacHost()) {
        const Qt::CaseSensitivity sensitivity = EditorManagerPrivate::readFileSystemSensitivity(
            settings);
        const Qt::CaseSensitivity selectedSensitivity = Qt::CaseSensitivity(
            m_fileSystemCaseSensitivityChooser->currentData().toInt());
        if (selectedSensitivity != sensitivity) {
            EditorManagerPrivate::writeFileSystemSensitivity(settings, selectedSensitivity);
            RestartDialog dialog(
                ICore::dialogParent(),
                Tr::tr("The file system case sensitivity change will take effect after restart."));
            dialog.exec();
        }
    }

    CorePlugin::setEnvironmentChanges(m_environmentChanges);
}

void SystemSettingsWidget::resetTerminal()
{
    if (HostOsInfo::isAnyUnixHost())
        m_terminalComboBox->setCurrentIndex(0);
}

void SystemSettingsWidget::updateTerminalUi(const TerminalCommand &term)
{
    m_terminalComboBox->lineEdit()->setText(term.command.toUserOutput());
    m_terminalOpenArgs->setText(term.openArgs);
    m_terminalExecuteArgs->setText(term.executeArgs);
}

void SystemSettingsWidget::resetFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        m_externalFileBrowserEdit->setText(UnixUtils::defaultFileBrowser());
}

void SystemSettingsWidget::updatePath()
{
    EnvironmentChange change;
    change.addAppendToPath(VcsManager::additionalToolsPath());
    m_patchChooser->setEnvironmentChange(change);
}

void SystemSettingsWidget::updateEnvironmentChangesLabel()
{
    const QString shortSummary = Utils::EnvironmentItem::toStringList(m_environmentChanges)
                                     .join("; ");
    m_environmentChangesLabel->setText(shortSummary.isEmpty() ? Tr::tr("No changes to apply.")
                                                              : shortSummary);
}

void SystemSettingsWidget::showHelpDialog(const QString &title, const QString &helpText)
{
    if (m_dialog) {
        if (m_dialog->windowTitle() != title)
            m_dialog->setText(helpText);

        if (m_dialog->text() != helpText)
            m_dialog->setText(helpText);

        m_dialog->show();
        ICore::raiseWindow(m_dialog);
        return;
    }
    auto mb = new QMessageBox(QMessageBox::Information, title, helpText, QMessageBox::Close, this);
    mb->setWindowModality(Qt::NonModal);
    m_dialog = mb;
    mb->show();
}

#ifdef ENABLE_CRASHPAD
void SystemSettingsWidget::updateClearCrashWidgets()
{
    QDir crashReportsDir(ICore::crashReportsPath().path());
    crashReportsDir.setFilter(QDir::Files);
    qint64 size = 0;
    const FilePaths crashFiles = ICore::crashReportsPath().dirEntries(QDir::Files);
    for (const FilePath &file : crashFiles)
        size += file.fileSize();
    m_clearCrashReportsButton->setEnabled(!crashFiles.isEmpty());
    m_crashReportsSizeText->setText(formatSize(size));
}
#endif

void SystemSettingsWidget::showHelpForFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        showHelpDialog(Tr::tr("Variables"), UnixUtils::fileBrowserHelpText());
}

SystemSettings::SystemSettings()
{
    setId(Constants::SETTINGS_ID_SYSTEM);
    setDisplayName(Tr::tr("System"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setWidgetCreator([] { return new SystemSettingsWidget; });
}

} // namespace Internal
} // namespace Core
