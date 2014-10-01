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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEFAULTASSISTINTERFACE_H
#define DEFAULTASSISTINTERFACE_H

#include "iassistinterface.h"

#include <utils/qtcoverride.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT DefaultAssistInterface : public IAssistInterface
{
public:
    DefaultAssistInterface(QTextDocument *textDocument,
                           int position,
                           const QString &fileName,
                           AssistReason reason);
    ~DefaultAssistInterface();

    int position() const QTC_OVERRIDE { return m_position; }
    QChar characterAt(int position) const QTC_OVERRIDE;
    QString textAt(int position, int length) const QTC_OVERRIDE;
    QString fileName() const QTC_OVERRIDE { return m_fileName; }
    QTextDocument *textDocument() const QTC_OVERRIDE { return m_textDocument; }
    void prepareForAsyncUse() QTC_OVERRIDE;
    void recreateTextDocument() QTC_OVERRIDE;
    AssistReason reason() const QTC_OVERRIDE;

private:
    QTextDocument *m_textDocument;
    bool m_isAsync;
    int m_position;
    QString m_fileName;
    AssistReason m_reason;
    QString m_text;
};

} // TextEditor

#endif // DEFAULTASSISTINTERFACE_H
