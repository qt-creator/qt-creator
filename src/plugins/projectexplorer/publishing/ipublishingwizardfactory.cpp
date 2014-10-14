/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include <ipublishingwizardfactory.h>


/*!
    \class ProjectExplorer::IPublishingWizardFactory

    \brief The IPublishingWizardFactory class provides an interface for creating
    wizards to publish a project.

    A class implementing this interface is used to create an associated wizard
    that allows users to publish their project to a remote facility, such as an
    app store.

    Such a wizard would typically transform the project content into a format
    expected by that facility (\e packaging) and also upload it, if possible.

    The factory objects have to be added to the global object pool via
    \c ExtensionSystem::PluginManager::addObject().

    \sa ExtensionSystem::PluginManager::addObject()
*/

/*!
    \fn virtual QString displayName() const = 0

    Describes on one line the type of wizard that this factory can create.
*/

 /*!
   \fn virtual QString description() const = 0

    Explains the exact purpose of the wizard created by this factory.
*/

/*!
    \fn virtual bool canCreateWizard(const Project *project) const = 0

    Returns true if the type of wizard that this factory can create is available
    for the specified \a project.
*/

/*!
    \fn virtual QWizard *createWizard(const Project *project) const = 0

    Creates a wizard that can publish \a project. Behavior is undefined if
    canCreateWizard() returns \c false for the project. Returns the newly
    created publishing wizard

    \sa canCreateWizard()
*/
