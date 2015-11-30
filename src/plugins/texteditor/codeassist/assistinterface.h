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

#ifndef IASSISTINTERFACE_H
#define IASSISTINTERFACE_H

#include "assistenums.h"

#include <texteditor/texteditor_global.h>


#include <QString>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT AssistInterface
{
public:
    AssistInterface(QTextDocument *textDocument,
                    int position,
                    const QString &fileName,
                    AssistReason reason);
    virtual ~AssistInterface();

    virtual int position() const { return m_position; }
    virtual QChar characterAt(int position) const;
    virtual QString textAt(int position, int length) const;
    virtual QString fileName() const { return m_fileName; }
    virtual QTextDocument *textDocument() const { return m_textDocument; }
    virtual void prepareForAsyncUse();
    virtual void recreateTextDocument();
    virtual AssistReason reason() const;

private:
    QTextDocument *m_textDocument;
    bool m_isAsync;
    int m_position;
    QString m_fileName;
    AssistReason m_reason;
    QString m_text;
};

} // namespace TextEditor

#endif // IASSISTINTERFACE_H
