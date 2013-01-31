/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

    QString file() const;
    void setFile(const QString &file);

    QString directory() const;
    void setDirectory(const QString &directory);

    int line() const;
    void setLine(int line);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Valgrind

#endif
