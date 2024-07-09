// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidsdkmanager.h"
#include "androidtr.h"
#include "sdkmanageroutputparser.h"

#include <coreplugin/icore.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/qtcprocess.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QRegularExpression>
#include <QTextCodec>

namespace {
Q_LOGGING_CATEGORY(sdkManagerLog, "qtc.android.sdkManager", QtWarningMsg)
}

using namespace Tasking;
using namespace Utils;

using namespace std::chrono;
using namespace std::chrono_literals;

namespace Android::Internal {

class QuestionProgressDialog : public QDialog
{
    Q_OBJECT

public:
    QuestionProgressDialog(QWidget *parent)
        : QDialog(parent)
        , m_outputTextEdit(new QPlainTextEdit)
        , m_questionLabel(new QLabel(Tr::tr("Do you want to accept the Android SDK license?")))
        , m_answerButtonBox(new QDialogButtonBox)
        , m_progressBar(new QProgressBar)
        , m_dialogButtonBox(new QDialogButtonBox)
        , m_formatter(new OutputFormatter)
    {
        setWindowTitle(Tr::tr("Android SDK Manager"));
        m_outputTextEdit->setReadOnly(true);
        m_questionLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
        m_answerButtonBox->setStandardButtons(QDialogButtonBox::No | QDialogButtonBox::Yes);
        m_dialogButtonBox->setStandardButtons(QDialogButtonBox::Cancel);
        m_formatter->setPlainTextEdit(m_outputTextEdit);
        m_formatter->setParent(this);

        using namespace Layouting;

        Column {
            m_outputTextEdit,
            Row { m_questionLabel, m_answerButtonBox },
            m_progressBar,
            m_dialogButtonBox
        }.attachTo(this);

        setQuestionVisible(false);
        setQuestionEnabled(false);

        connect(m_answerButtonBox, &QDialogButtonBox::rejected, this, [this] {
            emit answerClicked(false);
        });
        connect(m_answerButtonBox, &QDialogButtonBox::accepted, this, [this] {
            emit answerClicked(true);
        });
        connect(m_dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // GUI tuning
        setModal(true);
        resize(800, 600);
        show();
    }

    void setQuestionEnabled(bool enable)
    {
        m_questionLabel->setEnabled(enable);
        m_answerButtonBox->setEnabled(enable);
    }
    void setQuestionVisible(bool visible)
    {
        m_questionLabel->setVisible(visible);
        m_answerButtonBox->setVisible(visible);
    }
    void appendMessage(const QString &text, OutputFormat format)
    {
        m_formatter->appendMessage(text, format);
        m_outputTextEdit->ensureCursorVisible();
    }
    void setProgress(int value) { m_progressBar->setValue(value); }

signals:
    void answerClicked(bool accepted);

private:
    QPlainTextEdit *m_outputTextEdit = nullptr;
    QLabel *m_questionLabel = nullptr;
    QDialogButtonBox *m_answerButtonBox = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QDialogButtonBox *m_dialogButtonBox = nullptr;
    OutputFormatter *m_formatter = nullptr;
};

static QString sdkRootArg()
{
    return "--sdk_root=" + AndroidConfig::sdkLocation().toString();
}

const QRegularExpression &assertionRegExp()
{
    static const QRegularExpression theRegExp
        (R"((\(\s*y\s*[\/\\]\s*n\s*\)\s*)(?<mark>[\:\?]))", // (y/N)?
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);

    return theRegExp;
}

static std::optional<int> parseProgress(const QString &out)
{
    if (out.isEmpty())
        return {};

    static const QRegularExpression reg("(?<progress>\\d*)%");
    static const QRegularExpression regEndOfLine("[\\n\\r]");
    const QStringList lines = out.split(regEndOfLine, Qt::SkipEmptyParts);
    std::optional<int> progress;
    for (const QString &line : lines) {
        QRegularExpressionMatch match = reg.match(line);
        if (match.hasMatch()) {
            const int parsedProgress = match.captured("progress").toInt();
            if (parsedProgress >= 0 && parsedProgress <= 100)
                progress = parsedProgress;
        }
    }
    return progress;
}

struct DialogStorage
{
    DialogStorage() { m_dialog.reset(new QuestionProgressDialog(Core::ICore::dialogParent())); };
    std::unique_ptr<QuestionProgressDialog> m_dialog;
};

static GroupItem licensesRecipe(const Storage<DialogStorage> &dialogStorage)
{
    struct OutputData
    {
        QString buffer;
        int current = 0;
        int total = 0;
    };

    const Storage<OutputData> outputStorage;

    const auto onLicenseSetup = [dialogStorage, outputStorage](Process &process) {
        QuestionProgressDialog *dialog = dialogStorage->m_dialog.get();
        dialog->setProgress(0);
        dialog->appendMessage(Tr::tr("Checking pending licenses...") + "\n", NormalMessageFormat);
        dialog->appendMessage(Tr::tr("The installation of Android SDK packages may fail if the "
                                     "respective licenses are not accepted.") + "\n\n",
                              LogMessageFormat);
        process.setProcessMode(ProcessMode::Writer);
        process.setEnvironment(AndroidConfig::toolsEnvironment());
        process.setCommand(CommandLine(AndroidConfig::sdkManagerToolPath(),
                                       {"--licenses", sdkRootArg()}));
        process.setUseCtrlCStub(true);

        Process *processPtr = &process;
        OutputData *outputPtr = outputStorage.activeStorage();
        QObject::connect(processPtr, &Process::readyReadStandardOutput, dialog,
                         [processPtr, outputPtr, dialog] {
            QTextCodec *codec = QTextCodec::codecForLocale();
            const QString stdOut = codec->toUnicode(processPtr->readAllRawStandardOutput());
            outputPtr->buffer += stdOut;
            dialog->appendMessage(stdOut, StdOutFormat);
            const auto progress = parseProgress(stdOut);
            if (progress)
                dialog->setProgress(*progress);
            if (assertionRegExp().match(outputPtr->buffer).hasMatch()) {
                dialog->setQuestionVisible(true);
                dialog->setQuestionEnabled(true);
                if (outputPtr->total == 0) {
                    // Example output to match:
                    //   5 of 6 SDK package licenses not accepted.
                    //   Review licenses that have not been accepted (y/N)?
                    static const QRegularExpression reg(R"(((?<steps>\d+)\sof\s)\d+)");
                    const QRegularExpressionMatch match = reg.match(outputPtr->buffer);
                    if (match.hasMatch()) {
                        outputPtr->total = match.captured("steps").toInt();
                        const QByteArray reply = "y\n";
                        dialog->appendMessage(QString::fromUtf8(reply), NormalMessageFormat);
                        processPtr->writeRaw(reply);
                        dialog->setProgress(0);
                    }
                }
                outputPtr->buffer.clear();
            }
        });

        QObject::connect(dialog, &QuestionProgressDialog::answerClicked, processPtr,
                         [processPtr, outputPtr, dialog](bool accepted) {
            dialog->setQuestionEnabled(false);
            const QByteArray reply = accepted ? "y\n" : "n\n";
            dialog->appendMessage(QString::fromUtf8(reply), NormalMessageFormat);
            processPtr->writeRaw(reply);
            ++outputPtr->current;
            if (outputPtr->total != 0)
                dialog->setProgress(outputPtr->current * 100.0 / outputPtr->total);
        });
    };

    return Group { outputStorage, ProcessTask(onLicenseSetup) };
}

static void setupSdkProcess(const QStringList &args, Process *process,
                            QuestionProgressDialog *dialog, int current, int total)
{
    process->setEnvironment(AndroidConfig::toolsEnvironment());
    process->setCommand({AndroidConfig::sdkManagerToolPath(),
                         args + AndroidConfig::sdkManagerToolArgs()});
    QObject::connect(process, &Process::readyReadStandardOutput, dialog,
                     [process, dialog, current, total] {
        QTextCodec *codec = QTextCodec::codecForLocale();
        const auto progress = parseProgress(codec->toUnicode(process->readAllRawStandardOutput()));
        if (!progress)
            return;
        dialog->setProgress((current * 100.0 + *progress) / total);
    });
    QObject::connect(process, &Process::readyReadStandardError, dialog, [process, dialog] {
        QTextCodec *codec = QTextCodec::codecForLocale();
        dialog->appendMessage(codec->toUnicode(process->readAllRawStandardError()), StdErrFormat);
    });
};

static void handleSdkProcess(QuestionProgressDialog *dialog, DoneWith result)
{
    if (result == DoneWith::Success)
        dialog->appendMessage(Tr::tr("Finished successfully.") + "\n\n", StdOutFormat);
    else
        dialog->appendMessage(Tr::tr("Failed.") + "\n\n", StdErrFormat);
}

static GroupItem installationRecipe(const Storage<DialogStorage> &dialogStorage,
                                    const InstallationChange &change)
{
    const auto onSetup = [dialogStorage] {
        dialogStorage->m_dialog->appendMessage(
            Tr::tr("Installing / Uninstalling selected packages...") + '\n', NormalMessageFormat);
        const QString optionsMessage = HostOsInfo::isMacHost()
            ? Tr::tr("Closing the preferences dialog will cancel the running and scheduled SDK "
                     "operations.")
            : Tr::tr("Closing the options dialog will cancel the running and scheduled SDK "
                     "operations.");
        dialogStorage->m_dialog->appendMessage(optionsMessage + '\n', LogMessageFormat);
    };

    const int total = change.count();
    const LoopList uninstallIterator(change.toUninstall);
    const auto onUninstallSetup = [dialogStorage, uninstallIterator, total](Process &process) {
        const QStringList args = {"--uninstall", *uninstallIterator, sdkRootArg()};
        QuestionProgressDialog *dialog = dialogStorage->m_dialog.get();
        setupSdkProcess(args, &process, dialog, uninstallIterator.iteration(), total);
        dialog->appendMessage(Tr::tr("Uninstalling %1...").arg(*uninstallIterator) + '\n',
                              StdOutFormat);
        dialog->setProgress(uninstallIterator.iteration() * 100.0 / total);
    };

    const LoopList installIterator(change.toInstall);
    const int offset = change.toUninstall.count();
    const auto onInstallSetup = [dialogStorage, installIterator, offset, total](Process &process) {
        const QStringList args = {*installIterator, sdkRootArg()};
        QuestionProgressDialog *dialog = dialogStorage->m_dialog.get();
        setupSdkProcess(args, &process, dialog, offset + installIterator.iteration(), total);
        dialog->appendMessage(Tr::tr("Installing %1...").arg(*installIterator) + '\n',
                              StdOutFormat);
        dialog->setProgress((offset + installIterator.iteration()) * 100.0 / total);
    };

    const auto onDone = [dialogStorage](DoneWith result) {
        handleSdkProcess(dialogStorage->m_dialog.get(), result);
    };

    return Group {
        onGroupSetup(onSetup),
        For {
            uninstallIterator,
            finishAllAndSuccess,
            ProcessTask(onUninstallSetup, onDone)
        },
        For {
            installIterator,
            finishAllAndSuccess,
            ProcessTask(onInstallSetup, onDone)
        }
    };
}

static GroupItem updateRecipe(const Storage<DialogStorage> &dialogStorage)
{
    const auto onUpdateSetup = [dialogStorage](Process &process) {
        const QStringList args = {"--update", sdkRootArg()};
        QuestionProgressDialog *dialog = dialogStorage->m_dialog.get();
        setupSdkProcess(args, &process, dialog, 0, 1);
        dialog->appendMessage(Tr::tr("Updating installed packages...") + '\n', NormalMessageFormat);
        dialog->setProgress(0);
    };
    const auto onDone = [dialogStorage](DoneWith result) {
        handleSdkProcess(dialogStorage->m_dialog.get(), result);
    };

    return ProcessTask(onUpdateSetup, onDone);
}

class AndroidSdkManagerPrivate
{
public:
    AndroidSdkManagerPrivate(AndroidSdkManager &sdkManager);
    ~AndroidSdkManagerPrivate();

    AndroidSdkPackageList filteredPackages(AndroidSdkPackage::PackageState state,
                                           AndroidSdkPackage::PackageType type)
    {
        m_sdkManager.refreshPackages();
        return Utils::filtered(m_allPackages, [state, type](const AndroidSdkPackage *p) {
            return p->state() & state && p->type() & type;
        });
    }
    const AndroidSdkPackageList &allPackages();

    void reloadSdkPackages();

    void runDialogRecipe(const Storage<DialogStorage> &dialogStorage,
                         const GroupItem &licenseRecipe, const GroupItem &continuationRecipe);

    AndroidSdkManager &m_sdkManager;
    AndroidSdkPackageList m_allPackages;
    FilePath lastSdkManagerPath;
    bool m_packageListingSuccessful = false;
    TaskTreeRunner m_taskTreeRunner;
};

AndroidSdkManager::AndroidSdkManager() : m_d(new AndroidSdkManagerPrivate(*this)) {}

AndroidSdkManager::~AndroidSdkManager() = default;

SdkPlatformList AndroidSdkManager::installedSdkPlatforms()
{
    const AndroidSdkPackageList list = m_d->filteredPackages(AndroidSdkPackage::Installed,
                                                             AndroidSdkPackage::SdkPlatformPackage);
    return Utils::static_container_cast<SdkPlatform *>(list);
}

const AndroidSdkPackageList &AndroidSdkManager::allSdkPackages()
{
    return m_d->allPackages();
}

QStringList AndroidSdkManager::notFoundEssentialSdkPackages()
{
    QStringList essentials = AndroidConfig::allEssentials();
    const AndroidSdkPackageList &packages = allSdkPackages();
    for (AndroidSdkPackage *package : packages) {
        essentials.removeOne(package->sdkStylePath());
        if (essentials.isEmpty())
            return {};
    }
    return essentials;
}

QStringList AndroidSdkManager::missingEssentialSdkPackages()
{
    const QStringList essentials = AndroidConfig::allEssentials();
    const AndroidSdkPackageList &packages = allSdkPackages();
    QStringList missingPackages;
    for (AndroidSdkPackage *package : packages) {
        if (essentials.contains(package->sdkStylePath())
            && package->state() != AndroidSdkPackage::Installed) {
            missingPackages.append(package->sdkStylePath());
        }
    }
    return missingPackages;
}

AndroidSdkPackageList AndroidSdkManager::installedSdkPackages()
{
    return m_d->filteredPackages(AndroidSdkPackage::Installed, AndroidSdkPackage::AnyValidType);
}

SystemImageList AndroidSdkManager::installedSystemImages()
{
    const AndroidSdkPackageList list = m_d->filteredPackages(AndroidSdkPackage::AnyValidState,
                                                             AndroidSdkPackage::SdkPlatformPackage);
    const QList<SdkPlatform *> platforms = Utils::static_container_cast<SdkPlatform *>(list);
    SystemImageList result;
    for (SdkPlatform *platform : platforms) {
        if (platform->systemImages().size() > 0)
            result.append(platform->systemImages());
    }
    return result;
}

NdkList AndroidSdkManager::installedNdkPackages()
{
    const AndroidSdkPackageList list = m_d->filteredPackages(AndroidSdkPackage::Installed,
                                                             AndroidSdkPackage::NDKPackage);
    return Utils::static_container_cast<Ndk *>(list);
}

SdkPlatform *AndroidSdkManager::latestAndroidSdkPlatform(AndroidSdkPackage::PackageState state)
{
    SdkPlatform *result = nullptr;
    const AndroidSdkPackageList list = m_d->filteredPackages(state,
                                                             AndroidSdkPackage::SdkPlatformPackage);
    for (AndroidSdkPackage *p : list) {
        auto platform = static_cast<SdkPlatform *>(p);
        if (!result || result->apiLevel() < platform->apiLevel())
            result = platform;
    }
    return result;
}

SdkPlatformList AndroidSdkManager::filteredSdkPlatforms(int minApiLevel,
                                                        AndroidSdkPackage::PackageState state)
{
    const AndroidSdkPackageList list = m_d->filteredPackages(state,
                                                             AndroidSdkPackage::SdkPlatformPackage);
    SdkPlatformList result;
    for (AndroidSdkPackage *p : list) {
        auto platform = static_cast<SdkPlatform *>(p);
        if (platform && platform->apiLevel() >= minApiLevel)
            result << platform;
    }
    return result;
}

BuildToolsList AndroidSdkManager::filteredBuildTools(int minApiLevel,
                                                     AndroidSdkPackage::PackageState state)
{
    const AndroidSdkPackageList list = m_d->filteredPackages(state,
                                                             AndroidSdkPackage::BuildToolsPackage);
    BuildToolsList result;
    for (AndroidSdkPackage *p : list) {
        auto platform = dynamic_cast<BuildTools *>(p);
        if (platform && platform->revision().majorVersion() >= minApiLevel)
            result << platform;
    }
    return result;
}

void AndroidSdkManager::refreshPackages()
{
    if (AndroidConfig::sdkManagerToolPath() != m_d->lastSdkManagerPath)
        reloadPackages();
}

void AndroidSdkManager::reloadPackages()
{
    m_d->reloadSdkPackages();
}

bool AndroidSdkManager::packageListingSuccessful() const
{
    return m_d->m_packageListingSuccessful;
}

/*!
    Runs the \c sdkmanger tool with arguments \a args. Returns \c true if the command is
    successfully executed. Output is copied into \a output. The function blocks the calling thread.
 */
static bool sdkManagerCommand(const QStringList &args, QString *output)
{
    QStringList newArgs = args;
    newArgs.append(sdkRootArg());
    Process proc;
    proc.setEnvironment(AndroidConfig::toolsEnvironment());
    proc.setTimeOutMessageBoxEnabled(true);
    proc.setCommand({AndroidConfig::sdkManagerToolPath(), newArgs});
    qCDebug(sdkManagerLog).noquote() << "Running SDK Manager command (sync):"
                                     << proc.commandLine().toUserOutput();
    proc.runBlocking(60s, EventLoopMode::On);
    if (output)
        *output = proc.allOutput();
    return proc.result() == ProcessResult::FinishedWithSuccess;
}

AndroidSdkManagerPrivate::AndroidSdkManagerPrivate(AndroidSdkManager &sdkManager)
    : m_sdkManager(sdkManager)
{}

AndroidSdkManagerPrivate::~AndroidSdkManagerPrivate()
{
    qDeleteAll(m_allPackages);
}

const AndroidSdkPackageList &AndroidSdkManagerPrivate::allPackages()
{
    m_sdkManager.refreshPackages();
    return m_allPackages;
}

void AndroidSdkManagerPrivate::reloadSdkPackages()
{
    emit m_sdkManager.packageReloadBegin();
    qDeleteAll(m_allPackages);
    m_allPackages.clear();

    lastSdkManagerPath = AndroidConfig::sdkManagerToolPath();
    m_packageListingSuccessful = false;

    if (AndroidConfig::sdkToolsVersion().isNull()) {
        // Configuration has invalid sdk path or corrupt installation.
        emit m_sdkManager.packageReloadFinished();
        return;
    }

    QString packageListing;
    QStringList args({"--list", "--verbose"});
    args << AndroidConfig::sdkManagerToolArgs();
    m_packageListingSuccessful = sdkManagerCommand(args, &packageListing);
    if (m_packageListingSuccessful) {
        SdkManagerOutputParser parser(m_allPackages);
        parser.parsePackageListing(packageListing);
    } else {
        qCWarning(sdkManagerLog) << "Failed parsing packages:" << packageListing;
    }

    emit m_sdkManager.packageReloadFinished();
}

void AndroidSdkManagerPrivate::runDialogRecipe(const Storage<DialogStorage> &dialogStorage,
                                               const GroupItem &licensesRecipe,
                                               const GroupItem &continuationRecipe)
{
    const auto onCancelSetup = [dialogStorage] {
        return std::make_pair(dialogStorage->m_dialog.get(), &QDialog::rejected);
    };
    const Group root {
        dialogStorage,
        Group {
            licensesRecipe,
            Sync([dialogStorage] { dialogStorage->m_dialog->setQuestionVisible(false); }),
            continuationRecipe
        }.withCancel(onCancelSetup)
    };
    m_taskTreeRunner.start(root, {}, [this](DoneWith) {
        QMetaObject::invokeMethod(&m_sdkManager, &AndroidSdkManager::reloadPackages,
                                  Qt::QueuedConnection);
    });
}

void AndroidSdkManager::runInstallationChange(const InstallationChange &change,
                                              const QString &extraMessage)
{
    QString message = Tr::tr("%n Android SDK packages shall be updated.", "", change.count());
    if (!extraMessage.isEmpty())
        message.prepend(extraMessage + "\n\n");

    QMessageBox messageDlg(QMessageBox::Information, Tr::tr("Android SDK Changes"),
                           message, QMessageBox::Ok | QMessageBox::Cancel,
                           Core::ICore::dialogParent());

    QString details;
    if (!change.toUninstall.isEmpty()) {
        QStringList toUninstall = {Tr::tr("[Packages to be uninstalled:]")};
        toUninstall += change.toUninstall;
        details += toUninstall.join("\n   ");
    }
    if (!change.toInstall.isEmpty()) {
        if (!change.toUninstall.isEmpty())
            details.append("\n\n");
        QStringList toInstall = {Tr::tr("[Packages to be installed:]")};
        toInstall += change.toInstall;
        details += toInstall.join("\n   ");
    }
    messageDlg.setDetailedText(details);
    if (messageDlg.exec() == QMessageBox::Cancel)
        return;

    const Storage<DialogStorage> dialogStorage;
    m_d->runDialogRecipe(dialogStorage,
        change.toInstall.count() ? licensesRecipe(dialogStorage) : nullItem,
        installationRecipe(dialogStorage, change));
}

void AndroidSdkManager::runUpdate()
{
    const Storage<DialogStorage> dialogStorage;
    m_d->runDialogRecipe(dialogStorage, licensesRecipe(dialogStorage), updateRecipe(dialogStorage));
}

} // namespace Android::Internal

#include "androidsdkmanager.moc"
