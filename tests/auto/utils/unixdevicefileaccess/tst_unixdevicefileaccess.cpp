// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <QRandomGenerator>
#include <QtCore/qiodevice.h>
#include <QtTest>

#include <utils/commandline.h>
#include <utils/devicefileaccess.h>
#include <utils/hostosinfo.h>
#include <utils/link.h>

//TESTED_COMPONENT=src/libs/utils

QT_BEGIN_NAMESPACE
namespace QTest {
template<>
char *toString(const Utils::FilePath &filePath)
{
    return qstrdup(filePath.toString().toLocal8Bit().constData());
}
} // namespace QTest
QT_END_NAMESPACE

using namespace Utils;

class TestDFA : public UnixDeviceFileAccess
{
public:
    using UnixDeviceFileAccess::UnixDeviceFileAccess;

    virtual RunResult runInShell(const CommandLine &cmdLine,
                                 const QByteArray &inputData = {}) const override
    {
        // Note: Don't convert into Utils::Process. See more comments in this change in gerrit.
        QProcess p;
        p.setProgram(cmdLine.executable().toString());
        p.setArguments(cmdLine.splitArguments());
        p.setProcessChannelMode(QProcess::SeparateChannels);

        p.start();
        p.waitForStarted();
        if (inputData.size() > 0) {
            p.write(inputData);
            p.closeWriteChannel();
        }
        p.waitForFinished();
        return {p.exitCode(), p.readAllStandardOutput(), p.readAllStandardError()};
    }

    void findUsingLs(const QString &current, const FileFilter &filter, QStringList *found)
    {
        UnixDeviceFileAccess::findUsingLs(current, filter, found, {});
    }
};

class tst_unixdevicefileaccess : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("This test is only for Unix hosts");

        m_fileSizeTestFile.writeFileContents(QByteArray(1024, 'a'));
    }

    void fileSize()
    {
        const auto size = m_dfaPtr->fileSize(m_fileSizeTestFile);
        QCOMPARE(size, 1024);
    }

    void findUsingLs()
    {
        QStringList result;
        m_dfa.findUsingLs(m_tempDir.path(),
                          {{}, QDir::NoFilter, QDirIterator::Subdirectories},
                          &result);

        QCOMPARE(result, QStringList({".", "..", "size-test"}));

        QDir tDir(m_tempDir.path());
        tDir.mkdir("lsfindsubdir");

        result.clear();
        m_dfa.findUsingLs(m_tempDir.path(),
                          {{}, QDir::NoFilter, QDirIterator::Subdirectories},
                          &result);
        QCOMPARE(result,
                 QStringList(
                     {".", "..", "lsfindsubdir/.", "lsfindsubdir/..", "lsfindsubdir", "size-test"}));
    }

private:
    TestDFA m_dfa;
    DeviceFileAccess *m_dfaPtr = &m_dfa;

    QTemporaryDir m_tempDir;
    FilePath m_fileSizeTestFile = FilePath::fromString(m_tempDir.filePath("size-test"));
};

QTEST_GUILESS_MAIN(tst_unixdevicefileaccess)

#include "tst_unixdevicefileaccess.moc"
