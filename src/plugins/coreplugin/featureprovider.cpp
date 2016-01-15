/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    \fn IFeatureProvider::~IFeatureProvider()
    \internal
*/
