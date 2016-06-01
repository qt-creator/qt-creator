/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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
