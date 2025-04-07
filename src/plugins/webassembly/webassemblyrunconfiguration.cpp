// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyrunconfiguration.h"

#include "webassemblyconstants.h"
#include "webassemblytr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>
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

static CommandLine emrunCommand(const BuildConfiguration *bc,
                                const QString &buildKey,
                                const QString &browser,
                                const QString &port)
{
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
    args.append(html.toUrlishString());

    return CommandLine(pythonInterpreter(env), args);
}

static const char BROWSER_KEY[] = "WASM.WebBrowserSelectionAspect.Browser";

static WebBrowserEntries emrunBrowsers(BuildConfiguration *bc)
{
    WebBrowserEntries result;
    result.append(qMakePair(QString(), Tr::tr("Default Browser")));
    const Environment environment = bc->environment();
    const FilePath emrunPath = environment.searchInPath("emrun");

    Process browserLister;
    browserLister.setEnvironment(environment);
    browserLister.setCommand({emrunPath, {"--list_browsers"}});
    browserLister.start();

    if (browserLister.waitForFinished())
        result.append(parseEmrunOutput(browserLister.rawStdOut()));
    return result;
}

class WebBrowserSelectionAspect : public BaseAspect
{
    Q_OBJECT

public:
    WebBrowserSelectionAspect(AspectContainer *container)
        : BaseAspect(container)
    {}

    void setBuildConfiguration(BuildConfiguration *bc)
    {
        m_availableBrowsers = emrunBrowsers(bc);
        if (!m_availableBrowsers.isEmpty()) {
            const int defaultIndex = qBound(0, m_availableBrowsers.count() - 1, 1);
            m_currentBrowser = m_availableBrowsers.at(defaultIndex).first;
        }
        setDisplayName(Tr::tr("Web Browser"));
        setId("WebBrowserAspect");
        setSettingsKey("RunConfiguration.WebBrowser");

        addDataExtractor(this, &WebBrowserSelectionAspect::currentBrowser, &Data::currentBrowser);
    }

    void addToLayoutImpl(Layouting::Layout &parent) override
    {
        QTC_CHECK(!m_webBrowserComboBox);
        m_webBrowserComboBox = new QComboBox;
        for (const WebBrowserEntry &be : std::as_const(m_availableBrowsers))
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
    EmrunRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        webBrowser.setBuildConfiguration(bc);

        effectiveEmrunCall.setLabelText(Tr::tr("Effective emrun call:"));
        effectiveEmrunCall.setDisplayStyle(StringAspect::TextEditDisplay);
        effectiveEmrunCall.setReadOnly(true);

        setUpdater([this] {
            effectiveEmrunCall.setValue(emrunCommand(buildConfiguration(),
                                                     buildKey(),
                                                     webBrowser.currentBrowser(),
                                                     "<port>").toUserOutput());
        });

        connect(&webBrowser, &BaseAspect::changed, this, &RunConfiguration::update);
    }

private:
    WebBrowserSelectionAspect webBrowser{this};
    StringAspect effectiveEmrunCall{this};
};

// Factories

class EmrunRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    EmrunRunConfigurationFactory()
    {
        registerRunConfiguration<EmrunRunConfiguration>(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
        addSupportedTargetDeviceType(Constants::WEBASSEMBLY_DEVICE_TYPE);
    }
};

class EmrunRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    EmrunRunWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            runControl->requestWorkerChannel();

            const auto modifier = [runControl](Process &process) {
                const QString browserId =
                    runControl->aspectData<WebBrowserSelectionAspect>()->currentBrowser;
                process.setCommand(emrunCommand(runControl->buildConfiguration(), runControl->buildKey(),
                    browserId, QString::number(runControl->workerChannel().port())));
                process.setEnvironment(runControl->buildEnvironment());
            };

            return createProcessWorker(runControl, modifier);
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
    }
};

void setupEmrunRunSupport()
{
    static EmrunRunConfigurationFactory theEmrunRunConfigurationFactory;
    static EmrunRunWorkerFactory theEmrunRunWorkerFactory;
}

} // Webassembly::Internal

#include "webassemblyrunconfiguration.moc"
