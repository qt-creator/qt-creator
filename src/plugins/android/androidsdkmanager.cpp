// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidsdkmanager.h"
#include "androidtr.h"
#include "sdkmanageroutputparser.h"

#include <coreplugin/icore.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFutureWatcher>
#include <QDialogButtonBox>
#include <QLoggingCategory>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QTextCodec>

namespace {
Q_LOGGING_CATEGORY(sdkManagerLog, "qtc.android.sdkManager", QtWarningMsg)
const char commonArgsKey[] = "Common Arguments:";
}

using namespace Tasking;
using namespace Utils;

using namespace std::chrono;
using namespace std::chrono_literals;

namespace Android {
namespace Internal {

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

static QString sdkRootArg(const AndroidConfig &config)
{
    return "--sdk_root=" + config.sdkLocation().toString();
}

static const QRegularExpression &assertionRegExp()
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
        process.setEnvironment(androidConfig().toolsEnvironment());
        process.setCommand(CommandLine(androidConfig().sdkManagerToolPath(),
                                       {"--licenses", sdkRootArg(androidConfig())}));
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

const int sdkManagerCmdTimeoutS = 60;
const int sdkManagerOperationTimeoutS = 600;

using SdkCmdPromise = QPromise<AndroidSdkManager::OperationOutput>;

int parseProgress(const QString &out, bool &foundAssertion)
{
    int progress = -1;
    if (out.isEmpty())
        return progress;
    static const QRegularExpression reg("(?<progress>\\d*)%");
    static const QRegularExpression regEndOfLine("[\\n\\r]");
    const QStringList lines = out.split(regEndOfLine, Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QRegularExpressionMatch match = reg.match(line);
        if (match.hasMatch()) {
            progress = match.captured("progress").toInt();
            if (progress < 0 || progress > 100)
                progress = -1;
        }
        if (!foundAssertion)
            foundAssertion = assertionRegExp().match(line).hasMatch();
    }
    return progress;
}

void watcherDeleter(QFutureWatcher<void> *watcher)
{
    if (!watcher->isFinished() && !watcher->isCanceled())
        watcher->cancel();

    if (!watcher->isFinished())
        watcher->waitForFinished();

    delete watcher;
}

/*!
    Runs the \c sdkmanger tool with arguments \a args. Returns \c true if the command is
    successfully executed. Output is copied into \a output. The function blocks the calling thread.
 */
static bool sdkManagerCommand(const AndroidConfig &config, const QStringList &args,
                              QString *output, int timeout = sdkManagerCmdTimeoutS)
{
    QStringList newArgs = args;
    newArgs.append(sdkRootArg(config));
    qCDebug(sdkManagerLog).noquote() << "Running SDK Manager command (sync):"
                                     << CommandLine(config.sdkManagerToolPath(), newArgs)
                                        .toUserOutput();
    Process proc;
    proc.setEnvironment(config.toolsEnvironment());
    proc.setTimeOutMessageBoxEnabled(true);
    proc.setCommand({config.sdkManagerToolPath(), newArgs});
    proc.runBlocking(seconds(timeout), EventLoopMode::On);
    if (output)
        *output = proc.allOutput();
    return proc.result() == ProcessResult::FinishedWithSuccess;
}

/*!
    Runs the \c sdkmanger tool with arguments \a args. The operation command progress is updated in
    to the future interface \a fi and \a output is populated with command output. The command listens
    to cancel signal emmitted by \a sdkManager and kill the commands. The command is also killed
    after the lapse of \a timeout seconds. The function blocks the calling thread.
 */
static void sdkManagerCommand(const AndroidConfig &config, const QStringList &args,
                              AndroidSdkManager &sdkManager, SdkCmdPromise &promise,
                              AndroidSdkManager::OperationOutput &output, double progressQuota,
                              bool interruptible = true, int timeout = sdkManagerOperationTimeoutS)
{
    QStringList newArgs = args;
    newArgs.append(sdkRootArg(config));
    qCDebug(sdkManagerLog).noquote() << "Running SDK Manager command (async):"
                                     << CommandLine(config.sdkManagerToolPath(), newArgs)
                                        .toUserOutput();
    int offset = promise.future().progressValue();
    Process proc;
    proc.setEnvironment(config.toolsEnvironment());
    bool assertionFound = false;
    proc.setStdOutCallback([offset, progressQuota, &proc, &assertionFound, &promise](const QString &out) {
        int progressPercent = parseProgress(out, assertionFound);
        if (assertionFound)
            proc.stop();
        if (progressPercent != -1)
            promise.setProgressValue(offset + qRound((progressPercent / 100.0) * progressQuota));
    });
    proc.setStdErrCallback([&output](const QString &err) {
        output.stdError = err;
    });
    if (interruptible) {
        QObject::connect(&sdkManager, &AndroidSdkManager::cancelActiveOperations, &proc, [&proc] {
           proc.stop();
           proc.waitForFinished();
        });
    }
    proc.setCommand({config.sdkManagerToolPath(), newArgs});
    proc.runBlocking(seconds(timeout), EventLoopMode::On);
    if (assertionFound) {
        output.success = false;
        output.stdOutput = proc.cleanedStdOut();
        output.stdError = Tr::tr("The operation requires user interaction. "
                                 "Use the \"sdkmanager\" command-line tool.");
    } else {
        output.success = proc.result() == ProcessResult::FinishedWithSuccess;
    }
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

    void parseCommonArguments(QPromise<QString> &promise);
    void updateInstalled(SdkCmdPromise &fi);
    void updatePackages(SdkCmdPromise &fi, const InstallationChange &change);
    void licenseCheck(SdkCmdPromise &fi);
    void licenseWorkflow(SdkCmdPromise &fi);

    void addWatcher(const QFuture<AndroidSdkManager::OperationOutput> &future);
    void setLicenseInput(bool acceptLicense);

    std::unique_ptr<QFutureWatcher<void>, decltype(&watcherDeleter)> m_activeOperation;

    QByteArray getUserInput() const;
    void clearUserInput();
    void reloadSdkPackages();
    void clearPackages();

    AndroidSdkManager &m_sdkManager;
    AndroidSdkPackageList m_allPackages;
    FilePath lastSdkManagerPath;
    QByteArray m_licenseUserInput;
    mutable QReadWriteLock m_licenseInputLock;

public:
    bool m_packageListingSuccessful = false;
};

AndroidSdkManager::AndroidSdkManager()
    : m_d(new AndroidSdkManagerPrivate(*this))
{
}

AndroidSdkManager::~AndroidSdkManager()
{
    cancelOperatons();
}

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
    QStringList essentials = androidConfig().allEssentials();
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
    const QStringList essentials = androidConfig().allEssentials();
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
    QList<SdkPlatform *> platforms = Utils::static_container_cast<SdkPlatform *>(list);

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
    if (androidConfig().sdkManagerToolPath() != m_d->lastSdkManagerPath)
        reloadPackages();
}

void AndroidSdkManager::reloadPackages()
{
    m_d->reloadSdkPackages();
}

bool AndroidSdkManager::isBusy() const
{
    return m_d->m_activeOperation && !m_d->m_activeOperation->isFinished();
}

bool AndroidSdkManager::packageListingSuccessful() const
{
    return m_d->m_packageListingSuccessful;
}

QFuture<QString> AndroidSdkManager::availableArguments() const
{
    return Utils::asyncRun(&AndroidSdkManagerPrivate::parseCommonArguments, m_d.get());
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::updateInstalled()
{
    if (isBusy()) {
        return QFuture<AndroidSdkManager::OperationOutput>();
    }
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::updateInstalled, m_d.get());
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::updatePackages(const InstallationChange &change)
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::updatePackages, m_d.get(), change);
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::licenseCheck()
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::licenseCheck, m_d.get());
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::licenseWorkflow()
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::licenseWorkflow, m_d.get());
    m_d->addWatcher(future);
    return future;
}

void AndroidSdkManager::cancelOperatons()
{
    emit cancelActiveOperations();
    m_d->m_activeOperation.reset();
}

void AndroidSdkManager::acceptSdkLicense(bool accept)
{
    m_d->setLicenseInput(accept);
}

AndroidSdkManagerPrivate::AndroidSdkManagerPrivate(AndroidSdkManager &sdkManager):
    m_activeOperation(nullptr, watcherDeleter),
    m_sdkManager(sdkManager)
{}

AndroidSdkManagerPrivate::~AndroidSdkManagerPrivate()
{
    clearPackages();
}

const AndroidSdkPackageList &AndroidSdkManagerPrivate::allPackages()
{
    m_sdkManager.refreshPackages();
    return m_allPackages;
}

void AndroidSdkManagerPrivate::reloadSdkPackages()
{
    emit m_sdkManager.packageReloadBegin();
    clearPackages();

    lastSdkManagerPath = androidConfig().sdkManagerToolPath();
    m_packageListingSuccessful = false;

    if (androidConfig().sdkToolsVersion().isNull()) {
        // Configuration has invalid sdk path or corrupt installation.
        emit m_sdkManager.packageReloadFinished();
        return;
    }

    QString packageListing;
    QStringList args({"--list", "--verbose"});
    args << androidConfig().sdkManagerToolArgs();
    m_packageListingSuccessful = sdkManagerCommand(androidConfig(), args, &packageListing);
    if (m_packageListingSuccessful) {
        SdkManagerOutputParser parser(m_allPackages);
        parser.parsePackageListing(packageListing);
    } else {
        qCWarning(sdkManagerLog) << "Failed parsing packages:" << packageListing;
    }

    emit m_sdkManager.packageReloadFinished();
}

void AndroidSdkManagerPrivate::updateInstalled(SdkCmdPromise &promise)
{
    promise.setProgressRange(0, 100);
    promise.setProgressValue(0);
    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::UpdateInstalled;
    result.stdOutput = Tr::tr("Updating installed packages.");
    promise.addResult(result);
    QStringList args("--update");
    args << androidConfig().sdkManagerToolArgs();
    if (!promise.isCanceled())
        sdkManagerCommand(androidConfig(), args, m_sdkManager, promise, result, 100);
    else
        qCDebug(sdkManagerLog) << "Update: Operation cancelled before start";

    if (result.stdError.isEmpty() && !result.success)
        result.stdError = Tr::tr("Failed.");
    result.stdOutput = Tr::tr("Done") + "\n\n";
    promise.addResult(result);
    promise.setProgressValue(100);
}

void AndroidSdkManagerPrivate::updatePackages(SdkCmdPromise &fi, const InstallationChange &change)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    double progressQuota = 100.0 / change.count();
    int currentProgress = 0;

    QString installTag = Tr::tr("Installing");
    QString uninstallTag = Tr::tr("Uninstalling");

    auto doOperation = [&](const QString& packagePath, const QStringList& args,
            bool isInstall) {
        AndroidSdkManager::OperationOutput result;
        result.type = AndroidSdkManager::UpdatePackages;
        result.stdOutput = QString("%1 %2").arg(isInstall ? installTag : uninstallTag)
                .arg(packagePath);
        fi.addResult(result);
        if (fi.isCanceled())
            qCDebug(sdkManagerLog) << args << "Update: Operation cancelled before start";
        else
            sdkManagerCommand(androidConfig(), args, m_sdkManager, fi, result, progressQuota, isInstall);
        currentProgress += progressQuota;
        fi.setProgressValue(currentProgress);
        if (result.stdError.isEmpty() && !result.success)
            result.stdError = Tr::tr("Failed");
        result.stdOutput = Tr::tr("Done") + "\n\n";
        fi.addResult(result);
        return fi.isCanceled();
    };


    // Uninstall packages
    for (const QString &sdkStylePath : change.toUninstall) {
        // Uninstall operations are not interptible. We don't want to leave half uninstalled.
        QStringList args;
        args << "--uninstall" << sdkStylePath << androidConfig().sdkManagerToolArgs();
        if (doOperation(sdkStylePath, args, false))
            break;
    }

    // Install packages
    for (const QString &sdkStylePath : change.toInstall) {
        QStringList args(sdkStylePath);
        args << androidConfig().sdkManagerToolArgs();
        if (doOperation(sdkStylePath, args, true))
            break;
    }
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::licenseCheck(SdkCmdPromise &fi)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::LicenseCheck;
    if (!fi.isCanceled()) {
        const int timeOutS = 4; // Short timeout as workaround for QTCREATORBUG-25667
        sdkManagerCommand(androidConfig(), {"--licenses"}, m_sdkManager, fi, result, 100.0, true,
                          timeOutS);
    } else {
        qCDebug(sdkManagerLog) << "Update: Operation cancelled before start";
    }

    fi.addResult(result);
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::licenseWorkflow(SdkCmdPromise &fi)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);

    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::LicenseWorkflow;

    Process licenseCommand;
    licenseCommand.setProcessMode(ProcessMode::Writer);
    licenseCommand.setEnvironment(androidConfig().toolsEnvironment());
    bool reviewingLicenses = false;
    licenseCommand.setCommand(CommandLine(androidConfig().sdkManagerToolPath(),
                                          {"--licenses", sdkRootArg(androidConfig())}));
    licenseCommand.setUseCtrlCStub(true);
    licenseCommand.start();
    QTextCodec *codec = QTextCodec::codecForLocale();
    int inputCounter = 0, steps = -1;
    QString licenseTextCache;
    while (!licenseCommand.waitForFinished(200ms)) {
        const QString stdOut = codec->toUnicode(licenseCommand.readAllRawStandardOutput());
        bool assertion = false;
        if (!stdOut.isEmpty()) {
            licenseTextCache.append(stdOut);
            assertion = assertionRegExp().match(licenseTextCache).hasMatch();
            if (assertion) {
                if (reviewingLicenses) {
                    result.stdOutput = licenseTextCache;
                    fi.addResult(result);
                }
                licenseTextCache.clear();
            }
        }

        if (reviewingLicenses) {
            // Check user input
            QByteArray userInput = getUserInput();
            if (!userInput.isEmpty()) {
                clearUserInput();
                licenseCommand.writeRaw(userInput);
                ++inputCounter;
                if (steps != -1)
                    fi.setProgressValue(qRound((inputCounter / (double)steps) * 100));
            }
        } else if (assertion) {
            // The first assertion is to start reviewing licenses. Always accept.
            reviewingLicenses = true;
            static const QRegularExpression reg(R"((\d+\sof\s)(?<steps>\d+))");
            QRegularExpressionMatch match = reg.match(stdOut);
            if (match.hasMatch())
                steps = match.captured("steps").toInt();
            licenseCommand.write("Y\n");
        }

        if (fi.isCanceled()) {
            licenseCommand.terminate();
            if (!licenseCommand.waitForFinished(300ms)) {
                licenseCommand.kill();
                licenseCommand.waitForFinished(200ms);
            }
        }
        if (licenseCommand.state() == QProcess::NotRunning)
            break;
    }

    result.success = licenseCommand.exitStatus() == QProcess::NormalExit;
    if (!result.success)
        result.stdError = Tr::tr("License command failed.") + "\n\n";
    fi.addResult(result);
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::setLicenseInput(bool acceptLicense)
{
    QWriteLocker locker(&m_licenseInputLock);
    m_licenseUserInput = acceptLicense ? "Y\n" : "n\n";
}

QByteArray AndroidSdkManagerPrivate::getUserInput() const
{
    QReadLocker locker(&m_licenseInputLock);
    return m_licenseUserInput;
}

void AndroidSdkManagerPrivate::clearUserInput()
{
    QWriteLocker locker(&m_licenseInputLock);
    m_licenseUserInput.clear();
}

void AndroidSdkManagerPrivate::addWatcher(const QFuture<AndroidSdkManager::OperationOutput> &future)
{
    if (future.isFinished())
        return;
    m_activeOperation.reset(new QFutureWatcher<void>());
    m_activeOperation->setFuture(QFuture<void>(future));
}

void AndroidSdkManagerPrivate::parseCommonArguments(QPromise<QString> &promise)
{
    QString argumentDetails;
    QString output;
    sdkManagerCommand(androidConfig(), QStringList("--help"), &output);
    bool foundTag = false;
    const auto lines = output.split('\n');
    for (const QString& line : lines) {
        if (promise.isCanceled())
            break;
        if (foundTag)
            argumentDetails.append(line + "\n");
        else if (line.startsWith(commonArgsKey))
            foundTag = true;
    }

    if (!promise.isCanceled())
        promise.addResult(argumentDetails);
}

void AndroidSdkManagerPrivate::clearPackages()
{
    for (AndroidSdkPackage *p : std::as_const(m_allPackages))
        delete p;
    m_allPackages.clear();
}

} // namespace Internal
} // namespace Android

#include "androidsdkmanager.moc"
