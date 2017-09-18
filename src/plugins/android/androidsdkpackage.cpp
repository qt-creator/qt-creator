/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "androidsdkpackage.h"

#include "utils/algorithm.h"

namespace Android {

AndroidSdkPackage::AndroidSdkPackage(QVersionNumber version, QString sdkStylePathStr,
                                     QObject *parent) :
    QObject(parent),
    m_revision(version),
    m_sdkStylePath(sdkStylePathStr)
{

}

bool AndroidSdkPackage::operator <(const AndroidSdkPackage &other) const
{
    if (typeid(*this) != typeid(other))
        return type() < other.type();
    return displayText() < other.displayText();
}

QString AndroidSdkPackage::displayText() const
{
    return m_displayText;
}

QString AndroidSdkPackage::descriptionText() const
{
    return m_descriptionText;
}

const QVersionNumber &AndroidSdkPackage::revision() const
{
    return m_revision;
}

AndroidSdkPackage::PackageState AndroidSdkPackage::state() const
{
    return m_state;
}

const QString &AndroidSdkPackage::sdkStylePath() const
{
    return m_sdkStylePath;
}

const Utils::FileName &AndroidSdkPackage::installedLocation() const
{
    return m_installedLocation;
}

void AndroidSdkPackage::setDisplayText(const QString &str)
{
    m_displayText = str;
}

void AndroidSdkPackage::setDescriptionText(const QString &str)
{
    m_descriptionText = str;
}

void AndroidSdkPackage::setState(AndroidSdkPackage::PackageState state)
{
    m_state = state;
}

void AndroidSdkPackage::setInstalledLocation(const Utils::FileName &path)
{
    m_installedLocation = path;
    if (m_installedLocation.exists())
        updatePackageDetails();
}

void AndroidSdkPackage::updatePackageDetails()
{

}

SystemImage::SystemImage(QVersionNumber version, QString sdkStylePathStr, QString abi,
                         SdkPlatform *platform):
    AndroidSdkPackage(version, sdkStylePathStr, platform),
    m_platform(platform),
    m_abiName(abi)
{
}

bool SystemImage::isValid() const
{
    return m_platform && m_platform->isValid();
}

AndroidSdkPackage::PackageType SystemImage::type() const
{
    return SystemImagePackage;
}

const QString &SystemImage::abiName() const
{
    return m_abiName;
}

const SdkPlatform *SystemImage::platform() const
{
    return m_platform.data();
}

void SystemImage::setPlatform(SdkPlatform *platform)
{
    m_platform = platform;
}

SdkPlatform::SdkPlatform(QVersionNumber version, QString sdkStylePathStr, int api, QObject *parent) :
    AndroidSdkPackage(version, sdkStylePathStr, parent),
    m_apiLevel(api)
{
    setDisplayText(QString("android-%1")
                   .arg(m_apiLevel != -1 ? QString::number(m_apiLevel) : "Unknown"));
}

SdkPlatform::~SdkPlatform()
{
    for (SystemImage *image : m_systemImages)
        delete image;
    m_systemImages.clear();
}

bool SdkPlatform::isValid() const
{
    return m_apiLevel != -1;
}

AndroidSdkPackage::PackageType SdkPlatform::type() const
{
    return SdkPlatformPackage;
}

bool SdkPlatform::operator <(const AndroidSdkPackage &other) const
{
    if (typeid(*this) != typeid(other))
        return AndroidSdkPackage::operator <(other);

    const SdkPlatform &platform = static_cast<const SdkPlatform &>(other);
    if (platform.m_apiLevel == m_apiLevel)
        return AndroidSdkPackage::operator <(other);

    return platform.m_apiLevel < m_apiLevel;
}

int SdkPlatform::apiLevel() const
{
    return m_apiLevel;
}

QVersionNumber SdkPlatform::version() const
{
    return m_version;
}

void SdkPlatform::addSystemImage(SystemImage *image)
{
    // Ordered insert. Installed images on top with lexical comparison of the display name.
    auto itr = m_systemImages.begin();
    while (itr != m_systemImages.end()) {
        SystemImage *currentImage = *itr;
        if (currentImage->state() == image->state()) {
            if (currentImage->displayText() > image->displayText())
                break;
        } else if (currentImage->state() > image->state()) {
            break;
        }
        ++itr;
    }
    m_systemImages.insert(itr, image);
    image->setPlatform(this);
}

SystemImageList SdkPlatform::systemImages(PackageState state) const
{
    return Utils::filtered(m_systemImages, [state](const SystemImage *image) {
        return image->state() & state;
    });
}

BuildTools::BuildTools(QVersionNumber revision, QString sdkStylePathStr, QObject *parent):
    AndroidSdkPackage(revision, sdkStylePathStr, parent)
{
}

bool BuildTools::isValid() const
{
    return true;
}

AndroidSdkPackage::PackageType BuildTools::type() const
{
    return AndroidSdkPackage::BuildToolsPackage;
}

SdkTools::SdkTools(QVersionNumber revision, QString sdkStylePathStr, QObject *parent):
    AndroidSdkPackage(revision, sdkStylePathStr, parent)
{

}

bool SdkTools::isValid() const
{
    return true;
}

AndroidSdkPackage::PackageType SdkTools::type() const
{
    return AndroidSdkPackage::SdkToolsPackage;
}

PlatformTools::PlatformTools(QVersionNumber revision, QString sdkStylePathStr, QObject *parent):
    AndroidSdkPackage(revision, sdkStylePathStr, parent)
{

}

bool PlatformTools::isValid() const
{
    return true;
}

AndroidSdkPackage::PackageType PlatformTools::type() const
{
    return AndroidSdkPackage::PlatformToolsPackage;
}

} // namespace Android
