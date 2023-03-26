// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyconstants.h"
#include "webassemblyemsdk.h"
#include "webassemblyoptionspage.h"
#include "webassemblyqtversion.h"
#include "webassemblytoolchain.h"
#include "webassemblytr.h"

#include <coreplugin/icore.h>
#include <utils/environment.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QGroupBox>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace Utils;

namespace WebAssembly {
namespace Internal {

class WebAssemblyOptionsWidget : public Core::IOptionsPageWidget
{
public:
    WebAssemblyOptionsWidget();

    void updateStatus();

private:
    void apply() final;
    void showEvent(QShowEvent *event) final;

    PathChooser *m_emSdkPathChooser;
    InfoLabel *m_emSdkVersionDisplay;
    QGroupBox *m_emSdkEnvGroupBox;
    QTextBrowser *m_emSdkEnvDisplay;
    InfoLabel *m_qtVersionDisplay;
};

WebAssemblyOptionsWidget::WebAssemblyOptionsWidget()
{
    auto mainLayout = new QVBoxLayout(this);

    {
        auto pathChooserBox = new QGroupBox(Tr::tr("Emscripten SDK path:"));
        pathChooserBox->setFlat(true);
        auto layout = new QVBoxLayout(pathChooserBox);
        auto instruction = new QLabel(
                    Tr::tr("Select the root directory of an installed %1. "
                           "Ensure that the activated SDK version is compatible with the %2 "
                           "or %3 version that you plan to develop against.")
                    .arg(R"(<a href="https://emscripten.org/docs/getting_started/downloads.html">Emscripten SDK</a>)")
                    .arg(R"(<a href="https://doc.qt.io/qt-5/wasm.html#install-emscripten">Qt 5</a>)")
                    .arg(R"(<a href="https://doc.qt.io/qt-6/wasm.html#install-emscripten">Qt 6</a>)"));

        instruction->setOpenExternalLinks(true);
        instruction->setWordWrap(true);
        layout->addWidget(instruction);
        m_emSdkPathChooser = new PathChooser(this);
        m_emSdkPathChooser->setExpectedKind(PathChooser::Directory);
        m_emSdkPathChooser->setInitialBrowsePathBackup(FileUtils::homePath());
        m_emSdkPathChooser->setFilePath(WebAssemblyEmSdk::registeredEmSdk());
        connect(m_emSdkPathChooser, &PathChooser::textChanged,
                this, &WebAssemblyOptionsWidget::updateStatus);
        layout->addWidget(m_emSdkPathChooser);
        m_emSdkVersionDisplay = new InfoLabel(this);
        m_emSdkVersionDisplay->setElideMode(Qt::ElideNone);
        m_emSdkVersionDisplay->setWordWrap(true);
        layout->addWidget(m_emSdkVersionDisplay);
        mainLayout->addWidget(pathChooserBox);
    }

    {
        m_emSdkEnvGroupBox = new QGroupBox(Tr::tr("Emscripten SDK environment:"));
        m_emSdkEnvGroupBox->setFlat(true);
        m_emSdkEnvGroupBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
        auto layout = new QVBoxLayout(m_emSdkEnvGroupBox);
        m_emSdkEnvDisplay = new QTextBrowser;
        m_emSdkEnvDisplay->setLineWrapMode(QTextBrowser::NoWrap);
        m_emSdkEnvDisplay->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        layout->addWidget(m_emSdkEnvDisplay);
        mainLayout->addWidget(m_emSdkEnvGroupBox, 1);
    }

    mainLayout->addStretch();

    {
        const QString minimumSupportedQtVersion =
                WebAssemblyQtVersion::minimumSupportedQtVersion().toString();
        m_qtVersionDisplay = new InfoLabel(
                    Tr::tr("Note: %1 supports Qt %2 for WebAssembly and higher. "
                           "Your installed lower Qt version(s) are not supported.")
                    .arg(Core::ICore::versionString(), minimumSupportedQtVersion),
                    InfoLabel::Warning);
        m_qtVersionDisplay->setElideMode(Qt::ElideNone);
        m_qtVersionDisplay->setWordWrap(true);
        mainLayout->addWidget(m_qtVersionDisplay);
    }
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

void WebAssemblyOptionsWidget::updateStatus()
{
    WebAssemblyEmSdk::clearCaches();

    const FilePath sdkPath = m_emSdkPathChooser->filePath();
    const bool sdkValid = sdkPath.exists() && WebAssemblyEmSdk::isValid(sdkPath);

    m_emSdkVersionDisplay->setVisible(sdkValid);
    m_emSdkEnvGroupBox->setVisible(sdkValid);

    if (sdkValid) {
        const QVersionNumber sdkVersion = WebAssemblyEmSdk::version(sdkPath);
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
        m_emSdkEnvDisplay->setText(environmentDisplay(sdkPath));
    }

    m_qtVersionDisplay->setVisible(WebAssemblyQtVersion::isUnsupportedQtVersionInstalled());
}

void WebAssemblyOptionsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    updateStatus();
}

void WebAssemblyOptionsWidget::apply()
{
    const FilePath sdkPath = m_emSdkPathChooser->filePath();
    if (!WebAssemblyEmSdk::isValid(sdkPath))
        return;
    WebAssemblyEmSdk::registerEmSdk(sdkPath);
    WebAssemblyToolChain::registerToolChains();
}

WebAssemblyOptionsPage::WebAssemblyOptionsPage()
{
    setId(Id(Constants::SETTINGS_ID));
    setDisplayName(Tr::tr("WebAssembly"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new WebAssemblyOptionsWidget; });
}

} // Internal
} // WebAssembly
