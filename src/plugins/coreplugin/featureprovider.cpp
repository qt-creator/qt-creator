// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "featureprovider.h"

/*!
    \class Core::IFeatureProvider
    \inheaderfile coreplugin/featureprovider.h
    \inmodule QtCreator
    \ingroup mainclasses

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
    \fn IFeatureProvider::~IFeatureProvider()
    \internal
*/
