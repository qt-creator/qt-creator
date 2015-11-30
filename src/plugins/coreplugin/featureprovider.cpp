/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "featureprovider.h"

/*!
    \class Core::IFeatureProvider
    \mainclass

    \brief The IFeatureProvider class defines an interface to manage features
    for wizards.

    The features provided by an object in the object pool implementing IFeatureProvider
    will be respected by wizards implementing IWizard.

    This feature set, provided by all instances of IFeatureProvider in the
    object pool, is checked against \c IWizard::requiredFeatures()
    and only if all required features are available, the wizard is displayed
    when creating a new file or project.

    The QtSupport plugin creates an instance of IFeatureProvider and provides Qt specific
    features for the available versions of Qt.

    \sa Core::IWizard
    \sa QtSupport::QtVersionManager
*/


/*!
    \fn IFeatureProvider::IFeatureProvider()
    \internal
*/

/*!
    \fn IFeatureProvider::~IFeatureProvider()
    \internal
*/

/*!
    \fn FetureSet IFeatureProvider::availableFeatures(const QString &platform) const
    Returns available features provided by this manager.
    \sa FeatureProvider::Features
*/

/*!
    \class Core::Feature

    \brief The Feature class describes a single feature to be used in
    Core::FeatureProvider::Features.

    \sa Core::FeaturesSet
    \sa Core::IWizard
    \sa QtSupport::QtVersionManager
*/

/*!
    \class Core::FeatureSet

    \brief The FeatureSet class is a set of available or required feature sets.

    This class behaves similarly to QFlags. However, instead of enums, Features
    relies on string ids and is therefore extendable.

    \sa Core::Feature
    \sa Core::IWizard
    \sa QtSupport::QtVersionManager
*/


/*!
    \fn bool FeatureSet::contains(const Feature &feature) const

    Returns true if \a feature is available.
*/

/*!
    \fn bool FeatureSet::contains(const FeatureSet &features) const

    Returns true if all \a features are available.
*/

Core::Feature Core::Feature::versionedFeature(const QByteArray &prefix, int major, int minor)
{
    if (major < 0)
        return Feature::fromName(prefix);

    QByteArray result = prefix + '.';
    result += QString::number(major).toLatin1();

    if (minor < 0)
        return Feature::fromName(result);
    return Feature::fromName(result + '.' + QString::number(minor).toLatin1());
}

Core::FeatureSet Core::FeatureSet::versionedFeatures(const QByteArray &prefix, int major, int minor)
{
    FeatureSet result;
    result |= Feature::fromName(prefix);

    if (major < 0)
        return result;

    const QByteArray majorStr = QString::number(major).toLatin1();
    const QByteArray featureMajor = prefix + majorStr;
    const QByteArray featureDotMajor = prefix + '.' + majorStr;

    result |= Feature::fromName(featureMajor) | Feature::fromName(featureDotMajor);

    for (int i = 0; i <= minor; ++i) {
        const QByteArray minorStr = QString::number(i).toLatin1();
        result |= Feature::fromName(featureMajor + '.' + minorStr)
                | Feature::fromName(featureDotMajor + '.' + minorStr);
    }

    return result;
}
