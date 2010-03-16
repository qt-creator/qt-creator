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

#include "ieditor.h"

/*!
  \class Core::IEditor
  \brief The IEditor is an interface for providing different editors for different file types.

  Classes that implement this interface are for example the editors for
  C++ files, ui-files and resource files.

  Whenever a user wants to edit or create a file, the EditorManager scans all
  EditorFactoryInterfaces for suitable editors. The selected EditorFactory
  is then asked to create an editor, which must implement this interface.

  Guidelines for implementing:
  \list
  \o displayName() is used as a user visible description of the document (usually filename w/o path).
  \o kind() must be the same value as the kind() of the corresponding EditorFactory.
  \o The changed() signal should be emitted when the modified state of the document changes
     (so /bold{not} every time the document changes, but /bold{only once}).
  \o If duplication is supported, you need to ensure that all duplicates
        return the same file().
  \o QString preferredMode() const is the mode the editor manager should activate.
     Some editors use a special mode (such as Design mode).
  \endlist

  \sa Core::EditorFactoryInterface Core::IContext

*/
