// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalsettings.h"

#include "cococommon.h"
#include "cocopluginconstants.h"
#include "cocotr.h"

#include <utils/fancylineedit.h>
#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

using namespace Utils;

namespace Coco::Internal {

CocoSettings &cocoSettings()
{
    static CocoSettings theCocoSettings;
    return theCocoSettings;
}

CocoSettings::CocoSettings()
{
    m_errorMessage = Tr::tr("Error: Coco installation directory not set. (This can't happen.)");

    setAutoApply(false);

    cocoPath.setSettingsKey(Constants::COCO_SETTINGS_GROUP, Constants::COCO_DIR_KEY);
    cocoPath.setExpectedKind(Utils::PathChooser::ExistingDirectory);
    cocoPath.setPromptDialogTitle(Tr::tr("Coco Installation Directory"));

    readSettings();

    if (cocoPath().isEmpty()) {
        findDefaultDirectory();
        writeSettings();
    }

    setDirectoryVars(cocoPath());
}

FilePath CocoSettings::coverageBrowserPath() const
{
    QString browserPath;

    if (HostOsInfo::isAnyUnixHost())
        browserPath = "bin/coveragebrowser";
    else if (HostOsInfo::isMacHost())
        browserPath = "coveragebrowser";
    else
        browserPath = "coveragebrowser.exe";

    return cocoPath().resolvePath(browserPath);
}

void CocoSettings::setDirectoryVars(const FilePath &dir)
{
    if (isCocoDirectory(dir) && verifyCocoDirectory(dir)) {
        cocoPath.setValue(dir);
        m_isValid = true;
        m_errorMessage.clear();
    } else {
        cocoPath.setValue(FilePath());
        m_isValid = false;
        m_errorMessage
            = Tr::tr("Error: Coco installation directory not found at \"%1\".").arg(dir.nativePath());
    }

    writeSettings();
    emit updateCocoDir();
}

void CocoSettings::apply()
{
    AspectContainer::apply();
    setDirectoryVars(cocoPath());
}

static FilePath coverageScannerPath(const FilePath &cocoDir)
{
    QString scannerPath;

    if (HostOsInfo::isAnyUnixHost()  || HostOsInfo::isMacHost())
        scannerPath = "bin/coveragescanner";
    else
        scannerPath = "coveragescanner.exe";

    return cocoDir.resolvePath(scannerPath);
}

bool CocoSettings::isCocoDirectory(const Utils::FilePath &cocoDir) const
{
    return coverageScannerPath(cocoDir).exists();
}

void CocoSettings::logError(const QString &msg)
{
    logFlashing(msg);
    m_isValid = false;
    m_errorMessage = msg;
}

bool CocoSettings::verifyCocoDirectory(const FilePath &cocoDir)
{
    QString coveragescanner = coverageScannerPath(cocoDir).nativePath();

    QProcess proc;
    proc.setProgram(coveragescanner);
    proc.setArguments({"--cs-help"});
    proc.start();

    if (!proc.waitForStarted()) {
        logError(Tr::tr("Error: CoverageScanner at \"%1\" did not start.").arg(coveragescanner));
        return false;
    }

    if (!proc.waitForFinished()) {
        logError(Tr::tr("Error: CoverageScanner at \"%1\" did not finish.").arg(coveragescanner));
        return false;
    }

    QString result = QString::fromLatin1(proc.readAll());
    static const QRegularExpression linebreak("\n|\r\n|\r");
    const QStringList lines = result.split(linebreak, Qt::SkipEmptyParts);

    const qsizetype n = lines.size();
    if (n >= 2 && lines[n - 2].startsWith("Version:") && lines[n - 1].startsWith("Date:")) {
        logSilently(Tr::tr("Valid CoverageScanner found at \"%1\":").arg(coveragescanner));
        logSilently("   " + lines[n - 2]);
        logSilently("   " + lines[n - 1]);
        return true;
    } else {
        logError(
            Tr::tr("Error: CoverageScanner at \"%1\" did not run correctly.").arg(coveragescanner));
        for (const QString &l : lines) {
            logSilently(l);
        }
        return false;
    }
}

bool CocoSettings::isValid() const
{
    return m_isValid;
}

QString CocoSettings::errorMessage() const
{
    return m_errorMessage;
}

void CocoSettings::tryPath(const QString &path)
{
    if (m_isValid)
        return;

    const auto fpath = Utils::FilePath::fromString(path);
    const QString nativePath = fpath.nativePath();
    if (isCocoDirectory(fpath)) {
        logSilently(Tr::tr("Found Coco directory \"%1\".").arg(nativePath));
        setDirectoryVars(fpath);
    } else {
        logSilently(Tr::tr("Checked Coco directory \"%1\".").arg(nativePath));
    }
}

static QString envVar(const QString &var)
{
    return QProcessEnvironment::systemEnvironment().value(var);
}

void CocoSettings::findDefaultDirectory()
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

class GlobalSettingsWidget : public QFrame
{
public:
    GlobalSettingsWidget();

    void apply();
    void cancel();

private:
    Utils::TextDisplay m_messageLabel;
};

GlobalSettingsWidget::GlobalSettingsWidget()
{
    using namespace Layouting;
    Form {
        Column {
            Row { Tr::tr("Coco Directory"), cocoSettings().cocoPath },
            Row { m_messageLabel }
        }
    }.attachTo(this);

    connect(&cocoSettings(), &CocoSettings::updateCocoDir, this, [this] {
        m_messageLabel.setText(cocoSettings().errorMessage());
        if (cocoSettings().isValid())
            m_messageLabel.setIconType(Utils::InfoLabel::None);
        else
            m_messageLabel.setIconType(Utils::InfoLabel::Error);
    });
}

void GlobalSettingsWidget::apply()
{
    cocoSettings().apply();
}

void GlobalSettingsWidget::cancel()
{
    cocoSettings().cancel();
}

class GlobalSettingsPage : public Core::IOptionsPage
{
public:
    GlobalSettingsWidget *widget() override;
    void apply() override;
    void cancel() override;
    void finish() override;

    GlobalSettingsPage();

    QPointer<GlobalSettingsWidget> m_widget;
};

GlobalSettingsPage::GlobalSettingsPage()
    : m_widget(nullptr)
{
    setId(Constants::COCO_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Coco"));
    setCategory("I.Coco"); // Category I contains also the C++ settings.
}

GlobalSettingsWidget *GlobalSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new GlobalSettingsWidget;
    return m_widget;
}

void GlobalSettingsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void GlobalSettingsPage::cancel()
{
    if (m_widget)
        m_widget->cancel();
}

void GlobalSettingsPage::finish()
{
    delete m_widget;
}

void setupCocoSettings()
{
    static GlobalSettingsPage theGlobalSettingsPage;
}

} // namespace Coco::Internal
