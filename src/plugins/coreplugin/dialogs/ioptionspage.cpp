/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "ioptionspage.h"

/*!
  \class Core::IOptionsPage
  \mainclass
  \brief The IOptionsPage is an interface for providing options pages.

  You need to subclass this interface and put an instance of your subclass
  into the plugin manager object pool (e.g. ExtensionSystem::PluginManager::addObject).
  Guidelines for implementing:
  \list
  \o id() is a unique identifier for referencing this page
  \o displayName() is the (translated) name for display
  \o category() is the unique id for the category that the page should be displayed in
  \o displayCategory() is the translated name of the category
  \o createPage() is called to retrieve the widget to show in the preferences dialog
     The widget will be destroyed by the widget hierarchy when the dialog closes
  \o apply() is called to store the settings. It should detect if any changes have been
         made and store those
  \o finish() is called directly before the preferences dialog closes
  \o matches() is used for the options dialog search filter
  \endlist
*/
