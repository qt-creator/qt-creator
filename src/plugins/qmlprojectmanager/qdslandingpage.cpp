/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qdslandingpage.h"
#include "qdslandingpagetheme.h"
#include "utils/algorithm.h"

#include <coreplugin/icore.h>

#include <QtQml/QQmlEngine>
#include <QQuickItem>

namespace QmlProjectManager {
namespace Internal {

const char QMLRESOURCEPATH[] = "qmldesigner/propertyEditorQmlSources/imports";
const char LANDINGPAGEPATH[] = "qmldesigner/landingpage";

QdsLandingPage::QdsLandingPage(QWidget *parent)
    : m_dialog{new QQuickWidget(parent)}
{
    setParent(m_dialog);

    const QString resourcePath = Core::ICore::resourcePath(QMLRESOURCEPATH).toString();
    const QString landingPath = Core::ICore::resourcePath(LANDINGPAGEPATH).toString();

    qmlRegisterSingletonInstance<QdsLandingPage>("LandingPageApi", 1, 0, "LandingPageApi", this);
    QdsLandingPageTheme::setupTheme(m_dialog->engine());

    m_dialog->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_dialog->engine()->addImportPath(landingPath + "/imports");
    m_dialog->engine()->addImportPath(resourcePath);
    m_dialog->setSource(QUrl::fromLocalFile(landingPath + "/main.qml"));

    if (m_dialog->rootObject()) { // main.qml only works with Qt6
        connect(m_dialog->rootObject(), SIGNAL(openQtc(bool)), this, SIGNAL(openCreator(bool)));
        connect(m_dialog->rootObject(), SIGNAL(openQds(bool)), this, SIGNAL(openDesigner(bool)));
        connect(m_dialog->rootObject(), SIGNAL(installQds()), this, SIGNAL(installDesigner()));
        connect(m_dialog->rootObject(), SIGNAL(generateCmake()), this, SIGNAL(generateCmake()));
        connect(m_dialog->rootObject(), SIGNAL(generateProjectFile()), this, SIGNAL(generateProjectFile()));
    }
    m_dialog->hide();
}

QWidget* QdsLandingPage::dialog()
{
    return m_dialog;
}

void QdsLandingPage::show()
{
    m_dialog->rootObject()->setProperty("qdsInstalled", m_qdsInstalled);
    m_dialog->rootObject()->setProperty("projectFileExists", m_projectFileExists);
    m_dialog->rootObject()->setProperty("qtVersion", m_qtVersion);
    m_dialog->rootObject()->setProperty("qdsVersion", m_qdsVersion);
    m_dialog->rootObject()->setProperty("cmakeLists", m_cmakeResources);
    m_dialog->rootObject()->setProperty("rememberSelection", Qt::Unchecked);
    m_dialog->show();
}

void QdsLandingPage::hide()
{
    m_dialog->hide();
}

bool QdsLandingPage::qdsInstalled() const
{
    return m_qdsInstalled;
}

void QdsLandingPage::setQdsInstalled(bool installed)
{
    m_qdsInstalled = installed;
}

bool QdsLandingPage::projectFileExists() const
{
    return m_projectFileExists;
}

void QdsLandingPage::setProjectFileExists(bool exists)
{
    m_projectFileExists = exists;
}

const QString QdsLandingPage::qtVersion() const
{
    return m_qtVersion;
}

void QdsLandingPage::setQtVersion(const QString &version)
{
    m_qtVersion = version;
}

const QString QdsLandingPage::qdsVersion() const
{
    return m_qdsVersion;
}

void QdsLandingPage::setQdsVersion(const QString &version)
{
    m_qdsVersion = version;
}

const QStringList QdsLandingPage::cmakeResources() const
{
    return m_cmakeResources;
}

void QdsLandingPage::setCmakeResources(const Utils::FilePaths &resources)
{
    QStringList strings = Utils::transform(resources,
                                           [](const Utils::FilePath &path)
                                                { return path.fileName(); });
    setCmakeResources(strings);
}

void QdsLandingPage::setCmakeResources(const QStringList &resources)
{
    m_cmakeResources = resources;
}

} // namespace Internal
} // namespace QmlProjectManager
