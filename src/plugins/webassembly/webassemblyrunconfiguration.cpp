// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyrunconfiguration.h"

#include "webassemblyconstants.h"
#include "webassemblytr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QTextStream>

using namespace ProjectExplorer;
using namespace Utils;

namespace WebAssembly::Internal {

WebBrowserEntries parseEmrunOutput(const QByteArray &output)
{
    WebBrowserEntries result;
    QTextStream ts(output);
    QString line;
    static const QRegularExpression regExp(R"(  - (.*):\s*(.*))"); // '  - firefox: Mozilla Firefox'
                                                                   //      ^__1__^  ^______2______^
    while (ts.readLineInto(&line)) {
        const QRegularExpressionMatch match = regExp.match(line);
        if (match.hasMatch())
            result.push_back({match.captured(1), match.captured(2)});
    }
    return result;
}

static FilePath pythonInterpreter(const Environment &env)
{
    const QString emsdkPythonEnvVarKey("EMSDK_PYTHON");
    if (env.hasKey(emsdkPythonEnvVarKey))
        return FilePath::fromUserInput(env.value(emsdkPythonEnvVarKey));

    // FIXME: Centralize addPythonsFromPath() from the Python plugin and use that
    for (const char *interpreterCandidate : {"python3", "python", "python2"}) {
        const FilePath interpereter = env.searchInPath(QLatin1String(interpreterCandidate));
        if (interpereter.isExecutableFile())
            return interpereter;
    }
    return {};
}

static CommandLine emrunCommand(const Target *target,
                                const QString &buildKey,
                                const QString &browser,
                                const QString &port)
{
    if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
        const Environment env = bc->environment();
        const FilePath emrun = env.searchInPath("emrun");
        const FilePath emrunPy = emrun.absolutePath().pathAppended(emrun.baseName() + ".py");
        const FilePath targetPath = bc->buildSystem()->buildTarget(buildKey).targetFilePath;
        const FilePath html = targetPath.absolutePath() / (targetPath.baseName() + ".html");

        QStringList args(emrunPy.path());
        if (!browser.isEmpty()) {
            args.append("--browser");
            args.append(browser);
        }
        args.append("--port");
        args.append(port);
        args.append("--no_emrun_detect");
        args.append("--serve_after_close");
        args.append(html.toString());

        return CommandLine(pythonInterpreter(env), args);
    }
    return {};
}

static const char BROWSER_KEY[] = "WASM.WebBrowserSelectionAspect.Browser";

static WebBrowserEntries emrunBrowsers(Target *target)
{
    WebBrowserEntries result;
    result.append(qMakePair(QString(), Tr::tr("Default Browser")));
    if (auto bc = target->activeBuildConfiguration()) {
        const Environment environment = bc->environment();
        const FilePath emrunPath = environment.searchInPath("emrun");

        Process browserLister;
        browserLister.setEnvironment(environment);
        browserLister.setCommand({emrunPath, {"--list_browsers"}});
        browserLister.start();

        if (browserLister.waitForFinished())
            result.append(parseEmrunOutput(browserLister.readAllRawStandardOutput()));
    }
    return result;
}

class WebBrowserSelectionAspect : public BaseAspect
{
    Q_OBJECT

public:
    WebBrowserSelectionAspect(AspectContainer *container)
        : BaseAspect(container)
    {}

    void setTarget(Target *target)
    {
        m_availableBrowsers = emrunBrowsers(target);
        if (!m_availableBrowsers.isEmpty()) {
            const int defaultIndex = qBound(0, m_availableBrowsers.count() - 1, 1);
            m_currentBrowser = m_availableBrowsers.at(defaultIndex).first;
        }
        setDisplayName(Tr::tr("Web Browser"));
        setId("WebBrowserAspect");
        setSettingsKey("RunConfiguration.WebBrowser");

        addDataExtractor(this, &WebBrowserSelectionAspect::currentBrowser, &Data::currentBrowser);
    }

    void addToLayout(Layouting::LayoutItem &parent) override
    {
        QTC_CHECK(!m_webBrowserComboBox);
        m_webBrowserComboBox = new QComboBox;
        for (const WebBrowserEntry &be : m_availableBrowsers)
            m_webBrowserComboBox->addItem(be.second, be.first);
        m_webBrowserComboBox->setCurrentIndex(m_webBrowserComboBox->findData(m_currentBrowser));
        connect(m_webBrowserComboBox, &QComboBox::currentIndexChanged, this, [this] {
            m_currentBrowser = m_webBrowserComboBox->currentData().toString();
            emit changed();
        });
        parent.addItems({Tr::tr("Web browser:"), m_webBrowserComboBox});
    }

    void fromMap(const Store &map) override
    {
        if (!m_availableBrowsers.isEmpty())
            m_currentBrowser = map.value(BROWSER_KEY, m_availableBrowsers.first().first).toString();
    }

    void toMap(Store &map) const override
    {
        map.insert(BROWSER_KEY, m_currentBrowser);
    }

    QString currentBrowser() const { return m_currentBrowser; }

    struct Data : BaseAspect::Data
    {
        QString currentBrowser;
    };

private:
    QComboBox *m_webBrowserComboBox = nullptr;
    QString m_currentBrowser;
    WebBrowserEntries m_availableBrowsers;
};

// Runs a webassembly application via emscripten's "emrun" tool
// https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html
class EmrunRunConfiguration : public RunConfiguration
{
public:
    EmrunRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        webBrowser.setTarget(target);

        effectiveEmrunCall.setLabelText(Tr::tr("Effective emrun call:"));
        effectiveEmrunCall.setDisplayStyle(StringAspect::TextEditDisplay);
        effectiveEmrunCall.setReadOnly(true);

        setUpdater([this, target] {
            effectiveEmrunCall.setValue(emrunCommand(target,
                                                     buildKey(),
                                                     webBrowser.currentBrowser(),
                                                     "<port>").toUserOutput());
        });

        connect(&webBrowser, &BaseAspect::changed, this, &RunConfiguration::update);
        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

private:
    WebBrowserSelectionAspect webBrowser{this};
    StringAspect effectiveEmrunCall{this};
};

class EmrunRunWorker : public SimpleTargetRunner
{
public:
    EmrunRunWorker(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        auto portsGatherer = new PortsGatherer(runControl);
        addStartDependency(portsGatherer);

        setStartModifier([this, runControl, portsGatherer] {
            const QString browserId =
                    runControl->aspect<WebBrowserSelectionAspect>()->currentBrowser;
            setCommandLine(emrunCommand(runControl->target(),
                                        runControl->buildKey(),
                                        browserId,
                                        QString::number(portsGatherer->findEndPoint().port())));
            setEnvironment(runControl->buildEnvironment());
        });
    }
};

// Factories

EmrunRunConfigurationFactory::EmrunRunConfigurationFactory()
{
    registerRunConfiguration<EmrunRunConfiguration>(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
    addSupportedTargetDeviceType(Constants::WEBASSEMBLY_DEVICE_TYPE);
}

EmrunRunWorkerFactory::EmrunRunWorkerFactory()
{
    setProduct<EmrunRunWorker>();
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
}

} // Webassembly::Internal

#include "webassemblyrunconfiguration.moc"
