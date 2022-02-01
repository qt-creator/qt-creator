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

#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportsdk.h"

#include <coreplugin/icore.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QGridLayout>
#include <QToolButton>

using namespace Utils;

namespace McuSupport {
namespace Internal {

static bool automaticKitCreationFromSettings(QSettings::Scope scope = QSettings::UserScope)
{
    QSettings *settings = Core::ICore::settings(scope);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION);
    bool automaticKitCreation = settings->value(key, true).toBool();
    return automaticKitCreation;
}

McuPackage::McuPackage(const QString &label, const FilePath &defaultPath,
                       const QString &detectionPath, const QString &settingsKey,
                       const QString &envVarName, const QString &downloadUrl,
                       const McuPackageVersionDetector *versionDetector)
    : m_label(label)
    , m_defaultPath(Sdk::packagePathFromSettings(settingsKey, QSettings::SystemScope, defaultPath))
    , m_detectionPath(detectionPath)
    , m_settingsKey(settingsKey)
    , m_versionDetector(versionDetector)
    , m_environmentVariableName(envVarName)
    , m_downloadUrl(downloadUrl)
{
    m_path = Sdk::packagePathFromSettings(settingsKey, QSettings::UserScope, m_defaultPath);
    m_automaticKitCreation = automaticKitCreationFromSettings(QSettings::UserScope);
}

FilePath McuPackage::basePath() const
{
    return m_fileChooser != nullptr ? m_fileChooser->filePath() : m_path;
}

FilePath McuPackage::path() const
{
    return basePath().resolvePath(m_relativePathModifier).absoluteFilePath();
}

QString McuPackage::label() const
{
    return m_label;
}

FilePath McuPackage::defaultPath() const
{
    return m_defaultPath;
}

QString McuPackage::detectionPath() const
{
    return m_detectionPath;
}

QWidget *McuPackage::widget()
{
    if (m_widget)
        return m_widget;

    m_widget = new QWidget;
    m_fileChooser = new PathChooser;
    m_fileChooser->lineEdit()->setButtonIcon(FancyLineEdit::Right,
                                             Icons::RESET.icon());
    m_fileChooser->lineEdit()->setButtonVisible(FancyLineEdit::Right, true);
    connect(m_fileChooser->lineEdit(), &FancyLineEdit::rightButtonClicked, this, [&] {
        m_fileChooser->setFilePath(m_defaultPath);
    });

    auto layout = new QGridLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_infoLabel = new InfoLabel();

    if (!m_downloadUrl.isEmpty()) {
        auto downLoadButton = new QToolButton;
        downLoadButton->setIcon(Icons::ONLINE.icon());
        downLoadButton->setToolTip(tr("Download from \"%1\"").arg(m_downloadUrl));
        QObject::connect(downLoadButton, &QToolButton::pressed, this, [this] {
            QDesktopServices::openUrl(m_downloadUrl);
        });
        layout->addWidget(downLoadButton, 0, 2);
    }

    layout->addWidget(m_fileChooser, 0, 0, 1, 2);
    layout->addWidget(m_infoLabel, 1, 0, 1, -1);

    m_fileChooser->setFilePath(m_path);

    QObject::connect(this, &McuPackage::statusChanged, this, [this] {
        updateStatusUi();
    });

    QObject::connect(m_fileChooser, &PathChooser::pathChanged, this, [this] {
        updatePath();
        emit changed();
    });

    updateStatus();
    return m_widget;
}

McuPackage::Status McuPackage::status() const
{
    return m_status;
}

bool McuPackage::validStatus() const
{
    return m_status == McuPackage::ValidPackage || m_status == McuPackage::ValidPackageMismatchedVersion;
}

const QString &McuPackage::environmentVariableName() const
{
    return m_environmentVariableName;
}

void McuPackage::setAddToPath(bool addToPath)
{
    m_addToPath = addToPath;
}

bool McuPackage::addToPath() const
{
    return m_addToPath;
}

void McuPackage::writeGeneralSettings() const
{
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION);
    QSettings *settings = Core::ICore::settings();
    settings->setValue(key, m_automaticKitCreation);
}

bool McuPackage::writeToSettings() const
{
    const FilePath savedPath = Sdk::packagePathFromSettings(m_settingsKey, QSettings::UserScope, m_defaultPath);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/' +
            QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey;
    Core::ICore::settings()->setValueWithDefault(key, m_path.toString(), m_defaultPath.toString());

    return savedPath != m_path;
}

void McuPackage::setRelativePathModifier(const QString &path)
{
    m_relativePathModifier = path;
}

void McuPackage::setVersions(const QStringList &versions)
{
    m_versions = versions;
}

bool McuPackage::automaticKitCreationEnabled() const
{
    return m_automaticKitCreation;
}

void McuPackage::setAutomaticKitCreationEnabled(const bool enabled)
{
    m_automaticKitCreation = enabled;
}

void McuPackage::updatePath()
{
   m_path = m_fileChooser->rawFilePath();
   m_fileChooser->lineEdit()->button(FancyLineEdit::Right)->setEnabled(m_path != m_defaultPath);
   updateStatus();
}

void McuPackage::updateStatus()
{
    bool validPath = !m_path.isEmpty() && m_path.exists();
    const FilePath detectionPath = basePath() / m_detectionPath;
    const bool validPackage = m_detectionPath.isEmpty() || detectionPath.exists();
    m_detectedVersion = validPath && validPackage && m_versionDetector
            ? m_versionDetector->parseVersion(basePath().toString()) : QString();
    const bool validVersion = m_detectedVersion.isEmpty() ||
            m_versions.isEmpty() || m_versions.contains(m_detectedVersion);

    m_status = validPath ?
                ( validPackage ?
                      (validVersion ? ValidPackage : ValidPackageMismatchedVersion)
                    : ValidPathInvalidPackage )
              : m_path.isEmpty() ? EmptyPath : InvalidPath;

    emit statusChanged();
}

void McuPackage::updateStatusUi()
{
    switch (m_status) {
    case ValidPackage: m_infoLabel->setType(InfoLabel::Ok); break;
    case ValidPackageMismatchedVersion: m_infoLabel->setType(InfoLabel::Warning); break;
    default: m_infoLabel->setType(InfoLabel::NotOk); break;
    }
    m_infoLabel->setText(statusText());
}

QString McuPackage::statusText() const
{
    const QString displayPackagePath = m_path.toUserOutput();
    const QString displayVersions = m_versions.join(" or ");
    const QString outDetectionPath = FilePath::fromString(m_detectionPath).toUserOutput();
    const QString displayRequiredPath = m_versions.empty() ?
                outDetectionPath :
                QString("%1 %2").arg(outDetectionPath, displayVersions);
    const QString displayDetectedPath = m_versions.empty() ?
                outDetectionPath :
                QString("%1 %2").arg(outDetectionPath, m_detectedVersion);

    QString response;
    switch (m_status) {
    case ValidPackage:
        response = m_detectionPath.isEmpty()
                ? ( m_detectedVersion.isEmpty()
                    ? tr("Path %1 exists.").arg(displayPackagePath)
                    : tr("Path %1 exists. Version %2 was found.")
                      .arg(displayPackagePath, m_detectedVersion) )
                : tr("Path %1 is valid, %2 was found.")
                  .arg(displayPackagePath, displayDetectedPath);
        break;
    case ValidPackageMismatchedVersion: {
        const QString versionWarning = m_versions.size() == 1 ?
                    tr("but only version %1 is supported").arg(m_versions.first()) :
                    tr("but only versions %1 are supported").arg(displayVersions);
        response = tr("Path %1 is valid, %2 was found, %3.")
                .arg(displayPackagePath, displayDetectedPath, versionWarning);
        break;
    }
    case ValidPathInvalidPackage:
        response = tr("Path %1 exists, but does not contain %2.")
                .arg(displayPackagePath, displayRequiredPath);
        break;
    case InvalidPath:
        response = tr("Path %1 does not exist.").arg(displayPackagePath);
        break;
    case EmptyPath:
        response = m_detectionPath.isEmpty()
                ? tr("Path is empty.")
                : tr("Path is empty, %1 not found.")
                    .arg(displayRequiredPath);
        break;
    }
    return response;
}

McuToolChainPackage::McuToolChainPackage(const QString &label,
                                         const FilePath &defaultPath,
                                         const QString &detectionPath,
                                         const QString &settingsKey,
                                         McuToolChainPackage::Type type, const QString &envVarName,
                                         const McuPackageVersionDetector *versionDetector)
    : McuPackage(label, defaultPath, detectionPath, settingsKey, envVarName, {}, versionDetector)
    , m_type(type)
{
}

McuToolChainPackage::Type McuToolChainPackage::type() const
{
    return m_type;
}

bool McuToolChainPackage::isDesktopToolchain() const
{
    return m_type == TypeMSVC || m_type == TypeGCC;
}

} // namespace Internal
} // namespace McuSupport
