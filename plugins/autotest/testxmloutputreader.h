/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTXMLOUTPUTREADER_H
#define TESTXMLOUTPUTREADER_H

#include "testresult.h"

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
class QIODevice;
class QProcess;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

class TestXmlOutputReader : public QObject
{
    Q_OBJECT

public:
    TestXmlOutputReader(QProcess *testApplication);
    ~TestXmlOutputReader();

public slots:
    void processOutput();
signals:
    void testResultCreated(const TestResult &testResult);
    void increaseProgress();
private:
    QProcess *m_testApplication;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTXMLOUTPUTREADER_H
