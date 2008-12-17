/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
