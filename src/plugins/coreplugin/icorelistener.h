/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef ICORELISTENER_H
#define ICORELISTENER_H

#include "core_global.h"
#include <coreplugin/editormanager/ieditor.h>
#include <QtCore/QObject>

namespace Core {

/*!
  \class Core::ICoreListener
  \brief Provides a hook for plugins to veto on certain events emitted from the core plugin.

  You implement this interface if you want to prevent certain events from occurring, e.g.
  if you want to prevent the closing of the whole application or to prevent the closing
  of an editor window under certain conditions.

  If e.g. the application window requests a close, then first
  ICoreListener::coreAboutToClose() is called (in arbitrary order)
  on all registered objects implementing this interface. If one if these calls returns
  false, the process is aborted and the event is ignored.
  If all calls return true, the corresponding signal is emitted and the event is accepted/performed.

  Guidelines for implementing:
  \list
  \o Return false from the implemented method if you want to prevent the event.
  \o You need to add your implementing object to the plugin managers objects:
     ICore::pluginManager()->addObject(yourImplementingObject);
  \o Don't forget to remove the object again at deconstruction (e.g. in the destructor of
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
