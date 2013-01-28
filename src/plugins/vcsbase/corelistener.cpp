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

#include "corelistener.h"
#include "vcsbasesubmiteditor.h"

/*!
    \class VcsBase::Internal::CoreListener

    \brief CoreListener catching closing of a submit editor.

    Catch the closing of a submit editor to trigger the submit.
    One instance of this class exists, connected to the instances
    of VcsBasePlugin, which dispatch if the editor kind matches theirs
    (which is why the approach of passing the bool result was chosen).
*/

namespace VcsBase {
namespace Internal {

CoreListener::CoreListener(QObject *parent) :
    ICoreListener(parent)
{
}

bool CoreListener::editorAboutToClose(Core::IEditor *editor)
{
    bool result = true;
    if (editor)
        if (VcsBase::VcsBaseSubmitEditor *se = qobject_cast<VcsBase::VcsBaseSubmitEditor *>(editor))
            emit submitEditorAboutToClose(se, &result);
    return result;
}

} // namespace Internal
} // namespace VcsBase
