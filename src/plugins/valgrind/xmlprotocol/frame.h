// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSharedDataPointer>

namespace Valgrind::XmlProtocol {

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

    QString toolTip() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace Valgrind::XmlProtocol
