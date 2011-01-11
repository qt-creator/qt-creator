/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CORELISTENER_H
#define CORELISTENER_H

#include <coreplugin/icorelistener.h>

namespace VCSBase {

class VCSBaseSubmitEditor;

namespace Internal {

// Catch the closing of a submit editor to trigger the submit.
// One instance of this class exists, connected to the instances
// of VCSBasePlugin, which dispatch if the editor kind matches theirs
// (which is why the approach of passing the bool result was chosen).

class CoreListener : public Core::ICoreListener
{
    Q_OBJECT
public:
    explicit CoreListener(QObject *parent = 0);
    virtual bool editorAboutToClose(Core::IEditor *editor);

signals:
    void submitEditorAboutToClose(VCSBaseSubmitEditor *e, bool *result);
};

}
}
#endif // CORELISTENER_H
