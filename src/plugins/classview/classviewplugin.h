/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
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

#ifndef CLASSVIEWPLUGIN_H
#define CLASSVIEWPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace ClassView {
namespace Internal {

/*!
   \class Plugin
   \brief Base class for Class View plugin (class/namespaces in the navigation pane)
 */

class Plugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClassView.json")

public:
    //! Constructor
    Plugin() {}

    //! \implements ExtensionSystem::IPlugin::initialize
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);

    //! \implements ExtensionSystem::IPlugin::extensionsInitialized
    void extensionsInitialized() {}
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWPLUGIN_H
