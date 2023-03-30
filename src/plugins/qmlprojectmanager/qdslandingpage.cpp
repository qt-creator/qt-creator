// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdslandingpage.h"
#include "projectfilecontenttools.h"
#include "qdslandingpagetheme.h"
#include "qmlprojectconstants.h"
#include "qmlprojectgen/qmlprojectgenerator.h"
#include "qmlprojectplugin.h"
#include "utils/algorithm.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QQuickItem>
#include <QtQml/QQmlEngine>

namespace QmlProjectManager {
namespace Internal {

const char INSTALL_QDS_URL[] = "https://www.qt.io/product/ui-design-tools";
const char OBJECT_NAME_LANDING_PAGE[] = "QQuickWidgetQDSLandingPage";

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

        const QString resourcePath
            = Core::ICore::resourcePath(QmlProjectManager::Constants::QML_RESOURCE_PATH).toString();
        const QString landingPath
            = Core::ICore::resourcePath(QmlProjectManager::Constants::LANDING_PAGE_PATH).toString();

        QdsLandingPageTheme::setupTheme(m_widget->engine());

        m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
        m_widget->setObjectName(OBJECT_NAME_LANDING_PAGE);
        m_widget->engine()->addImportPath(landingPath + "/imports");
        m_widget->engine()->addImportPath(resourcePath);
        m_widget->engine()->addImportPath("qrc:/studiofonts");
        m_widget->setSource(QUrl::fromLocalFile(landingPath + "/main.qml"));
        m_widget->hide();

        layout()->addWidget(m_widget);
    }

    return m_widget;
}

QdsLandingPage::QdsLandingPage()
    : m_widget{nullptr}
{}

void QdsLandingPage::openQtc(bool rememberSelection)
{
    if (rememberSelection)
        Core::ICore::settings()->setValue(QmlProjectManager::Constants::ALWAYS_OPEN_UI_MODE,
                                          Core::Constants::MODE_EDIT);

    hide();

    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

void QdsLandingPage::openQds(bool rememberSelection)
{
    if (rememberSelection)
        Core::ICore::settings()->setValue(QmlProjectManager::Constants::ALWAYS_OPEN_UI_MODE,
                                          Core::Constants::MODE_DESIGN);

    auto editor = Core::EditorManager::currentEditor();
    if (editor)
        QmlProjectPlugin::openInQDSWithProject(editor->document()->filePath());
}

void QdsLandingPage::installQds()
{
    QDesktopServices::openUrl(QUrl(INSTALL_QDS_URL));
}

void QdsLandingPage::generateProjectFile()
{
    GenerateQmlProject::QmlProjectFileGenerator generator;

    Core::IEditor *editor = Core::EditorManager::currentEditor();
    if (!editor)
        return;

    if (generator.prepareForUiQmlFile(editor->document()->filePath())) {
        if (generator.execute()) {
            const QString qtVersion = ProjectFileContentTools::qtVersion(generator.targetFile());
            const QString qdsVersion = ProjectFileContentTools::qdsVersion(generator.targetFile());
            setProjectFileExists(generator.targetFile().exists());
            setQtVersion(qtVersion);
            setQdsVersion(qdsVersion);
        }
    }
}

void QdsLandingPage::setWidget(QWidget *widget)
{
    m_widget = widget;
}

QWidget *QdsLandingPage::widget()
{
    return m_widget;
}

void QdsLandingPage::show()
{
    if (!m_widget)
        return;

    m_widget->show();
}

void QdsLandingPage::hide()
{
    if (!m_widget)
        return;

    m_widget->hide();
}

bool QdsLandingPage::qdsInstalled() const
{
    return m_qdsInstalled;
}

void QdsLandingPage::setQdsInstalled(bool installed)
{
    if (m_qdsInstalled != installed) {
        m_qdsInstalled = installed;
        emit qdsInstalledChanged();
    }
}

bool QdsLandingPage::projectFileExists() const
{
    return m_projectFileExists;
}

void QdsLandingPage::setProjectFileExists(bool exists)
{
    if (m_projectFileExists != exists) {
        m_projectFileExists = exists;
        emit projectFileExistshanged();
    }
}

const QString QdsLandingPage::qtVersion() const
{
    return m_qtVersion;
}

void QdsLandingPage::setQtVersion(const QString &version)
{
    if (m_qtVersion != version) {
        m_qtVersion = version;
        emit qtVersionChanged();
    }
}

const QString QdsLandingPage::qdsVersion() const
{
    return m_qdsVersion;
}

void QdsLandingPage::setQdsVersion(const QString &version)
{
    if (m_qdsVersion != version) {
        m_qdsVersion = version;
        emit qdsVersionChanged();
    }
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
