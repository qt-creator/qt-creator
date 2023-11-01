// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidsdkmanager.h"
#include "androidtr.h"
#include "avdmanageroutputparser.h"
#include "sdkmanageroutputparser.h"

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QTextCodec>

namespace {
Q_LOGGING_CATEGORY(sdkManagerLog, "qtc.android.sdkManager", QtWarningMsg)
const char commonArgsKey[] = "Common Arguments:";
}

using namespace Utils;

namespace Android {
namespace Internal {


const int sdkManagerCmdTimeoutS = 60;
const int sdkManagerOperationTimeoutS = 600;

using SdkCmdPromise = QPromise<AndroidSdkManager::OperationOutput>;

static const QRegularExpression &assertionRegExp()
{
    static const QRegularExpression theRegExp
        (R"((\(\s*y\s*[\/\\]\s*n\s*\)\s*)(?<mark>[\:\?]))", // (y/N)?
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);

    return theRegExp;
}

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

static QString sdkRootArg(const AndroidConfig &config)
{
    return "--sdk_root=" + config.sdkLocation().toString();
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
    proc.setTimeoutS(timeout);
    proc.setTimeOutMessageBoxEnabled(true);
    proc.setCommand({config.sdkManagerToolPath(), newArgs});
    proc.runBlocking(EventLoopMode::On);
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
    proc.setTimeoutS(timeout);
    proc.setStdOutCallback([offset, progressQuota, &proc, &assertionFound, &promise](const QString &out) {
        int progressPercent = parseProgress(out, assertionFound);
        if (assertionFound) {
            proc.stop();
            proc.waitForFinished();
        }
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
    proc.runBlocking(EventLoopMode::On);
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
    AndroidSdkManagerPrivate(AndroidSdkManager &sdkManager, const AndroidConfig &config);
    ~AndroidSdkManagerPrivate();

    AndroidSdkPackageList filteredPackages(AndroidSdkPackage::PackageState state,
                                           AndroidSdkPackage::PackageType type,
                                           bool forceUpdate = false);
    const AndroidSdkPackageList &allPackages(bool forceUpdate = false);
    void refreshSdkPackages(bool forceReload = false);

    void parseCommonArguments(QPromise<QString> &promise);
    void updateInstalled(SdkCmdPromise &fi);
    void update(SdkCmdPromise &fi, const QStringList &install,
                const QStringList &uninstall);
    void checkPendingLicense(SdkCmdPromise &fi);
    void getPendingLicense(SdkCmdPromise &fi);

    void addWatcher(const QFuture<AndroidSdkManager::OperationOutput> &future);
    void setLicenseInput(bool acceptLicense);

    std::unique_ptr<QFutureWatcher<void>, decltype(&watcherDeleter)> m_activeOperation;

private:
    QByteArray getUserInput() const;
    void clearUserInput();
    void reloadSdkPackages();
    void clearPackages();
    bool onLicenseStdOut(const QString &output, bool notify,
                         AndroidSdkManager::OperationOutput &result, SdkCmdPromise &fi);

    AndroidSdkManager &m_sdkManager;
    const AndroidConfig &m_config;
    AndroidSdkPackageList m_allPackages;
    FilePath lastSdkManagerPath;
    QString m_licenseTextCache;
    QByteArray m_licenseUserInput;
    mutable QReadWriteLock m_licenseInputLock;

public:
    bool m_packageListingSuccessful = false;
};

AndroidSdkManager::AndroidSdkManager(const AndroidConfig &config):
    m_d(new AndroidSdkManagerPrivate(*this, config))
{
}

AndroidSdkManager::~AndroidSdkManager()
{
    cancelOperatons();
}

SdkPlatformList AndroidSdkManager::installedSdkPlatforms()
{
    AndroidSdkPackageList list = m_d->filteredPackages(AndroidSdkPackage::Installed,
                                                       AndroidSdkPackage::SdkPlatformPackage);
    return Utils::static_container_cast<SdkPlatform *>(list);
}

const AndroidSdkPackageList &AndroidSdkManager::allSdkPackages()
{
    return m_d->allPackages();
}

AndroidSdkPackageList AndroidSdkManager::installedSdkPackages()
{
    return m_d->filteredPackages(AndroidSdkPackage::Installed, AndroidSdkPackage::AnyValidType);
}

SystemImageList AndroidSdkManager::installedSystemImages()
{
    AndroidSdkPackageList list = m_d->filteredPackages(AndroidSdkPackage::AnyValidState,
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
    AndroidSdkPackageList list = m_d->filteredPackages(AndroidSdkPackage::Installed,
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

void AndroidSdkManager::reloadPackages(bool forceReload)
{
    m_d->refreshSdkPackages(forceReload);
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

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::updateAll()
{
    if (isBusy()) {
        return QFuture<AndroidSdkManager::OperationOutput>();
    }
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::updateInstalled, m_d.get());
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput>
AndroidSdkManager::update(const QStringList &install, const QStringList &uninstall)
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::update, m_d.get(), install, uninstall);
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::checkPendingLicenses()
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::checkPendingLicense, m_d.get());
    m_d->addWatcher(future);
    return future;
}

QFuture<AndroidSdkManager::OperationOutput> AndroidSdkManager::runLicenseCommand()
{
    if (isBusy())
        return QFuture<AndroidSdkManager::OperationOutput>();
    auto future = Utils::asyncRun(&AndroidSdkManagerPrivate::getPendingLicense, m_d.get());
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

AndroidSdkManagerPrivate::AndroidSdkManagerPrivate(AndroidSdkManager &sdkManager,
                                                   const AndroidConfig &config):
    m_activeOperation(nullptr, watcherDeleter),
    m_sdkManager(sdkManager),
    m_config(config)
{
}

AndroidSdkManagerPrivate::~AndroidSdkManagerPrivate()
{
    clearPackages();
}

AndroidSdkPackageList
AndroidSdkManagerPrivate::filteredPackages(AndroidSdkPackage::PackageState state,
                                           AndroidSdkPackage::PackageType type, bool forceUpdate)
{
    refreshSdkPackages(forceUpdate);
    return Utils::filtered(m_allPackages, [state, type](const AndroidSdkPackage *p) {
       return p->state() & state && p->type() & type;
    });
}

const AndroidSdkPackageList &AndroidSdkManagerPrivate::allPackages(bool forceUpdate)
{
    refreshSdkPackages(forceUpdate);
    return m_allPackages;
}

void AndroidSdkManagerPrivate::reloadSdkPackages()
{
    emit m_sdkManager.packageReloadBegin();
    clearPackages();

    lastSdkManagerPath = m_config.sdkManagerToolPath();
    m_packageListingSuccessful = false;

    if (m_config.sdkToolsVersion().isNull()) {
        // Configuration has invalid sdk path or corrupt installation.
        emit m_sdkManager.packageReloadFinished();
        return;
    }

    QString packageListing;
    QStringList args({"--list", "--verbose"});
    args << m_config.sdkManagerToolArgs();
    m_packageListingSuccessful = sdkManagerCommand(m_config, args, &packageListing);
    if (m_packageListingSuccessful) {
        SdkManagerOutputParser parser(m_allPackages);
        parser.parsePackageListing(packageListing);
    }
    emit m_sdkManager.packageReloadFinished();
}

void AndroidSdkManagerPrivate::refreshSdkPackages(bool forceReload)
{
    // Sdk path changed. Updated packages.
    // QTC updates the package listing only
    if (m_config.sdkManagerToolPath() != lastSdkManagerPath || forceReload)
        reloadSdkPackages();
}

void AndroidSdkManagerPrivate::updateInstalled(SdkCmdPromise &promise)
{
    promise.setProgressRange(0, 100);
    promise.setProgressValue(0);
    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::UpdateAll;
    result.stdOutput = Tr::tr("Updating installed packages.");
    promise.addResult(result);
    QStringList args("--update");
    args << m_config.sdkManagerToolArgs();
    if (!promise.isCanceled())
        sdkManagerCommand(m_config, args, m_sdkManager, promise, result, 100);
    else
        qCDebug(sdkManagerLog) << "Update: Operation cancelled before start";

    if (result.stdError.isEmpty() && !result.success)
        result.stdError = Tr::tr("Failed.");
    result.stdOutput = Tr::tr("Done") + "\n\n";
    promise.addResult(result);
    promise.setProgressValue(100);
}

void AndroidSdkManagerPrivate::update(SdkCmdPromise &fi, const QStringList &install,
                                      const QStringList &uninstall)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    double progressQuota = 100.0 / (install.count() + uninstall.count());
    int currentProgress = 0;

    QString installTag = Tr::tr("Installing");
    QString uninstallTag = Tr::tr("Uninstalling");

    auto doOperation = [&](const QString& packagePath, const QStringList& args,
            bool isInstall) {
        AndroidSdkManager::OperationOutput result;
        result.type = AndroidSdkManager::UpdatePackage;
        result.stdOutput = QString("%1 %2").arg(isInstall ? installTag : uninstallTag)
                .arg(packagePath);
        fi.addResult(result);
        if (fi.isCanceled())
            qCDebug(sdkManagerLog) << args << "Update: Operation cancelled before start";
        else
            sdkManagerCommand(m_config, args, m_sdkManager, fi, result, progressQuota, isInstall);
        currentProgress += progressQuota;
        fi.setProgressValue(currentProgress);
        if (result.stdError.isEmpty() && !result.success)
            result.stdError = Tr::tr("Failed");
        result.stdOutput = Tr::tr("Done") + "\n\n";
        fi.addResult(result);
        return fi.isCanceled();
    };


    // Uninstall packages
    for (const QString &sdkStylePath : uninstall) {
        // Uninstall operations are not interptible. We don't want to leave half uninstalled.
        QStringList args;
        args << "--uninstall" << sdkStylePath << m_config.sdkManagerToolArgs();
        if (doOperation(sdkStylePath, args, false))
            break;
    }

    // Install packages
    for (const QString &sdkStylePath : install) {
        QStringList args(sdkStylePath);
        args << m_config.sdkManagerToolArgs();
        if (doOperation(sdkStylePath, args, true))
            break;
    }
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::checkPendingLicense(SdkCmdPromise &fi)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);
    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::LicenseCheck;
    const QStringList args = {"--licenses", sdkRootArg(m_config)};
    if (!fi.isCanceled()) {
        const int timeOutS = 4; // Short timeout as workaround for QTCREATORBUG-25667
        sdkManagerCommand(m_config, args, m_sdkManager, fi, result, 100.0, true, timeOutS);
    } else {
        qCDebug(sdkManagerLog) << "Update: Operation cancelled before start";
    }

    fi.addResult(result);
    fi.setProgressValue(100);
}

void AndroidSdkManagerPrivate::getPendingLicense(SdkCmdPromise &fi)
{
    fi.setProgressRange(0, 100);
    fi.setProgressValue(0);

    AndroidSdkManager::OperationOutput result;
    result.type = AndroidSdkManager::LicenseWorkflow;

    Process licenseCommand;
    licenseCommand.setProcessMode(ProcessMode::Writer);
    licenseCommand.setEnvironment(m_config.toolsEnvironment());
    bool reviewingLicenses = false;
    licenseCommand.setCommand(CommandLine(m_config.sdkManagerToolPath(), {"--licenses", sdkRootArg(m_config)}));
    licenseCommand.setUseCtrlCStub(true);
    licenseCommand.start();
    QTextCodec *codec = QTextCodec::codecForLocale();
    int inputCounter = 0, steps = -1;
    while (!licenseCommand.waitForFinished(200)) {
        QString stdOut = codec->toUnicode(licenseCommand.readAllRawStandardOutput());
        bool assertionFound = false;
        if (!stdOut.isEmpty())
            assertionFound = onLicenseStdOut(stdOut, reviewingLicenses, result, fi);

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
        } else if (assertionFound) {
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
            if (!licenseCommand.waitForFinished(300)) {
                licenseCommand.kill();
                licenseCommand.waitForFinished(200);
            }
        }
        if (licenseCommand.state() == QProcess::NotRunning)
            break;
    }

    m_licenseTextCache.clear();
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

bool AndroidSdkManagerPrivate::onLicenseStdOut(const QString &output, bool notify,
                                               AndroidSdkManager::OperationOutput &result,
                                               SdkCmdPromise &fi)
{
    m_licenseTextCache.append(output);
    const QRegularExpressionMatch assertionMatch = assertionRegExp().match(m_licenseTextCache);
    if (assertionMatch.hasMatch()) {
        if (notify) {
            result.stdOutput = m_licenseTextCache;
            fi.addResult(result);
        }
        // Clear the current contents. The found license text is dispatched. Continue collecting the
        // next license text.
        m_licenseTextCache.clear();
        return true;
    }
    return false;
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
    sdkManagerCommand(m_config, QStringList("--help"), &output);
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
