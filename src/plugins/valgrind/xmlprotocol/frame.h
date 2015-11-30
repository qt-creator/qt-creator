/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#ifndef LIBVALGRIND_PROTOCOL_FRAME_H
#define LIBVALGRIND_PROTOCOL_FRAME_H

#include <QSharedDataPointer>

namespace Valgrind {
namespace XmlProtocol {

class Frame
{
public:
    Frame();
    ~Frame();
    Frame(const Frame &other);

    Frame &operator=(const Frame &other);
    void swap(Frame &other);

    bool operator==(const Frame &other) const;
    bool operator!=(const Frame &other) const;

    quint64 instructionPointer() const;
    void setInstructionPointer(quint64);

    QString object() const;
    void setObject(const QString &obj);

    QString functionName() const;
    void setFunctionName(const QString &functionName);

    QString fileName() const;
    void setFileName(const QString &fileName);

    QString directory() const;
    void setDirectory(const QString &directory);

    QString filePath() const;

    int line() const;
    void setLine(int line);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Valgrind

#endif
