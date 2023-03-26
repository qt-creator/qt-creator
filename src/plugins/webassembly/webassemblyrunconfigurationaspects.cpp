// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyrunconfigurationaspects.h"
#include "webassemblytr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QComboBox>
#include <QTextStream>

using namespace Utils;

namespace WebAssembly {
namespace Internal {

static const char BROWSER_KEY[] = "WASM.WebBrowserSelectionAspect.Browser";

static WebBrowserEntries emrunBrowsers(ProjectExplorer::Target *target)
{
    WebBrowserEntries result;
    result.append(qMakePair(QString(), Tr::tr("Default Browser")));
    if (auto bc = target->activeBuildConfiguration()) {
        const Utils::Environment environment = bc->environment();
        const Utils::FilePath emrunPath = environment.searchInPath("emrun");

        QtcProcess browserLister;
        browserLister.setEnvironment(environment);
        browserLister.setCommand({emrunPath, {"--list_browsers"}});
        browserLister.start();

        if (browserLister.waitForFinished())
            result.append(WebBrowserSelectionAspect::parseEmrunOutput(
                              browserLister.readAllRawStandardOutput()));
    }
    return result;
}

WebBrowserSelectionAspect::WebBrowserSelectionAspect(ProjectExplorer::Target *target)
    : m_availableBrowsers(emrunBrowsers(target))
{
    if (!m_availableBrowsers.isEmpty()) {
        const int defaultIndex = qBound(0, m_availableBrowsers.count() - 1, 1);
        m_currentBrowser = m_availableBrowsers.at(defaultIndex).first;
    }
    setDisplayName(Tr::tr("Web Browser"));
    setId("WebBrowserAspect");
    setSettingsKey("RunConfiguration.WebBrowser");

    addDataExtractor(this, &WebBrowserSelectionAspect::currentBrowser, &Data::currentBrowser);
}

void WebBrowserSelectionAspect::addToLayout(Layouting::LayoutBuilder &builder)
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
    builder.addItems({Tr::tr("Web browser:"), m_webBrowserComboBox});
}

void WebBrowserSelectionAspect::fromMap(const QVariantMap &map)
{
    if (!m_availableBrowsers.isEmpty())
        m_currentBrowser = map.value(BROWSER_KEY, m_availableBrowsers.first().first).toString();
}

void WebBrowserSelectionAspect::toMap(QVariantMap &map) const
{
    map.insert(BROWSER_KEY, m_currentBrowser);
}

QString WebBrowserSelectionAspect::currentBrowser() const
{
    return m_currentBrowser;
}

WebBrowserEntries WebBrowserSelectionAspect::parseEmrunOutput(const QByteArray &output)
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

} // namespace Internal
} // namespace Webassembly
