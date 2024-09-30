// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocoinstallation.h"

#include "cocotr.h"
#include "common.h"
#include "globalsettings.h"

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>

namespace Coco::Internal {

struct CocoInstallationPrivate
{
    Utils::FilePath cocoPath;
    bool isValid = false;
    QString errorMessage = Tr::tr("Error: Coco installation directory not set. (This can't happen.)");
};

CocoInstallationPrivate *CocoInstallation::d = nullptr;

CocoInstallation::CocoInstallation()
{
    if (!d)
        d = new CocoInstallationPrivate;
}

Utils::FilePath CocoInstallation::directory() const
{
    return d->cocoPath;
}

Utils::FilePath CocoInstallation::coverageBrowserPath() const
{
    QString browserPath;

    if (Utils::HostOsInfo::isAnyUnixHost() || Utils::HostOsInfo::isMacHost())
        browserPath = "bin/coveragebrowser";
    else
        browserPath = "coveragebrowser.exe";

    return d->cocoPath.resolvePath(browserPath);
}

void CocoInstallation::setDirectory(const Utils::FilePath &dir)
{
    if (isCocoDirectory(dir)) {
        d->cocoPath = dir;
        d->isValid = true;
        d->errorMessage = "";
        verifyCocoDirectory();
    }
    else {
        d->cocoPath = Utils::FilePath();
        d->isValid = false;
        d->errorMessage
            = Tr::tr("Error: Coco installation directory not found at \"%1\".").arg(dir.nativePath());
    }
}

Utils::FilePath CocoInstallation::coverageScannerPath(const Utils::FilePath &cocoDir) const
{
    QString scannerPath;

    if (Utils::HostOsInfo::isAnyUnixHost()  || Utils::HostOsInfo::isMacHost())
        scannerPath = "bin/coveragescanner";
    else
        scannerPath = "coveragescanner.exe";

    return cocoDir.resolvePath(scannerPath);
}

bool CocoInstallation::isCocoDirectory(const Utils::FilePath &cocoDir) const
{
    return coverageScannerPath(cocoDir).exists();
}

void CocoInstallation::logError(const QString &msg)
{
    logFlashing(msg);
    d->isValid = false;
    d->errorMessage = msg;
}

bool CocoInstallation::verifyCocoDirectory()
{
    QString coveragescanner = coverageScannerPath(d->cocoPath).nativePath();

    QProcess proc;
    proc.setProgram(coveragescanner);
    proc.setArguments({"--cs-help"});
    proc.start();

    if (!proc.waitForStarted()) {
        logError(Tr::tr("Error: Coveragescanner at \"%1\" did not start.").arg(coveragescanner));
        return false;
    }

    if (!proc.waitForFinished()) {
        logError(Tr::tr("Error: Coveragescanner at \"%1\" did not finish.").arg(coveragescanner));
        return false;
    }

    QString result = QString::fromLatin1(proc.readAll());
    static const QRegularExpression linebreak("\n|\r\n|\r");
    QStringList lines = result.split(linebreak, Qt::SkipEmptyParts);

    const qsizetype n = lines.size();
    if (n >= 2 && lines[n - 2].startsWith("Version:") && lines[n - 1].startsWith("Date:")) {
        logSilently(Tr::tr("Valid CoverageScanner found at \"%1\":").arg(coveragescanner));
        logSilently("   " + lines[n - 2]);
        logSilently("   " + lines[n - 1]);
        return true;
    } else {
        logError(
            Tr::tr("Error: Coveragescanner at \"%1\" did not run correctly.").arg(coveragescanner));
        for (const QString &l : lines) {
            logSilently(l);
        }
        return false;
    }
}

bool CocoInstallation::isValid() const
{
    return d->isValid;
}

QString CocoInstallation::errorMessage() const
{
    return d->errorMessage;
}

void CocoInstallation::tryPath(const QString &path)
{
    if (d->isValid)
        return;

    const auto fpath = Utils::FilePath::fromString(path);
    const QString nativePath = fpath.nativePath();
    if (isCocoDirectory(fpath)) {
        logSilently(Tr::tr("Found Coco directory \"%1\".").arg(nativePath));
        setDirectory(fpath);
        GlobalSettings::save();
    } else
        logSilently(Tr::tr("Checked Coco directory \"%1\".").arg(nativePath));
}

QString CocoInstallation::envVar(const QString &var) const
{
    return QProcessEnvironment::systemEnvironment().value(var);
}

void CocoInstallation::findDefaultDirectory()
{
    if (Utils::HostOsInfo::isMacHost())
        tryPath("/Applications/SquishCoco");
    else if (Utils::HostOsInfo::isAnyUnixHost()) {
        tryPath((Utils::FileUtils::homePath() / "SquishCoco").nativePath());
        tryPath("/opt/SquishCoco");
    } else {
        tryPath(envVar("SQUISHCOCO"));
        QStringList homeDirs = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        if (!homeDirs.isEmpty())
            tryPath(homeDirs[0] + "/squishcoco");
        tryPath(envVar("ProgramFiles") + "\\squishcoco");
        tryPath(envVar("ProgramFiles(x86)") + "\\squishcoco");
    }
}

} // namespace Coco::Internal
