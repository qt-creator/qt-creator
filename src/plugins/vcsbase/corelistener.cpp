/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "corelistener.h"
#include "vcsbasesubmiteditor.h"

/*!
    \class VcsBase::Internal::CoreListener

    \brief The CoreListener class catches the closing of a submit editor.

    Catches the closing of a submit editor to trigger the submit.
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
