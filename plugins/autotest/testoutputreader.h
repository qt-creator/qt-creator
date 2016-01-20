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

#include <QFutureInterface>
#include <QObject>
#include <QString>
#include <QXmlStreamReader>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

class TestOutputReader : public QObject
{
    Q_OBJECT
public:
    TestOutputReader(QFutureInterface<TestResult *> futureInterface,
                     QProcess *testApplication);

protected:
    virtual void processOutput() = 0;
    QFutureInterface<TestResult *> m_futureInterface;
    QProcess *m_testApplication;  // not owned
};

class QtTestOutputReader : public TestOutputReader
{
public:
    QtTestOutputReader(QFutureInterface<TestResult *> futureInterface,
                       QProcess *testApplication);

protected:
    void processOutput() override;

private:
    enum CDATAMode
    {
        None,
        DataTag,
        Description,
        QtVersion,
        QtBuild,
        QTestVersion
    };

    CDATAMode m_cdataMode = None;
    QString m_className;
    QString m_testCase;
    QString m_dataTag;
    Result::Type m_result = Result::Invalid;
    QString m_description;
    QString m_file;
    int m_lineNumber = 0;
    QString m_duration;
    QXmlStreamReader m_xmlReader;
};

class GTestOutputReader : public TestOutputReader
{
public:
    GTestOutputReader(QFutureInterface<TestResult *> futureInterface,
                      QProcess *testApplication);

protected:
    void processOutput() override;

private:
    QString m_currentTestName;
    QString m_currentTestSet;
    QString m_description;
    QByteArray m_unprocessed;
};


} // namespace Internal
} // namespace Autotest

#endif // TESTOUTPUTREADER_H
