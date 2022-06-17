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
#include <QHBoxLayout>
#include <QQuickItem>

namespace QmlProjectManager {
namespace Internal {

const char QMLRESOURCEPATH[] = "qmldesigner/propertyEditorQmlSources/imports";
const char LANDINGPAGEPATH[] = "qmldesigner/landingpage";
const char PROPERTY_QDSINSTALLED[] = "qdsInstalled";
const char PROPERTY_PROJECTFILEEXISTS[] = "projectFileExists";
const char PROPERTY_QTVERSION[] = "qtVersion";
const char PROPERTY_QDSVERSION[] = "qdsVersion";
const char PROPERTY_CMAKES[] = "cmakeLists";
const char PROPERTY_REMEMBER[] = "rememberSelection";

QdsLandingPageWidget::QdsLandingPageWidget(QWidget* parent)
    : QWidget(parent)
{
    setLayout(new QHBoxLayout());
    layout()->setContentsMargins(0, 0, 0, 0);
}

QdsLandingPageWidget::~QdsLandingPageWidget()
{
    if (m_widget)
        m_widget->deleteLater();
}

QQuickWidget *QdsLandingPageWidget::widget()
{
    if (!m_widget) {
        m_widget = new QQuickWidget();
        layout()->addWidget(m_widget);
    }

    return m_widget;
}

QdsLandingPage::QdsLandingPage(QdsLandingPageWidget *widget, QWidget *parent)
    : m_widget{widget->widget()}
{
    Q_UNUSED(parent)

    setParent(m_widget);

    const QString resourcePath = Core::ICore::resourcePath(QMLRESOURCEPATH).toString();
    const QString landingPath = Core::ICore::resourcePath(LANDINGPAGEPATH).toString();

    qmlRegisterSingletonInstance<QdsLandingPage>("LandingPageApi", 1, 0, "LandingPageApi", this);
    QdsLandingPageTheme::setupTheme(m_widget->engine());

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->engine()->addImportPath(landingPath + "/imports");
    m_widget->engine()->addImportPath(resourcePath);
    m_widget->setSource(QUrl::fromLocalFile(landingPath + "/main.qml"));

    if (m_widget->rootObject()) { // main.qml only works with Qt6
        connect(m_widget->rootObject(), SIGNAL(openQtc(bool)), this, SIGNAL(openCreator(bool)));
        connect(m_widget->rootObject(), SIGNAL(openQds(bool)), this, SIGNAL(openDesigner(bool)));
        connect(m_widget->rootObject(), SIGNAL(installQds()), this, SIGNAL(installDesigner()));
        connect(m_widget->rootObject(), SIGNAL(generateCmake()), this, SIGNAL(generateCmake()));
        connect(m_widget->rootObject(), SIGNAL(generateProjectFile()), this, SIGNAL(generateProjectFile()));
    }
    m_widget->hide();
}

QWidget *QdsLandingPage::widget()
{
    return m_widget;
}

void QdsLandingPage::show()
{
    if (m_widget->rootObject()) {
        m_widget->rootObject()->setProperty(PROPERTY_QDSINSTALLED, m_qdsInstalled);
        m_widget->rootObject()->setProperty(PROPERTY_PROJECTFILEEXISTS, m_projectFileExists);
        m_widget->rootObject()->setProperty(PROPERTY_QTVERSION, m_qtVersion);
        m_widget->rootObject()->setProperty(PROPERTY_QDSVERSION, m_qdsVersion);
        m_widget->rootObject()->setProperty(PROPERTY_CMAKES, m_cmakeResources);
        m_widget->rootObject()->setProperty(PROPERTY_REMEMBER, Qt::Unchecked);
    }
    m_widget->show();
}

void QdsLandingPage::hide()
{
    m_widget->hide();
}

bool QdsLandingPage::qdsInstalled() const
{
    return m_qdsInstalled;
}

void QdsLandingPage::setQdsInstalled(bool installed)
{
    m_qdsInstalled = installed;
    if (m_widget->rootObject())
        m_widget->rootObject()->setProperty(PROPERTY_QDSINSTALLED, installed);
}

bool QdsLandingPage::projectFileExists() const
{
    return m_projectFileExists;
}

void QdsLandingPage::setProjectFileExists(bool exists)
{
    m_projectFileExists = exists;
    if (m_widget->rootObject())
        m_widget->rootObject()->setProperty(PROPERTY_PROJECTFILEEXISTS, exists);
}

const QString QdsLandingPage::qtVersion() const
{
    return m_qtVersion;
}

void QdsLandingPage::setQtVersion(const QString &version)
{
    m_qtVersion = version;
    if (m_widget->rootObject())
        m_widget->rootObject()->setProperty(PROPERTY_QTVERSION, version);
}

const QString QdsLandingPage::qdsVersion() const
{
    return m_qdsVersion;
}

void QdsLandingPage::setQdsVersion(const QString &version)
{
    m_qdsVersion = version;
    if (m_widget->rootObject())
        m_widget->rootObject()->setProperty(PROPERTY_QDSVERSION, version);
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
