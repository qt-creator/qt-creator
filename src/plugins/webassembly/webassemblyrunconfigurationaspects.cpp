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

#include <QComboBox>
#include <QFormLayout>

namespace WebAssembly {
namespace Internal {

static const char BROWSER_KEY[] = "WASM.WebBrowserSelectionAspect.Browser";

static QStringList detectedBrowsers(ProjectExplorer::Target *target)
{
    static QStringList result;
    if (result.isEmpty()) {
        if (auto bc = target->activeBuildConfiguration()) {
            const Utils::Environment environment = bc->environment();
            const Utils::FilePath emrunPath = environment.searchInPath("emrun");

            QProcess browserLister;
            browserLister.setProcessEnvironment(environment.toProcessEnvironment());
            browserLister.setProgram(emrunPath.toString());
            browserLister.setArguments({"--list_browsers"});
            browserLister.start(QIODevice::ReadOnly);

            if (browserLister.waitForFinished()) {
                const QByteArray output = browserLister.readAllStandardOutput();
                QTextStream ts(output);
                QString line;
                const QRegularExpression regExp("  - (.*):.*");
                while (ts.readLineInto(&line)) {
                    const QRegularExpressionMatch match = regExp.match(line);
                    if (match.hasMatch())
                        result << match.captured(1);
                }
            }
        }
    }
    return result;
}

WebBrowserSelectionAspect::WebBrowserSelectionAspect(ProjectExplorer::Target *target)
    : m_availableBrowsers(detectedBrowsers(target))
{
    if (!m_availableBrowsers.isEmpty())
        m_currentBrowser = m_availableBrowsers.first();
    setDisplayName(tr("Web browser"));
    setId("WebBrowserAspect");
    setSettingsKey("RunConfiguration.WebBrowser");
}

void WebBrowserSelectionAspect::addToLayout(ProjectExplorer::LayoutBuilder &builder)
{
    QTC_CHECK(!m_webBrowserComboBox);
    m_webBrowserComboBox = new QComboBox;
    m_webBrowserComboBox->addItems(m_availableBrowsers);
    m_webBrowserComboBox->setCurrentText(m_currentBrowser);
    connect(m_webBrowserComboBox, &QComboBox::currentTextChanged,
            [this](const QString &selectedBrowser){
                m_currentBrowser = selectedBrowser;
                emit changed();
            });
    builder.addItems(tr("Web browser:"), m_webBrowserComboBox);
}

void WebBrowserSelectionAspect::fromMap(const QVariantMap &map)
{
    if (!m_availableBrowsers.isEmpty())
        m_currentBrowser = map.value(BROWSER_KEY, m_availableBrowsers.first()).toString();
}

void WebBrowserSelectionAspect::toMap(QVariantMap &map) const
{
    map.insert(BROWSER_KEY, m_currentBrowser);
}

QString WebBrowserSelectionAspect::currentBrowser() const
{
    return m_currentBrowser;
}

} // namespace Internal
} // namespace Webassembly
