// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "crashreporting.h"

#include "appinfo.h"
#include "aspects.h"
#include "environment.h"
#include "infobar.h"
#include "layoutbuilder.h"
#include "utilstr.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>

#ifdef ENABLE_SENTRY
#include <QLoggingCategory>
#include <QSysInfo>

#include <sentry.h>

Q_LOGGING_CATEGORY(sentryLog, "qtc.sentry", QtWarningMsg)
#endif

namespace Utils {

#ifdef ENABLE_SENTRY
static bool s_isCrashreportingSetUp = false;
#endif

const char kCrashReportingInfoBarEntry[] = "WarnCrashReporting";
const char kCrashReportingSetting[] = "CrashReportingEnabled";

class CrashReportAspect : public BoolAspect
{
public:
    CrashReportAspect()
    {
        setSettingsKey(kCrashReportingSetting);
        setLabelPlacement(BoolAspect::LabelPlacement::Compact);
        setLabelText(Tr::tr("Enable crash reporting"));
        setToolTip(
            "<p>"
            + Tr::tr(
                "Allow crashes to be automatically reported. Collected reports are "
                "used for the sole purpose of fixing bugs.")
            + "</p><p>"
            + Tr::tr("Crash reports are saved in \"%1\".")
                  .arg(appInfo().crashReports.toUserOutput()));
        setDefaultValue(false);
        readSettings();
    }
};

static BoolAspect &theCrashReportSetting()
{
    static CrashReportAspect theSetting;
    return theSetting;
}

static bool isCrashReportingUsingCrashpad()
{
#ifdef SENTRY_CRASHPAD_PATH
    return true;
#else
    return false;
#endif
}

static QString msgCrashpadInformation()
{
    if (!isCrashReportingAvailable())
        return {};
    QString backend;
    QString url;
    if (isCrashReportingUsingCrashpad()) {
        backend = "Google Crashpad";
        url = "https://chromium.googlesource.com/crashpad/crashpad/+/master/doc/"
              "overview_design.md";
    } else {
        backend = "Google Breakpad";
        url = "https://chromium.googlesource.com/breakpad/breakpad/+/HEAD/docs/"
              "client_design.md";
    }
    //: %1 = application name, %2 crash backend name (Google Crashpad or Google Breakpad)
    return Tr::tr(
               "%1 uses %2 for collecting crashes and sending them to Sentry "
               "for processing. %2 may capture arbitrary contents from crashed process’ "
               "memory, including user sensitive information, URLs, and whatever other content "
               "users have trusted %1 with. The collected crash reports are however only used "
               "for the sole purpose of fixing bugs.")
               .arg(QGuiApplication::applicationDisplayName(), backend)
           + "<br><br>" + Tr::tr("More information:") + "<br><a href='" + url
           + "'>"
           //: %1 = crash backend name (Google Crashpad or Google Breakpad)
           + Tr::tr("%1 Overview").arg(backend)
           + "</a>"
             "<br><a href='https://sentry.io/security/'>"
           + Tr::tr("%1 security policy").arg("Sentry.io") + "</a>";
}

// TODO join with whatever the "properties" item in the document menu does?
static QString formatSize(qint64 size)
{
    QStringList units{Tr::tr("Bytes"), Tr::tr("KiB"), Tr::tr("MiB"), Tr::tr("GiB"), Tr::tr("TiB")};
    double outputSize = size;
    int i;
    for (i = 0; i < units.size() - 1; ++i) {
        if (outputSize < 1024)
            break;
        outputSize /= 1024;
    }
    return i == 0 ? QString("%0 %1").arg(outputSize).arg(units[i])             // Bytes
                  : QString("%0 %1").arg(outputSize, 0, 'f', 2).arg(units[i]); // KB, MB, GB, TB
}

bool isCrashReportingAvailable()
{
#ifdef ENABLE_SENTRY
    return true;
#else
    return false;
#endif
}

/*!
    Returns whether it succeeded.
    Normally \c false indicates that a restart is required before the change can take effect.
*/
bool setUpCrashReporting()
{
    if (!isCrashReportingAvailable())
        return false;
    QTC_ASSERT(!appInfo().crashReports.isEmpty(), return false);
    QTC_ASSERT(QCoreApplication::instance(), return false);
#ifdef ENABLE_SENTRY
    // Disabling crash reporting when it is already set up requires restart
    if (!theCrashReportSetting().value())
        return !s_isCrashreportingSetUp;

    if (s_isCrashreportingSetUp)
        return true;

    s_isCrashreportingSetUp = true;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, SENTRY_DSN);
#ifdef Q_OS_WIN
    sentry_options_set_database_pathw(
        options, appInfo().crashReports.nativePath().toStdWString().c_str());
#else
    sentry_options_set_database_path(
        options, appInfo().crashReports.nativePath().toUtf8().constData());
#endif
#ifdef SENTRY_CRASHPAD_PATH
    if (const FilePath handlerpath = appInfo().libexec / "crashpad_handler"; handlerpath.exists()) {
        sentry_options_set_handler_path(options, handlerpath.nativePath().toUtf8().constData());
    } else if (const auto fallback = FilePath::fromString(SENTRY_CRASHPAD_PATH); fallback.exists()) {
        sentry_options_set_handler_path(options, fallback.nativePath().toUtf8().constData());
    } else {
        qCWarning(sentryLog) << "Failed to find crashpad_handler for Sentry crash reports.";
    }
#endif
    const QString release = QString(SENTRY_PROJECT) + "@" + QCoreApplication::applicationVersion();
    sentry_options_set_release(options, release.toUtf8().constData());
    sentry_options_set_debug(options, sentryLog().isDebugEnabled() ? 1 : 0);
    sentry_init(options);
    sentry_set_tag("qt.version", qVersion());
    sentry_set_tag("qt.architecture", QSysInfo::buildCpuArchitecture().toUtf8().constData());
    sentry_set_tag("qtcreator.compiler", Utils::compilerString().toUtf8().constData());
    if (!appInfo().revision.isEmpty())
        sentry_set_tag("qtcreator.revision", appInfo().revision.toUtf8().constData());
#endif
    return true;
}

void addCrashReportSetting(
    AspectContainer *container, const std::function<void(QString)> &askForRestart)
{
    if (!isCrashReportingAvailable())
        return;
    BoolAspect *setting = &theCrashReportSetting();
    theCrashReportSetting().addOnChanged(setting, [askForRestart] {
        if (!setUpCrashReporting())
            askForRestart(Tr::tr("The crash reporting state will take effect after restart."));
    });
    container->registerAspect(setting);
}

void addCrashReportSettingsUi(QWidget *parent, Layouting::Grid *grid)
{
    if (!isCrashReportingAvailable())
        return;
    using namespace Layouting;
    Row crashDetails;
    if (isCrashReportingUsingCrashpad()) {
        QPushButton *crashReportsMenuButton = new QPushButton(Tr::tr("Manage"), parent);
        crashReportsMenuButton->setToolTip(theCrashReportSetting().toolTip());
        QLabel *crashReportsSizeText = new QLabel(parent);
        crashReportsSizeText->setToolTip(theCrashReportSetting().toolTip());
        auto crashReportsMenu = new QMenu(crashReportsMenuButton);
        crashReportsMenuButton->setMenu(crashReportsMenu);
        crashDetails.addItems({crashReportsMenuButton, crashReportsSizeText});
        const FilePath crashReportsPath = appInfo().crashReports;
        const FilePaths reportsPaths
            = {crashReportsPath / "completed",
               crashReportsPath / "reports",
               crashReportsPath / "attachments",
               crashReportsPath / "pending",
               crashReportsPath / "new"};

        auto openLocationAction = new QAction(Tr::tr("Go to Crash Reports"));
        QObject::connect(openLocationAction, &QAction::triggered, [reportsPaths] {
            const FilePath path = reportsPaths.first().parentDir();
            if (!QDesktopServices::openUrl(path.toUrl())) {
                qWarning() << "Failed to open path:" << path;
            }
        });
        crashReportsMenu->addAction(openLocationAction);

        auto clearAction = new QAction(Tr::tr("Clear Crash Reports"));
        crashReportsMenu->addAction(clearAction);

        const auto updateClearCrashWidgets = [reportsPaths, clearAction, crashReportsSizeText] {
            qint64 size = 0;
            FilePath::iterateDirectories(
                reportsPaths,
                [&size](const FilePath &item) {
                    size += item.fileSize();
                    return IterationPolicy::Continue;
                },
                FileFilter({}, QDir::Files, QDirIterator::Subdirectories));
            clearAction->setEnabled(size > 0);
            crashReportsSizeText->setText(formatSize(size));
        };
        updateClearCrashWidgets();
        QObject::connect(
            clearAction, &QAction::triggered, clearAction, [updateClearCrashWidgets, reportsPaths] {
                FilePath::iterateDirectories(
                    reportsPaths,
                    [](const FilePath &item) {
                        item.removeRecursively();
                        return IterationPolicy::Continue;
                    },
                    FileFilter({}, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot));
                updateClearCrashWidgets();
            });
    }
    auto helpCrashReportingButton = new QToolButton(parent);
    helpCrashReportingButton->setText(Tr::tr("?"));
    QObject::connect(helpCrashReportingButton, &QAbstractButton::clicked, parent, [parent] {
        QMessageBox::information(
            parent, Tr::tr("Crash Reporting"), msgCrashpadInformation(), QMessageBox::Close);
    });
    crashDetails.addItem(helpCrashReportingButton);
    if (qtcEnvironmentVariableIsSet("QTC_SHOW_CRASHBUTTON")) {
        auto crashButton = new QPushButton("CRASH!!!");
        QObject::connect(crashButton, &QPushButton::clicked, [] {
            // do a real crash
            volatile int *a = reinterpret_cast<volatile int *>(NULL);
            *a = 1;
        });
        crashDetails.addItem(crashButton);
    }
    crashDetails.addItem(st);
    grid->addRow({theCrashReportSetting(), crashDetails});
}

void warnAboutCrashReporting(
    InfoBar *infoBar,
    const QString &optionsButtonText,
    const InfoBarEntry::CallBack &optionsButtonCallback)
{
    if (!isCrashReportingAvailable())
        return;
    if (!infoBar->canInfoBeAdded(kCrashReportingInfoBarEntry))
        return;

    QString warnStr = theCrashReportSetting().value()
                          ? Tr::tr(
                                "%1 collects crash reports for the sole purpose of fixing bugs. "
                                "To disable this feature go to %2.")
                          : Tr::tr(
                                "%1 can collect crash reports for the sole purpose of fixing bugs. "
                                "To enable this feature go to %2.");

    if (Utils::HostOsInfo::isMacHost()) {
        warnStr = warnStr.arg(
            QGuiApplication::applicationDisplayName(),
            QGuiApplication::applicationDisplayName()
                + Tr::tr(" > Preferences > Environment > System"));
    } else {
        warnStr = warnStr.arg(
            QGuiApplication::applicationDisplayName(),
            Tr::tr("Edit > Preferences > Environment > System"));
    }

    Utils::InfoBarEntry
        info(kCrashReportingInfoBarEntry, warnStr, Utils::InfoBarEntry::GlobalSuppression::Enabled);
    info.setTitle(Tr::tr("Crash Reporting"));
    info.setInfoType(InfoLabel::Information);
    info.addCustomButton(
        optionsButtonText,
        optionsButtonCallback,
        {},
        InfoBarEntry::ButtonAction::SuppressPersistently);

    info.setDetailsWidgetCreator([]() -> QWidget * {
        auto label = new QLabel;
        label->setWindowTitle(Tr::tr("Crash Reporting"));
        label->setWordWrap(true);
        label->setOpenExternalLinks(true);
        label->setText(Utils::msgCrashpadInformation());
        label->setContentsMargins(0, 0, 0, 8);
        return label;
    });
    infoBar->addInfo(info);
}

void shutdownCrashReporting()
{
#ifdef ENABLE_SENTRY
    sentry_close();
#endif
}

} // namespace Utils
