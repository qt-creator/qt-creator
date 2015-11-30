/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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
#include "xcodebuildparser.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace ProjectExplorer {

static const char failureRe[] = "\\*\\* BUILD FAILED \\*\\*$";
static const char successRe[] = "\\*\\* BUILD SUCCEEDED \\*\\*$";
static const char buildRe[] = "=== BUILD (AGGREGATE )?TARGET (.*) OF PROJECT (.*) WITH .* ===$";
static const char signatureChangeRe[] = "(.+): replacing existing signature$";

XcodebuildParser::XcodebuildParser() :
    m_fatalErrorCount(0),
    m_xcodeBuildParserState(OutsideXcodebuild)
{
    setObjectName(QLatin1String("XcodeParser"));
    m_failureRe.setPattern(QLatin1String(failureRe));
    QTC_CHECK(m_failureRe.isValid());
    m_successRe.setPattern(QLatin1String(successRe));
    QTC_CHECK(m_successRe.isValid());
    m_buildRe.setPattern(QLatin1String(buildRe));
    QTC_CHECK(m_buildRe.isValid());
    m_replacingSignatureRe.setPattern(QLatin1String(signatureChangeRe));
    QTC_CHECK(m_replacingSignatureRe.isValid());
}

bool XcodebuildParser::hasFatalErrors() const
{
    return (m_fatalErrorCount > 0) || IOutputParser::hasFatalErrors();
}

void XcodebuildParser::stdOutput(const QString &line)
{
    const QString lne = rightTrimmed(line);
    if (m_buildRe.indexIn(lne) > -1) {
        m_xcodeBuildParserState = InXcodebuild;
        m_lastTarget = m_buildRe.cap(2);
        m_lastProject = m_buildRe.cap(3);
        return;
    }
    if (m_xcodeBuildParserState == InXcodebuild || m_xcodeBuildParserState == UnknownXcodebuildState) {
        if (m_successRe.indexIn(lne) > -1) {
            m_xcodeBuildParserState = OutsideXcodebuild;
            return;
        }
        if (m_replacingSignatureRe.indexIn(lne) > -1) {
            Task task(Task::Warning,
                      QCoreApplication::translate("ProjectExplorer::XcodebuildParser",
                                                  "Replacing signature"),
                      Utils::FileName::fromString(m_replacingSignatureRe.cap(1)), /* filename */
                      -1, /* line */
                      Constants::TASK_CATEGORY_COMPILE);
            taskAdded(task, 1);
            return;
        }
        IOutputParser::stdError(line);
    } else {
        IOutputParser::stdOutput(line);
    }
}

void XcodebuildParser::stdError(const QString &line)
{
    const QString lne = rightTrimmed(line);
    if (m_failureRe.indexIn(lne) > -1) {
        ++m_fatalErrorCount;
        m_xcodeBuildParserState = UnknownXcodebuildState;
        // unfortunately the m_lastTarget, m_lastProject might not be in sync
        Task task(Task::Error,
                  QCoreApplication::translate("ProjectExplorer::XcodebuildParser",
                                              "Xcodebuild failed."),
                  Utils::FileName(), /* filename */
                  -1, /* line */
                  Constants::TASK_CATEGORY_COMPILE);
        taskAdded(task);
        return;
    }
    if (m_xcodeBuildParserState == OutsideXcodebuild) { // also forward if UnknownXcodebuildState ?
        IOutputParser::stdError(line);
        return;
    }
}

} // namespace ProjectExplorer

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "outputparser_test.h"
#   include "projectexplorer.h"

#   include "metatypedeclarations.h"

using namespace ProjectExplorer;

Q_DECLARE_METATYPE(ProjectExplorer::XcodebuildParser::XcodebuildStatus)

XcodebuildParserTester::XcodebuildParserTester(XcodebuildParser *p, QObject *parent) :
    QObject(parent),
    parser(p)
{ }

void XcodebuildParserTester::onAboutToDeleteParser()
{
    QCOMPARE(parser->m_xcodeBuildParserState, expectedFinalState);
}

void ProjectExplorerPlugin::testXcodebuildParserParsing_data()
{
    QTest::addColumn<ProjectExplorer::XcodebuildParser::XcodebuildStatus>("initialStatus");
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");
    QTest::addColumn<ProjectExplorer::XcodebuildParser::XcodebuildStatus>("finalStatus");

    QTest::newRow("outside pass-through stdout")
            << XcodebuildParser::OutsideXcodebuild
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString()
            << XcodebuildParser::OutsideXcodebuild;
    QTest::newRow("outside pass-through stderr")
            << XcodebuildParser::OutsideXcodebuild
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString()
            << XcodebuildParser::OutsideXcodebuild;
    QTest::newRow("inside pass stdout to stderr")
            << XcodebuildParser::InXcodebuild
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString()
            << XcodebuildParser::InXcodebuild;
    QTest::newRow("inside ignore stderr")
            << XcodebuildParser::InXcodebuild
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString()
            << QList<Task>()
            << QString()
            << XcodebuildParser::InXcodebuild;
    QTest::newRow("unknown pass stdout to stderr")
            << XcodebuildParser::UnknownXcodebuildState
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString()
            << XcodebuildParser::UnknownXcodebuildState;
    QTest::newRow("unknown ignore stderr (change?)")
            << XcodebuildParser::UnknownXcodebuildState
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString()
            << QList<Task>()
            << QString()
            << XcodebuildParser::UnknownXcodebuildState;
    QTest::newRow("switch outside->in->outside")
            << XcodebuildParser::OutsideXcodebuild
            << QString::fromLatin1("outside\n"
                                   "=== BUILD AGGREGATE TARGET Qt Preprocess OF PROJECT testQQ WITH THE DEFAULT CONFIGURATION (Debug) ===\n"
                                   "in xcodebuild\n"
                                   "=== BUILD TARGET testQQ OF PROJECT testQQ WITH THE DEFAULT CONFIGURATION (Debug) ===\n"
                                   "in xcodebuild2\n"
                                   "** BUILD SUCCEEDED **\n"
                                   "outside2")
            << OutputParserTester::STDOUT
            << QString::fromLatin1("outside\noutside2\n") << QString::fromLatin1("in xcodebuild\nin xcodebuild2\n")
            << QList<Task>()
            << QString()
            << XcodebuildParser::OutsideXcodebuild;
    QTest::newRow("switch Unknown->in->outside")
            << XcodebuildParser::UnknownXcodebuildState
            << QString::fromLatin1("unknown\n"
                                   "=== BUILD TARGET testQQ OF PROJECT testQQ WITH THE DEFAULT CONFIGURATION (Debug) ===\n"
                                   "in xcodebuild\n"
                                   "** BUILD SUCCEEDED **\n"
                                   "outside")
            << OutputParserTester::STDOUT
            << QString::fromLatin1("outside\n") << QString::fromLatin1("unknown\nin xcodebuild\n")
            << QList<Task>()
            << QString()
            << XcodebuildParser::OutsideXcodebuild;
    QTest::newRow("switch in->unknown")
            << XcodebuildParser::InXcodebuild
            << QString::fromLatin1("insideErr\n"
                                   "** BUILD FAILED **\n"
                                   "unknownErr")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(
                    Task::Error,
                    QCoreApplication::translate("ProjectExplorer::XcodebuildParser",
                                                "Xcodebuild failed."),
                    Utils::FileName(), /* filename */
                    -1, /* line */
                    Constants::TASK_CATEGORY_COMPILE))
            << QString()
            << XcodebuildParser::UnknownXcodebuildState;
    QTest::newRow("switch out->unknown")
            << XcodebuildParser::OutsideXcodebuild
            << QString::fromLatin1("outErr\n"
                                   "** BUILD FAILED **\n"
                                   "unknownErr")
            << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("outErr\n")
            << (QList<Task>()
                << Task(
                    Task::Error,
                    QCoreApplication::translate("ProjectExplorer::XcodebuildParser",
                                                "Xcodebuild failed."),
                    Utils::FileName(), /* filename */
                    -1, /* line */
                    Constants::TASK_CATEGORY_COMPILE))
            << QString()
            << XcodebuildParser::UnknownXcodebuildState;
    QTest::newRow("inside catch codesign replace signature")
            << XcodebuildParser::InXcodebuild
            << QString::fromLatin1("/somepath/somefile.app: replacing existing signature") << OutputParserTester::STDOUT
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Warning,
                        QCoreApplication::translate("ProjectExplorer::XcodebuildParser",
                                                    "Replacing signature"),
                        Utils::FileName::fromString(QLatin1String("/somepath/somefile.app")), /* filename */
                        -1, /* line */
                        Constants::TASK_CATEGORY_COMPILE))
            << QString()
            << XcodebuildParser::InXcodebuild;
    QTest::newRow("outside forward codesign replace signature")
            << XcodebuildParser::OutsideXcodebuild
            << QString::fromLatin1("/somepath/somefile.app: replacing existing signature") << OutputParserTester::STDOUT
            << QString::fromLatin1("/somepath/somefile.app: replacing existing signature\n") << QString()
            << QList<Task>()
            << QString()
            << XcodebuildParser::OutsideXcodebuild;
}

void ProjectExplorerPlugin::testXcodebuildParserParsing()
{
    OutputParserTester testbench;
    XcodebuildParser *childParser = new XcodebuildParser;
    XcodebuildParserTester *tester = new XcodebuildParserTester(childParser);

    connect(&testbench, SIGNAL(aboutToDeleteParser()), tester, SLOT(onAboutToDeleteParser()));

    testbench.appendOutputParser(childParser);
    QFETCH(ProjectExplorer::XcodebuildParser::XcodebuildStatus, initialStatus);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, outputLines);
    QFETCH(ProjectExplorer::XcodebuildParser::XcodebuildStatus, finalStatus);

    tester->expectedFinalState = finalStatus;
    childParser->m_xcodeBuildParserState = initialStatus;
    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);

    delete tester;
}

#endif

