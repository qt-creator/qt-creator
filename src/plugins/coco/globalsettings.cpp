// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalsettings.h"

#include "cococommon.h"
#include "cocopluginconstants.h"
#include "cocotr.h"

#include <coreplugin/icore.h>

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

static const char DIRECTORY[] = "CocoDirectory";

CocoSettings &cocoSettings()
{
    static CocoSettings theCocoSettings;
    return theCocoSettings;
}

CocoSettings::CocoSettings()
{
    m_errorMessage = Tr::tr("Error: Coco installation directory not set. (This can't happen.)");
}

void CocoSettings::read()
{
    bool directoryInSettings = false;

    QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::COCO_SETTINGS_GROUP);
    const QStringList keys = s->allKeys();
    for (const QString &keyString : keys) {
        Key key(keyString.toLatin1());
        if (key == DIRECTORY) {
            setDirectory(FilePath::fromUserInput(s->value(key).toString()));
            directoryInSettings = true;
        } else
            s->remove(key);
    }
    s->endGroup();

    if (!directoryInSettings)
       findDefaultDirectory();

    save();
}

void CocoSettings::save()
{
    QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::COCO_SETTINGS_GROUP);
    s->setValue(DIRECTORY, directory().toUserOutput());
    s->endGroup();
}

FilePath CocoSettings::directory() const
{
    return m_cocoPath;
}

FilePath CocoSettings::coverageBrowserPath() const
{
    QString browserPath;

    if (HostOsInfo::isAnyUnixHost() || HostOsInfo::isMacHost())
        browserPath = "bin/coveragebrowser";
    else
        browserPath = "coveragebrowser.exe";

    return m_cocoPath.resolvePath(browserPath);
}

void CocoSettings::setDirectory(const FilePath &dir)
{
    if (isCocoDirectory(dir)) {
        m_cocoPath = dir;
        m_isValid = true;
        m_errorMessage.clear();
        verifyCocoDirectory();
    } else {
        m_cocoPath = Utils::FilePath();
        m_isValid = false;
        m_errorMessage
            = Tr::tr("Error: Coco installation directory not found at \"%1\".").arg(dir.nativePath());
    }
}

FilePath CocoSettings::coverageScannerPath(const FilePath &cocoDir) const
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

bool CocoSettings::verifyCocoDirectory()
{
    QString coveragescanner = coverageScannerPath(m_cocoPath).nativePath();

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
        setDirectory(fpath);
        save();
    } else {
        logSilently(Tr::tr("Checked Coco directory \"%1\".").arg(nativePath));
    }
}

QString CocoSettings::envVar(const QString &var) const
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

GlobalSettingsWidget::GlobalSettingsWidget(QFrame *parent)
    : QFrame(parent)
{
    m_cocoPathAspect.setDefaultPathValue(cocoSettings().directory());
    m_cocoPathAspect.setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_cocoPathAspect.setPromptDialogTitle(Tr::tr("Coco Installation Directory"));

    connect(
        &m_cocoPathAspect,
        &Utils::FilePathAspect::changed,
        this,
        &GlobalSettingsWidget::onCocoPathChanged);

    using namespace Layouting;
    Form{
        Column{
            Row{Tr::tr("Coco Directory"), m_cocoPathAspect},
            Row{m_messageLabel}}
    }.attachTo(this);
}

void GlobalSettingsWidget::onCocoPathChanged()
{
    if (!verifyCocoDirectory(m_cocoPathAspect()))
        m_cocoPathAspect.setValue(m_previousCocoDir, Utils::BaseAspect::BeQuiet);
}

bool GlobalSettingsWidget::verifyCocoDirectory(const Utils::FilePath &cocoDir)
{
    cocoSettings().setDirectory(cocoDir);
    m_messageLabel.setText(cocoSettings().errorMessage());
    if (cocoSettings().isValid())
        m_messageLabel.setIconType(Utils::InfoLabel::None);
    else
        m_messageLabel.setIconType(Utils::InfoLabel::Error);
    return cocoSettings().isValid();
}

void GlobalSettingsWidget::apply()
{
    if (!verifyCocoDirectory(widgetCocoDir()))
        return;

    cocoSettings().setDirectory(widgetCocoDir());
    cocoSettings().save();

    emit updateCocoDir();
}

void GlobalSettingsWidget::cancel()
{
    cocoSettings().setDirectory(m_previousCocoDir);
}

void GlobalSettingsWidget::setVisible(bool visible)
{
    QFrame::setVisible(visible);
    m_previousCocoDir = cocoSettings().directory();
}

Utils::FilePath GlobalSettingsWidget::widgetCocoDir() const
{
    return Utils::FilePath::fromUserInput(m_cocoPathAspect.value());
}

GlobalSettingsPage::GlobalSettingsPage()
    : m_widget(nullptr)
{
    setId(Constants::COCO_SETTINGS_PAGE_ID);
    setDisplayName(QCoreApplication::translate("Coco", "Coco"));
    setCategory("I.Coco"); // Category I contains also the C++ settings.
}

GlobalSettingsPage &GlobalSettingsPage::instance()
{
    static GlobalSettingsPage instance;
    return instance;
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

} // namespace Coco::Internal
