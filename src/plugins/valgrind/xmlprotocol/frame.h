/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
