/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "webassemblyconstants.h"
#include "webassemblyemsdk.h"
#include "webassemblyoptionspage.h"
#include "webassemblyqtversion.h"
#include "webassemblytoolchain.h"

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
    Q_DECLARE_TR_FUNCTIONS(WebAssembly::Internal::WebAssemblyOptionsWidget)

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
        auto pathChooserBox = new QGroupBox(tr("Emscripten SDK path:"));
        pathChooserBox->setFlat(true);
        auto layout = new QVBoxLayout(pathChooserBox);
        auto instruction = new QLabel(
                    tr("Select the root directory of an "
                       "<a href=\"https://emscripten.org/docs/getting_started/downloads.html\">"
                       "installed Emscripten SDK</a>. Ensure that the activated SDK version is "
                       "<a href=\"https://doc.qt.io/qt-5/wasm.html#install-emscripten\">"
                       "compatible</a> with the Qt version that you plan to develop against."));
        instruction->setOpenExternalLinks(true);
        instruction->setWordWrap(true);
        layout->addWidget(instruction);
        m_emSdkPathChooser = new PathChooser(this);
        m_emSdkPathChooser->setExpectedKind(PathChooser::Directory);
        m_emSdkPathChooser->setInitialBrowsePathBackup(FileUtils::homePath());
        m_emSdkPathChooser->setFilePath(WebAssemblyEmSdk::registeredEmSdk());
        connect(m_emSdkPathChooser, &PathChooser::pathChanged, [this](){ updateStatus(); });
        layout->addWidget(m_emSdkPathChooser);
        m_emSdkVersionDisplay = new InfoLabel(this);
        m_emSdkVersionDisplay->setElideMode(Qt::ElideNone);
        m_emSdkVersionDisplay->setWordWrap(true);
        layout->addWidget(m_emSdkVersionDisplay);
        mainLayout->addWidget(pathChooserBox);
    }

    {
        m_emSdkEnvGroupBox = new QGroupBox(tr("Emscripten SDK environment:"));
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
        const QString minimumSupportedQtVersion = QString::fromLatin1("%1.%2")
                .arg(WebAssemblyQtVersion::minimumSupportedQtVersion().majorVersion)
                .arg(WebAssemblyQtVersion::minimumSupportedQtVersion().minorVersion);
        m_qtVersionDisplay = new InfoLabel(
                    tr("Note: %1 supports Qt %2 for WebAssembly and higher. "
                       "Your installed lower version(s) are not supported.")
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
    result.append(WebAssemblyOptionsWidget::tr("<h4>Adding directories to PATH:</h4>"));
    result.append(env.value("PATH").replace(OsSpecificAspects::pathListSeparator(sdkRoot.osType()), "<br/>"));
    result.append(WebAssemblyOptionsWidget::tr("<h4>Setting environment variables:</h4>"));
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
        m_emSdkVersionDisplay->setText(
                    versionTooLow ? tr("The activated version <b>%1</b> is not supported by %2."
                                       "<br/>Activate version <b>%3</b> or higher.")
                                    .arg(sdkVersion.toString(), Core::ICore::versionString(),
                                         minVersion.toString())
                                  : tr("Activated version: <b>%1</b>").arg(sdkVersion.toString()));
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
    setDisplayName(WebAssemblyOptionsWidget::tr("WebAssembly"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new WebAssemblyOptionsWidget; });
}

} // Internal
} // WebAssembly
