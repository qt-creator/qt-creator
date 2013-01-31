/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "featureprovider.h"

/*!
    \class Core::IFeatureProvider
    \mainclass

    \brief The class IFeatureProvider defines an interface to manage features for wizards

    The features provided by an object in the object pool implementing IFeatureProvider
    will be respected by wizards implementing IWizard.

    This feature set, provided by all instances of IFeatureProvider in the object pool, is checked against IWizard::requiredFeatures()
    and only if all required features are available the wizard is displayed when creating a new file or project.

    Qt4VersionManager creates an instance of IFeatureProvider and provides Qt specific features for the available
    versions of Qt.

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
    \fn FeatureProvider::Features availableFeatures() const;
    Returns available features provided by this manager.
    \sa FeatureProvider::Features
*/

/*!
    \class Core::Feature

    \brief This class describes a single Feature

    This class describes a single Feature to be used
    in Core::FeatureProvider::Features.

    \sa Core::FeaturesSet
    \sa Core::IWizard
    \sa QtSupport::QtVersionManager
*/

/*!
    \class Core::FeatureSet

    \brief This class is a set of features

    Features is used to describe available or
    required feature sets and behaves similar to QFlags.

    But instead of enums Features relies on string ids
    and is therefore extendable.

    \sa Core::Feature
    \sa Core::IWizard
    \sa QtSupport::QtVersionManager
*/


/*!
    \fn bool Features::~contains(const Feature &feature)()

    \returns true if the \param features is available.
*/

/*!
    \fn bool Features::~contains(const Features &features)()

    \returns true if all \param features are available.
*/
