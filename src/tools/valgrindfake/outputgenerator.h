/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef LIBVALGRIND_FAKE_OUTPUTGENERATOR_H
#define LIBVALGRIND_FAKE_OUTPUTGENERATOR_H

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
private slots:
    /// write output to the stream until the next error
    void writeOutput();

private:
    void produceRuntimeError();

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

#endif // LIBVALGRIND_FAKE_OUTPUTGENERATOR_H
