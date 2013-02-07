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

#ifndef ICORELISTENER_H
#define ICORELISTENER_H

#include "core_global.h"
#include <QObject>

namespace Core {
class IEditor;
/*!
  \class Core::ICoreListener

  \brief Provides a hook for plugins to veto on certain events emitted from
the core plugin.

  You implement this interface if you want to prevent certain events from
  occurring, e.g.  if you want to prevent the closing of the whole application
  or to prevent the closing of an editor window under certain conditions.

  If e.g. the application window requests a close, then first
  ICoreListener::coreAboutToClose() is called (in arbitrary order) on all
  registered objects implementing this interface. If one if these calls returns
  false, the process is aborted and the event is ignored.  If all calls return
  true, the corresponding signal is emitted and the event is accepted/performed.

  Guidelines for implementing:
  \list
  \li Return false from the implemented method if you want to prevent the event.
  \li You need to add your implementing object to the plugin managers objects:
     ExtensionSystem::PluginManager::instance()->addObject(yourImplementingObject);
  \li Don't forget to remove the object again at deconstruction (e.g. in the destructor of
     your plugin).
*/
class CORE_EXPORT ICoreListener : public QObject
{
    Q_OBJECT
public:
    ICoreListener(QObject *parent = 0) : QObject(parent) {}
    virtual ~ICoreListener() {}

    virtual bool editorAboutToClose(IEditor * /*editor*/) { return true; }
    virtual bool coreAboutToClose() { return true; }
};

} // namespace Core

#endif // ICORELISTENER_H
