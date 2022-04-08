/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "webassemblyrunconfigurationaspects.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>

#include <QComboBox>

#ifdef WITH_TESTS
#   include <QTest>
#   include "webassemblyplugin.h"
#endif // WITH_TESTS

using namespace Utils;

namespace WebAssembly {
namespace Internal {

static const char BROWSER_KEY[] = "WASM.WebBrowserSelectionAspect.Browser";

static WebBrowserEntries parseEmrunOutput(const QByteArray &output)
{
    WebBrowserEntries result;
    QTextStream ts(output);
    QString line;
    const QRegularExpression regExp("  - (.*):(.*)");
    while (ts.readLineInto(&line)) {
        const QRegularExpressionMatch match = regExp.match(line);
        if (match.hasMatch())
            result << qMakePair(match.captured(1), match.captured(2).trimmed());
    }
    return result;
}

static WebBrowserEntries emrunBrowsers(ProjectExplorer::Target *target)
{
    WebBrowserEntries result;
    result.append(qMakePair(QString(), WebBrowserSelectionAspect::tr("Default Browser")));
    if (auto bc = target->activeBuildConfiguration()) {
        const Utils::Environment environment = bc->environment();
        const Utils::FilePath emrunPath = environment.searchInPath("emrun");

        QtcProcess browserLister;
        browserLister.setEnvironment(environment);
        browserLister.setCommand({emrunPath, {"--list_browsers"}});
        browserLister.start();

        if (browserLister.waitForFinished())
            result.append(parseEmrunOutput(browserLister.readAllStandardOutput()));
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
    setDisplayName(tr("Web Browser"));
    setId("WebBrowserAspect");
    setSettingsKey("RunConfiguration.WebBrowser");

    addDataExtractor(this, &WebBrowserSelectionAspect::currentBrowser, &Data::currentBrowser);
}

void WebBrowserSelectionAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!m_webBrowserComboBox);
    m_webBrowserComboBox = new QComboBox;
    for (const WebBrowserEntry &be : m_availableBrowsers)
        m_webBrowserComboBox->addItem(be.second, be.first);
    m_webBrowserComboBox->setCurrentIndex(m_webBrowserComboBox->findData(m_currentBrowser));
    connect(m_webBrowserComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() {
                m_currentBrowser = m_webBrowserComboBox->currentData().toString();
                emit changed();
            });
    builder.addItems({tr("Web browser:"), m_webBrowserComboBox});
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

// Unit tests:
#ifdef WITH_TESTS

void testEmrunBrowserListParsing();
void testEmrunBrowserListParsing_data();

void WebAssemblyPlugin::testEmrunBrowserListParsing()
{
    QFETCH(QByteArray, emrunOutput);
    QFETCH(WebBrowserEntries, expectedBrowsers);

    QCOMPARE(parseEmrunOutput(emrunOutput), expectedBrowsers);
}

void WebAssemblyPlugin::testEmrunBrowserListParsing_data()
{
    QTest::addColumn<QByteArray>("emrunOutput");
    QTest::addColumn<WebBrowserEntries>("expectedBrowsers");

    QTest::newRow("emsdk 1.39.8")
// Output of "emrun --list_browsers"
            << QByteArray(
R"(emrun has automatically found the following browsers in the default install locations on the system:

  - firefox: Mozilla Firefox
  - chrome: Google Chrome

You can pass the --browser <id> option to launch with the given browser above.
Even if your browser was not detected, you can use --browser /path/to/browser/executable to launch with that browser.

)")
            << WebBrowserEntries({
                qMakePair(QLatin1String("firefox"), QLatin1String("Mozilla Firefox")),
                qMakePair(QLatin1String("chrome"), QLatin1String("Google Chrome"))});

    QTest::newRow("emsdk 2.0.14")
            << QByteArray(
R"(emrun has automatically found the following browsers in the default install locations on the system:

  - firefox: Mozilla Firefox 96.0.0.8041
  - chrome: Google Chrome 97.0.4692.71

You can pass the --browser <id> option to launch with the given browser above.
Even if your browser was not detected, you can use --browser /path/to/browser/executable to launch with that browser.

)")
            << WebBrowserEntries({
                qMakePair(QLatin1String("firefox"), QLatin1String("Mozilla Firefox 96.0.0.8041")),
                qMakePair(QLatin1String("chrome"), QLatin1String("Google Chrome 97.0.4692.71"))});
}

#endif // WITH_TESTS

} // namespace Internal
} // namespace Webassembly
