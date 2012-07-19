/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef IASSISTINTERFACE_H
#define IASSISTINTERFACE_H

#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QString>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QThread;
QT_END_NAMESPACE

namespace Core {
class IDocument;
}

namespace TextEditor {

class TEXTEDITOR_EXPORT IAssistInterface
{
public:
    IAssistInterface();
    virtual ~IAssistInterface();

    virtual int position() const = 0;
    virtual QChar characterAt(int position) const = 0;
    virtual QString textAt(int position, int length) const = 0;
    virtual const Core::IDocument *document() const = 0;
    virtual QTextDocument *textDocument() const = 0;
    virtual void detach(QThread *destination) = 0;
    virtual AssistReason reason() const = 0;
};

} // TextEditor

#endif // IASSISTINTERFACE_H
