/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEFAULTASSISTINTERFACE_H
#define DEFAULTASSISTINTERFACE_H

#include "iassistinterface.h"

namespace TextEditor {

class TEXTEDITOR_EXPORT DefaultAssistInterface : public IAssistInterface
{
public:
    DefaultAssistInterface(QTextDocument *document,
                           int position,
                           Core::IFile *file,
                           AssistReason reason);
    virtual ~DefaultAssistInterface();

    virtual int position() const { return m_position; }
    virtual QChar characterAt(int position) const;
    virtual QString textAt(int position, int length) const;
    virtual const Core::IFile *file() const { return m_file; }
    virtual QTextDocument *document() const { return m_document; }
    virtual void detach(QThread *destination);
    virtual AssistReason reason() const;

private:
    QTextDocument *m_document;
    bool m_detached;
    int m_position;
    Core::IFile *m_file;
    AssistReason m_reason;
};

} // TextEditor

#endif // DEFAULTASSISTINTERFACE_H
