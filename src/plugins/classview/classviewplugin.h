/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CLASSVIEWPLUGIN_H
#define CLASSVIEWPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QScopedPointer>

namespace ClassView {
namespace Internal {

/*!
   \class Plugin
   \brief Base class for Class View plugin (class/namespaces in the navigation pane)
 */

class Plugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_DISABLE_COPY(Plugin)

public:
    //! Constructor
    Plugin();

    //! Destructor
    virtual ~Plugin();

    //! \implements ExtensionSystem::IPlugin::initialize
    bool initialize(const QStringList &arguments, QString *error_message = 0);

    //! \implements ExtensionSystem::IPlugin::extensionsInitialized
    void extensionsInitialized();

private:
    //! private class data pointer
    QScopedPointer<struct PluginPrivate> d_ptr;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWPLUGIN_H
