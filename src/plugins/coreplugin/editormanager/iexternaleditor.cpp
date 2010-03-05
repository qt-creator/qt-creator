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

#include "iexternaleditor.h"

/*!
    \class Core::IExternalEditor
    \mainclass

    \brief Core::IExternalEditor allows for registering an external
    Editor in the \gui{Open With...} dialogs .
*/

/*!
    \fn IExternalEditor::IExternalEditor(QObject *parent)
    \internal
*/

/*!
    \fn IExternalEditor::~IExternalEditor()
    \internal
*/

/*!
    \fn QStringList IExternalEditor::mimeTypes() const
    Returns the mime type the editor supports
*/

/*!
    \fn QString IExternalEditor::kind() const
    Returns the editor kind (identifying string).
*/

/*!

    \fn bool IExternalEditor::startEditor(const QString &fileName, QString *errorMessage) = 0;

    Opens the editor with \param fileName. Returns true on success or false
    on failure along with the error in \param errorMessage.
*/
