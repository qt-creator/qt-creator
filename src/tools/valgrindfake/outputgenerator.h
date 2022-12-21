// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QTimer>
#include <QTextStream>


QT_BEGIN_NAMESPACE
class QAbstractSocket;
class QIODevice;
QT_END_NAMESPACE

namespace Valgrind {
namespace Fake {

class OutputGenerator : public QObject
{
    Q_OBJECT

public:
    explicit OutputGenerator(QAbstractSocket *output, QIODevice *input);

    void setCrashRandomly(bool enable);
    void setOutputGarbage(bool enable);
    void setWait(uint seconds);

Q_SIGNALS:
    void finished();

private:
    void produceRuntimeError();

    /// write output to the stream until the next error
    void writeOutput();

    QAbstractSocket *m_output;
    QIODevice *m_input;
    QTimer m_timer;
    bool m_finished;
    bool m_crash;
    bool m_garbage;
    uint m_wait;
};

} // namespace Fake
} // namespace Valgrind
