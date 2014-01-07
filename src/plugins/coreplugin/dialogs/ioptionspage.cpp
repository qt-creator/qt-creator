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

#include "ioptionspage.h"

/*!
  \class Core::IOptionsPage
  \mainclass
  \brief The IOptionsPage class is an interface for providing pages for the
  \gui Options dialog (called \gui Preferences on Mac OS).

  You need to subclass this interface and put an instance of your subclass
  into the plugin manager object pool (e.g. ExtensionSystem::PluginManager::addObject).
  Guidelines for implementing:
  \list
  \li \c id() is a unique identifier for referencing this page
  \li \c displayName() is the (translated) name for display
  \li \c category() is the unique id for the category that the page should be displayed in
  \li \c displayCategory() is the translated name of the category
  \li \c createPage() is called to retrieve the widget to show in the
        \gui Options dialog
     The widget will be destroyed by the widget hierarchy when the dialog closes
  \li \c apply() is called to store the settings. It should detect if any changes have been
         made and store those
  \li \c finish() is called directly before the \gui Options dialog closes
  \li \c matches() is used for the \gui Options dialog search filter
  \endlist
*/
