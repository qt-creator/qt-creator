// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblysettings.h"

#include "webassemblyconstants.h"
#include "webassemblyemsdk.h"
#include "webassemblyqtversion.h"
#include "webassemblytoolchain.h"
#include "webassemblytr.h"

#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QTextBrowser>
#include <QTimer>

using namespace Utils;

namespace WebAssembly::Internal {

WebAssemblySettings &settings()
{
    static WebAssemblySettings theSettings;
    return theSettings;
}

static QString environmentDisplay(const FilePath &sdkRoot)
{
    Environment env;
    WebAssemblyEmSdk::addToEnvironment(sdkRoot, env);
    QString result;
    auto h4 = [](const QString &text) { return QString("<h4>" + text + "</h4>"); };
    result.append(h4(Tr::tr("Adding directories to PATH:")));
    result.append(env.value("PATH").replace(OsSpecificAspects::pathListSeparator(sdkRoot.osType()), "<br/>"));
    result.append(h4(Tr::tr("Setting environment variables:")));
    for (const QString &envVar : env.toStringList()) {
        if (!envVar.startsWith("PATH")) // Path was already printed out above
            result.append(envVar + "<br/>");
    }
    return result;
}

WebAssemblySettings::WebAssemblySettings()
{
    setSettingsGroup("WebAssembly");
    setAutoApply(false);

    emSdk.setSettingsKey("EmSdk");
    emSdk.setExpectedKind(Utils::PathChooser::ExistingDirectory);
    emSdk.setDefaultValue(QDir::homePath());

    connect(this, &Utils::AspectContainer::applied, &WebAssemblyToolChain::registerToolChains);

    setLayouter([this] {
        auto instruction = new QLabel(
            Tr::tr("Select the root directory of an installed %1. "
                   "Ensure that the activated SDK version is compatible with the %2 "
                   "or %3 version that you plan to develop against.")
                .arg(R"(<a href="https://emscripten.org/docs/getting_started/downloads.html">Emscripten SDK</a>)")
                .arg(R"(<a href="https://doc.qt.io/qt-5/wasm.html#install-emscripten">Qt 5</a>)")
                .arg(R"(<a href="https://doc.qt.io/qt-6/wasm.html#install-emscripten">Qt 6</a>)"));
        instruction->setOpenExternalLinks(true);
        instruction->setWordWrap(true);

        m_emSdkVersionDisplay = new InfoLabel;
        m_emSdkVersionDisplay->setElideMode(Qt::ElideNone);
        m_emSdkVersionDisplay->setWordWrap(true);

        m_emSdkEnvDisplay = new QTextBrowser;
        m_emSdkEnvDisplay->setLineWrapMode(QTextBrowser::NoWrap);

        const QString minimumSupportedQtVersion =
            WebAssemblyQtVersion::minimumSupportedQtVersion().toString();
        m_qtVersionDisplay = new InfoLabel(
            Tr::tr("Note: %1 supports Qt %2 for WebAssembly and higher. "
                   "Your installed lower Qt version(s) are not supported.")
                .arg(Core::ICore::versionString(), minimumSupportedQtVersion),
            InfoLabel::Warning);
        m_qtVersionDisplay->setElideMode(Qt::ElideNone);
        m_qtVersionDisplay->setWordWrap(true);

        // _clang-format off
        using namespace Layouting;
        Column col {
            Group {
                title(Tr::tr("Emscripten SDK path:")),
                Column {
                    instruction,
                    emSdk,
                    m_emSdkVersionDisplay,
                },
            },
            Group {
                title(Tr::tr("Emscripten SDK environment:")),
                bindTo(&m_emSdkEnvGroupBox),
                Column {
                    m_emSdkEnvDisplay,
                },
            },
            m_qtVersionDisplay,
        };
        // _clang-format on

        connect(emSdk.pathChooser(), &Utils::PathChooser::textChanged,
                this, &WebAssemblySettings::updateStatus);

        // updateStatus() uses m_emSdkEnvGroupBox which only exists
        // after this here emerges. So delay the update a bit.
        QTimer::singleShot(0, this, &WebAssemblySettings::updateStatus);

        return col;
    });

    readSettings();
}

void WebAssemblySettings::updateStatus()
{
    WebAssemblyEmSdk::clearCaches();

    const Utils::FilePath newEmSdk = emSdk.pathChooser()->filePath();
    const bool sdkValid = newEmSdk.exists() && WebAssemblyEmSdk::isValid(newEmSdk);

    QTC_ASSERT(m_emSdkVersionDisplay, return);
    QTC_ASSERT(m_emSdkEnvGroupBox, return);
    m_emSdkVersionDisplay->setVisible(sdkValid);
    m_emSdkEnvGroupBox->setEnabled(sdkValid);

    if (sdkValid) {
        const QVersionNumber sdkVersion = WebAssemblyEmSdk::version(newEmSdk);
        const QVersionNumber minVersion = WebAssemblyToolChain::minimumSupportedEmSdkVersion();
        const bool versionTooLow = sdkVersion < minVersion;
        m_emSdkVersionDisplay->setType(versionTooLow ? InfoLabel::NotOk : InfoLabel::Ok);
        auto bold = [](const QString &text) { return QString("<b>" + text + "</b>"); };
        m_emSdkVersionDisplay->setText(
            versionTooLow ? Tr::tr("The activated version %1 is not supported by %2. "
                                   "Activate version %3 or higher.")
                                .arg(bold(sdkVersion.toString()))
                                .arg(bold(Core::ICore::versionString()))
                                .arg(bold(minVersion.toString()))
                          : Tr::tr("Activated version: %1")
                                .arg(bold(sdkVersion.toString())));
        m_emSdkEnvDisplay->setText(environmentDisplay(newEmSdk));
    } else {
        m_emSdkEnvDisplay->clear();
    }

    m_qtVersionDisplay->setVisible(WebAssemblyQtVersion::isUnsupportedQtVersionInstalled());
}

// WebAssemblySettingsPage

class WebAssemblySettingsPage final : public Core::IOptionsPage
{
public:
    WebAssemblySettingsPage()
    {
        setId(Id(Constants::SETTINGS_ID));
        setDisplayName(Tr::tr("WebAssembly"));
        setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const WebAssemblySettingsPage settingsPage;

} // WebAssembly::Internal
