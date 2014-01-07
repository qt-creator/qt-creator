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

#include "icorelistener.h"

/*!
    \class Core::ICoreListener

    \brief The ICoreListener class provides a hook for plugins to veto on
    certain events emitted from the core plugin.

    Implement this interface to prevent certain events from occurring. For
    example, to prevent the closing of the whole application
    or to prevent the closing of an editor window under certain conditions.

    For example, if the application window requests a close,
    \c ICoreListener::coreAboutToClose() is called (in arbitrary order) on all
    registered objects implementing this interface. If one if these calls
    returns \c false, the process is aborted and the event is ignored.  If all
    calls return \c true, the corresponding signal is emitted and the event is
    accepted or performed.

    Guidelines for implementing the class:
    \list
        \li Return \c false from the implemented function if you want to prevent
            the event.
        \li Add your implementing object to the plugin managers objects:
            \c{ExtensionSystem::PluginManager::instance()->addObject(yourImplementingObject)}
        \li Do not forget to remove the object again at deconstruction
            (for example, in the destructor of your plugin).
    \endlist
*/
