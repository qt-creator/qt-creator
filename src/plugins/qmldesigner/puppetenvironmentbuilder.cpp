// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "puppetenvironmentbuilder.h"
#include "designersettings.h"
#include "qmldesignerplugin.h"

#include <model.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <qmlprojectmanager/qmlmultilanguageaspect.h>
#include <qmlprojectmanager/qmlproject.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversions.h>

#include <QLibraryInfo>

namespace QmlDesigner {

namespace {
Q_LOGGING_CATEGORY(puppetEnvirmentBuild, "qtc.puppet.environmentBuild", QtWarningMsg)

void filterOutQtBaseImportPath(QStringList *stringList)
{
    Utils::erase(*stringList, [](const QString &string) {
        QDir dir(string);
        return dir.dirName() == "qml"
               && !dir.entryInfoList(QStringList("QtTest"), QDir::Dirs).isEmpty();
    });
}
} // namespace

QProcessEnvironment PuppetEnvironmentBuilder::processEnvironment() const
{
    qCInfo(puppetEnvirmentBuild) << Q_FUNC_INFO;
    m_availablePuppetType = determinePuppetType();
    m_environment = Utils::Environment::systemEnvironment();

    addKit();
    addRendering();
    addControls();
    addPixelRatio();
    addVirtualKeyboard();
    addQuick3D();
    addForceQApplication();
    addMultiLanguageDatatbase();
    addImportPaths();
    addCustomFileSelectors();
    addDisableDeferredProperties();
    addResolveUrlsOnAssignment();

    qCInfo(puppetEnvirmentBuild) << "Puppet environment:" << m_environment.toStringList();

    return m_environment.toProcessEnvironment();
}

QProcessEnvironment PuppetEnvironmentBuilder::createEnvironment(
    ProjectExplorer::Target *target,
    const DesignerSettings &designerSettings,
    const Model &model,
    const Utils::FilePath &qmlPuppetPath)
{
    PuppetEnvironmentBuilder builder{target, designerSettings, model, qmlPuppetPath};
    return builder.processEnvironment();
}

bool PuppetEnvironmentBuilder::usesVirtualKeyboard() const
{
    if (m_target) {
        auto *qmlbuild = qobject_cast<QmlProjectManager::QmlBuildSystem *>(m_target->buildSystem());

        const Utils::EnvironmentItem virtualKeyboard("QT_IM_MODULE", "qtvirtualkeyboard");
        return qmlbuild && qmlbuild->environment().indexOf(virtualKeyboard);
    }

    return false;
}

QString PuppetEnvironmentBuilder::getStyleConfigFileName() const
{
    if (m_target) {
        const auto *qmlBuild = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
            m_target->buildSystem());
        if (qmlBuild) {
            const auto &environment = qmlBuild->environment();
            const auto &envVar = std::find_if(
                std::begin(environment), std::end(environment), [](const auto &envVar) {
                    return (envVar.name == u"QT_QUICK_CONTROLS_CONF"
                            && envVar.operation != Utils::EnvironmentItem::SetDisabled);
                });
            if (envVar != std::end(environment)) {
                const auto &sourceFiles = m_target->project()->files(
                    ProjectExplorer::Project::SourceFiles);
                const auto &foundFile = std::find_if(std::begin(sourceFiles),
                                                     std::end(sourceFiles),
                                                     [&](const auto &fileName) {
                                                         return fileName.fileName() == envVar->value;
                                                     });
                if (foundFile != std::end(sourceFiles))
                    return foundFile->toString();
            }
        }
    }

    return {};
}

void PuppetEnvironmentBuilder::addKit() const
{
    if (m_target) {
        if (m_availablePuppetType == PuppetType::Kit) {
            m_target->kit()->addToBuildEnvironment(m_environment);
            const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(m_target->kit());
            if (qt) { // Kits without a Qt version should not have a puppet!
                // Update PATH to include QT_HOST_BINS
                m_environment.prependOrSetPath(qt->hostBinPath());
            }
        }
    }
}

void PuppetEnvironmentBuilder::addRendering() const
{
    m_environment.set("QML_BAD_GUI_RENDER_LOOP", "true");
    m_environment.set("QML_PUPPET_MODE", "true");
    m_environment.set("QML_DISABLE_DISK_CACHE", "true");
    m_environment.set("QMLPUPPET_RENDER_EFFECTS", "true");
    if (!m_environment.hasKey("QT_SCREEN_SCALE_FACTORS") && !m_environment.hasKey("QT_SCALE_FACTOR"))
        m_environment.set("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

    const bool smoothRendering = m_designerSettings.value(DesignerSettingsKey::SMOOTH_RENDERING).toBool();

    if (smoothRendering)
        m_environment.set("QMLPUPPET_SMOOTH_RENDERING", "true");
}

void PuppetEnvironmentBuilder::addControls() const
{
    const QString controlsStyle = m_designerSettings.value(DesignerSettingsKey::CONTROLS_STYLE).toString();

    if (!controlsStyle.isEmpty()) {
        m_environment.set("QT_QUICK_CONTROLS_STYLE", controlsStyle);
        m_environment.set("QT_LABS_CONTROLS_STYLE", controlsStyle);
    }

    const QString styleConfigFileName = getStyleConfigFileName();

    if (!styleConfigFileName.isEmpty())
        m_environment.appendOrSet("QT_QUICK_CONTROLS_CONF", styleConfigFileName);
}

void PuppetEnvironmentBuilder::addPixelRatio() const
{
    m_environment.set("FORMEDITOR_DEVICE_PIXEL_RATIO",
                      QString::number(QmlDesignerPlugin::formEditorDevicePixelRatio()));
}

void PuppetEnvironmentBuilder::addVirtualKeyboard() const
{
    if (usesVirtualKeyboard()) {
        m_environment.set("QT_IM_MODULE", "qtvirtualkeyboard");
        m_environment.set("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", "1");
    }
}

void PuppetEnvironmentBuilder::addQuick3D() const
{
    // set env var if QtQuick3D import exists
    QmlDesigner::Import import = QmlDesigner::Import::createLibraryImport("QtQuick3D", "1.0");
    if (m_model.hasImport(import, true, true))
        m_environment.set("QMLDESIGNER_QUICK3D_MODE", "true");

    import = QmlDesigner::Import::createLibraryImport("QtQuick3D.Particles3D", "1.0");
    if (m_model.hasImport(import, true, true))
        m_environment.set("QMLDESIGNER_QUICK3D_PARTICLES3D_MODE", "true");

    bool particlemode = m_designerSettings.value("particleMode").toBool();
    if (!particlemode)
        m_environment.set("QT_QUICK3D_DISABLE_PARTICLE_SYSTEMS", "1");
    else
        m_environment.set("QT_QUICK3D_EDITOR_PARTICLE_SYSTEMS", "1");
}

void PuppetEnvironmentBuilder::addForceQApplication() const
{
    auto import = QmlDesigner::Import::createLibraryImport("QtCharts", "2.0");
    if (m_model.hasImport(import, true, true)) {
        m_environment.set("QMLDESIGNER_FORCE_QAPPLICATION", "true");
    } else if (m_target) {
        auto bs = qobject_cast<QmlProjectManager::QmlBuildSystem *>(m_target->buildSystem());
        if (bs && bs->widgetApp())
            m_environment.set("QMLDESIGNER_FORCE_QAPPLICATION", "true");
    }
}

void PuppetEnvironmentBuilder::addMultiLanguageDatatbase() const
{
    if (m_target) {
        if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current(m_target)) {
            if (!multiLanguageAspect->databaseFilePath().isEmpty())
                m_environment.set("QT_MULTILANGUAGE_DATABASE",
                                  multiLanguageAspect->databaseFilePath().toString());
        }
    }
}

void PuppetEnvironmentBuilder::addImportPaths() const
{
    QStringList importPaths = m_model.importPaths();

    if (m_availablePuppetType == PuppetType::Fallback)
        filterOutQtBaseImportPath(&importPaths);

    if (m_target) {
        QStringList designerImports = m_target->additionalData("QmlDesignerImportPath").toStringList();
        importPaths.append(designerImports);
    }

    if (m_availablePuppetType == PuppetType::Fallback)
        importPaths.prepend(QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath));

    constexpr auto pathSep = Utils::HostOsInfo::pathListSeparator();
    m_environment.appendOrSet("QML2_IMPORT_PATH", importPaths.join(pathSep), pathSep);

    qCInfo(puppetEnvirmentBuild) << "Puppet import paths:" << importPaths;
}

void PuppetEnvironmentBuilder::addCustomFileSelectors() const
{
    QStringList customFileSelectors;

    if (m_target)
        customFileSelectors = m_target->additionalData("CustomFileSelectorsData").toStringList();

    customFileSelectors.append("DesignMode");

    constexpr auto pathSep = Utils::HostOsInfo::pathListSeparator();
    if (!customFileSelectors.isEmpty())
        m_environment.appendOrSet("QML_FILE_SELECTORS", customFileSelectors.join(","), pathSep);

    qCInfo(puppetEnvirmentBuild) << "Puppet selectors:" << customFileSelectors;
}

void PuppetEnvironmentBuilder::addDisableDeferredProperties() const
{
    m_environment.set("QML_DISABLE_INTERNAL_DEFERRED_PROPERTIES", "true");
}

void PuppetEnvironmentBuilder::addResolveUrlsOnAssignment() const
{
    m_environment.set("QML_COMPAT_RESOLVE_URLS_ON_ASSIGNMENT", "true");
}

PuppetType PuppetEnvironmentBuilder::determinePuppetType() const
{
    if (m_target && m_target->kit() && m_target->kit()->isValid()) {
        if (m_qmlPuppetPath.isExecutableFile())
            return PuppetType::Kit;
    }

    return PuppetType::Fallback;
}

} // namespace QmlDesigner
