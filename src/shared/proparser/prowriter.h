/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROWRITER_H
#define PROWRITER_H

#include "namespace_global.h"

#include <QtCore/QTextStream>

QT_BEGIN_NAMESPACE
class ProFile;
class ProValue;
class ProItem;
class ProBlock;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class ProWriter
{
public:
    bool write(ProFile *profile, const QString &fileName);
    QString contents(ProFile *profile);

protected:
    QString fixComment(const QString &comment, const QString &indent) const;
    void writeValue(ProValue *value, const QString &indent);
    void writeOther(ProItem *item, const QString &indent);
    void writeBlock(ProBlock *block, const QString &indent);
    void writeItem(ProItem *item, const QString &indent);

private:
    enum ProWriteState {
        NewLine     = 0x01,
        FirstItem   = 0x02,
        LastItem    = 0x04
    };

    QTextStream m_out;
    int m_writeState;
    QString m_comment;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROWRITER_H
