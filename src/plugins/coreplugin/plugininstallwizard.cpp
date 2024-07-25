// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugininstallwizard.h"

#include "coreplugin.h"
#include "coreplugintr.h"
#include "icore.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/async.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <utils/unarchiver.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>

#include <QButtonGroup>
#include <QDir>
#include <QDirIterator>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>

#include <memory>

using namespace ExtensionSystem;
using namespace Tasking;
using namespace Utils;

struct Data
{
    FilePath sourcePath;
    FilePath extractedPath;
    bool installIntoApplication = false;
    std::unique_ptr<PluginSpec> pluginSpec = nullptr;
};

static bool hasLibSuffix(const FilePath &path)
{
    return (HostOsInfo::isWindowsHost() && path.endsWith(".dll"))
           || (HostOsInfo::isLinuxHost() && path.completeSuffix().startsWith("so"))
           || (HostOsInfo::isMacHost() && path.endsWith(".dylib"));
}

namespace Core {

class SourcePage : public WizardPage
{
public:
    SourcePage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Source"));

        auto label = new QLabel(
            "<p>"
            + Tr::tr("Choose source location. This can be a plugin library file or a zip file.")
            + "</p>");
        label->setWordWrap(true);

        auto chooser = new PathChooser;
        chooser->setExpectedKind(PathChooser::Any);
        connect(chooser, &PathChooser::textChanged, this, [this, chooser] {
            m_data->sourcePath = chooser->filePath();
            updateWarnings();
        });

        m_info = new InfoLabel;
        m_info->setType(InfoLabel::Error);
        m_info->setVisible(false);

        Layouting::Column { label, chooser, m_info }.attachTo(this);
    }

    void updateWarnings()
    {
        m_info->setVisible(!isComplete());
        emit completeChanged();
    }

    bool isComplete() const final
    {
        const FilePath path = m_data->sourcePath;
        if (!QFileInfo::exists(path.toString())) {
            m_info->setText(Tr::tr("File does not exist."));
            return false;
        }
        if (hasLibSuffix(path))
            return true;

        const auto sourceAndCommand = Unarchiver::sourceAndCommand(path);
        if (!sourceAndCommand)
            m_info->setText(sourceAndCommand.error());

        return bool(sourceAndCommand);
    }

    int nextId() const final
    {
        if (hasLibSuffix(m_data->sourcePath))
            return WizardPage::nextId() + 1; // jump over check archive
        return WizardPage::nextId();
    }

    InfoLabel *m_info = nullptr;
    Data *m_data = nullptr;
};

using CheckResult = expected_str<PluginSpec *>;

// Async. Result is set if any issue was found.
void checkContents(QPromise<CheckResult> &promise, const FilePath &tempDir)
{
    QList<PluginSpec *> plugins = pluginSpecsFromArchive(tempDir);
    if (plugins.isEmpty()) {
        promise.addResult(Utils::make_unexpected(Tr::tr("No plugins found.")));
        return;
    }
    if (plugins.size() > 1) {
        promise.addResult(Utils::make_unexpected(Tr::tr("More than one plugin found.")));
        qDeleteAll(plugins);
        return;
    }

    if (!plugins.front()->resolveDependencies(PluginManager::plugins())) {
        promise.addResult(Utils::make_unexpected(
            Tr::tr("Plugin failed to resolve dependencies:") + " "
            + plugins.front()->errorString()));
        qDeleteAll(plugins);
        return;
    }

    promise.addResult(plugins.front());
}

class CheckArchivePage : public WizardPage
{
public:
    CheckArchivePage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Check Archive"));

        m_label = new InfoLabel;
        m_label->setElideMode(Qt::ElideNone);
        m_label->setWordWrap(true);
        m_cancelButton = new QPushButton(Tr::tr("Cancel"));
        connect(m_cancelButton, &QPushButton::clicked, this, [this] {
            m_taskTreeRunner.reset();
            m_cancelButton->setVisible(false);
            m_label->setType(InfoLabel::Information);
            m_label->setText(Tr::tr("Canceled."));
        });
        m_output = new QTextEdit;
        m_output->setReadOnly(true);

        using namespace Layouting;
        Column {
            Row { m_label, st, m_cancelButton },
            m_output,
        }.attachTo(this);
    }

    void initializePage() final
    {
        m_isComplete = false;
        emit completeChanged();

        m_tempDir = std::make_unique<TemporaryDirectory>("plugininstall");
        m_data->extractedPath = m_tempDir->path();
        m_label->setText(Tr::tr("Checking archive..."));
        m_label->setType(InfoLabel::None);

        m_output->clear();

        const auto sourceAndCommand = Unarchiver::sourceAndCommand(m_data->sourcePath);
        if (!sourceAndCommand) {
            m_label->setType(InfoLabel::Error);
            m_label->setText(sourceAndCommand.error());
            return;
        }

        const auto onUnarchiverSetup = [this, sourceAndCommand](Unarchiver &unarchiver) {
            unarchiver.setSourceAndCommand(*sourceAndCommand);
            unarchiver.setDestDir(m_tempDir->path());
            connect(&unarchiver, &Unarchiver::outputReceived, this, [this](const QString &output) {
                m_output->append(output);
            });
        };
        const auto onUnarchiverError = [this] {
            m_label->setType(InfoLabel::Error);
            m_label->setText(Tr::tr("There was an error while unarchiving."));
        };

        const auto onCheckerSetup = [this](Async<CheckResult> &async) {
            if (!m_tempDir)
                return SetupResult::StopWithError;

            async.setConcurrentCallData(checkContents, m_tempDir->path());
            return SetupResult::Continue;
        };
        const auto onCheckerDone = [this](const Async<CheckResult> &async) {
            expected_str<PluginSpec *> result = async.result();
            if (!result) {
                m_label->setType(InfoLabel::Error);
                m_label->setText(result.error());
            } else {
                m_label->setType(InfoLabel::Ok);
                m_label->setText(Tr::tr("Archive is OK."));
                m_data->pluginSpec.reset(*result);
                m_isComplete = true;
            }
            emit completeChanged();
        };

        // clang-format off
        const Group root{
            UnarchiverTask(onUnarchiverSetup, onUnarchiverError, CallDoneIf::Error),
            AsyncTask<CheckResult>(onCheckerSetup, onCheckerDone, CallDoneIf::Success)
        };
        // clang-format on
        m_cancelButton->setVisible(true);
        m_taskTreeRunner.start(root, {}, [this](DoneWith) { m_cancelButton->setVisible(false); });
    }

    void cleanupPage() final
    {
        // back button pressed
        m_taskTreeRunner.reset();
        m_tempDir.reset();
    }

    bool isComplete() const final { return m_isComplete; }

    std::unique_ptr<TemporaryDirectory> m_tempDir;
    TaskTreeRunner m_taskTreeRunner;
    InfoLabel *m_label = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QTextEdit *m_output = nullptr;
    Data *m_data = nullptr;
    bool m_isComplete = false;
};

class InstallLocationPage : public WizardPage
{
public:
    InstallLocationPage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Install Location"));

        auto label = new QLabel("<p>" + Tr::tr("Choose install location.") + "</p>");
        label->setWordWrap(true);

        auto localInstall = new QRadioButton(Tr::tr("User plugins"));
        localInstall->setChecked(!m_data->installIntoApplication);
        auto localLabel = new QLabel(Tr::tr("The plugin will be available to all compatible %1 "
                                            "installations, but only for the current user.")
                                         .arg(QGuiApplication::applicationDisplayName()));
        localLabel->setWordWrap(true);
        localLabel->setAttribute(Qt::WA_MacSmallSize, true);

        auto appInstall = new QRadioButton(
            Tr::tr("%1 installation").arg(QGuiApplication::applicationDisplayName()));
        appInstall->setChecked(m_data->installIntoApplication);
        auto appLabel = new QLabel(Tr::tr("The plugin will be available only to this %1 "
                                          "installation, but for all users that can access it.")
                                       .arg(QGuiApplication::applicationDisplayName()));
        appLabel->setWordWrap(true);
        appLabel->setAttribute(Qt::WA_MacSmallSize, true);

        using namespace Layouting;
        Column {
            label, Space(10), localInstall, localLabel, Space(10), appInstall, appLabel,
        }.attachTo(this);

        auto group = new QButtonGroup(this);
        group->addButton(localInstall);
        group->addButton(appInstall);

        connect(appInstall, &QRadioButton::toggled, this, [this](bool toggled) {
            m_data->installIntoApplication = toggled;
        });
    }

    Data *m_data = nullptr;
};

class SummaryPage : public WizardPage
{
public:
    SummaryPage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Summary"));

        m_summaryLabel = new QLabel(this);
        m_summaryLabel->setWordWrap(true);
        Layouting::Column { m_summaryLabel }.attachTo(this);
    }

    void initializePage() final
    {
        m_summaryLabel->setText(
            Tr::tr("\"%1\" will be installed into \"%2\".")
                .arg(m_data->sourcePath.toUserOutput())
                .arg(m_data->pluginSpec->installLocation(!m_data->installIntoApplication)
                         .toUserOutput()));
    }

private:
    QLabel *m_summaryLabel;
    Data *m_data = nullptr;
};

static std::function<void(FilePath)> postCopyOperation()
{
    return [](const FilePath &filePath) {
        if (!HostOsInfo::isMacHost())
            return;
        // On macOS, downloaded files get a quarantine flag, remove it, otherwise it is a hassle
        // to get it loaded as a plugin in Qt Creator.
        Process xattr;
        xattr.setCommand({"/usr/bin/xattr", {"-d", "com.apple.quarantine", filePath.absoluteFilePath().toString()}});
        using namespace std::chrono_literals;
        xattr.runBlocking(1s);
    };
}

static bool copyPluginFile(const FilePath &src, const FilePath &dest)
{
    const FilePath destFile = dest.pathAppended(src.fileName());
    if (destFile.exists()) {
        QMessageBox box(QMessageBox::Question,
                        Tr::tr("Overwrite File"),
                        Tr::tr("The file \"%1\" exists. Overwrite?").arg(destFile.toUserOutput()),
                        QMessageBox::Cancel,
                        ICore::dialogParent());
        QPushButton *acceptButton = box.addButton(Tr::tr("Overwrite"), QMessageBox::AcceptRole);
        box.setDefaultButton(acceptButton);
        box.exec();
        if (box.clickedButton() != acceptButton)
            return false;
        destFile.removeFile();
    }
    dest.parentDir().ensureWritableDir();
    if (!src.copyFile(destFile)) {
        QMessageBox::warning(ICore::dialogParent(),
                             Tr::tr("Failed to Write File"),
                             Tr::tr("Failed to write file \"%1\".").arg(destFile.toUserOutput()));
        return false;
    }
    postCopyOperation()(destFile);
    return true;
}

bool executePluginInstallWizard(const FilePath &archive)
{
    Wizard wizard(ICore::dialogParent());
    wizard.setWindowTitle(Tr::tr("Install Plugin"));

    Data data;

    if (archive.isEmpty()) {
        auto filePage = new SourcePage(&data, &wizard);
        wizard.addPage(filePage);
    } else {
        data.sourcePath = archive;
    }

    auto checkArchivePage = new CheckArchivePage(&data, &wizard);
    wizard.addPage(checkArchivePage);

    auto installLocationPage = new InstallLocationPage(&data, &wizard);
    wizard.addPage(installLocationPage);

    auto summaryPage = new SummaryPage(&data, &wizard);
    wizard.addPage(summaryPage);

    if (wizard.exec()) {
        const FilePath installPath = data.pluginSpec->installLocation(!data.installIntoApplication);
        if (hasLibSuffix(data.sourcePath)) {
            return copyPluginFile(data.sourcePath, installPath);
        } else {
            QString error;
            FileUtils::CopyAskingForOverwrite copy(ICore::dialogParent(),
                                              postCopyOperation());
            if (!FileUtils::copyRecursively(data.extractedPath,
                                            installPath,
                                            &error,
                                            copy())) {
                QMessageBox::warning(ICore::dialogParent(),
                                     Tr::tr("Failed to Copy Plugin Files"),
                                     error);
                return false;
            }
            return true;
        }
    }
    return false;
}

} // namespace Core
