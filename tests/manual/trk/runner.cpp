/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

#include <QtGui/QApplication>

#include <unistd.h>

class Runner : public QObject
{
    Q_OBJECT

public:
    Runner();

private:
    void parseArguments();
    void launchAdapter();
    void launchTrkServer();
    void writeGdbInit();
    void run();

    QStringList m_adapterOptions;
    QStringList m_trkServerOptions;
    QString m_endianness;
    QString m_trkServerName;
    bool m_runTrkServer;
    bool m_isUnix;
    int m_waitAdapter;
    QString m_gdbServerIP;
    QString m_gdbServerPort;

    QProcess m_adapterProc;
    QProcess m_trkServerProc;
    QProcess m_debuggerProc;
};

Runner::Runner()
{
#ifdef Q_OS_UNIX
    m_isUnix = true;
#else
    m_isUnix = false;
#endif
    m_endianness = "little";
    m_runTrkServer = true;
    m_waitAdapter = 0;
}

static QString usage()
{
   return "Usage: run.pl -w -av -aq -au -tv -tq -l [COM]\n\n"
    "Options:\n"
    "     -av     Adapter verbose\n"
    "     -aq     Adapter quiet\n"
    "     -au     Adapter turn off buffered memory read\n"
    "     -af     Adapter turn off serial frame\n"
    "     -w      Wait for termination of Adapter (Bluetooth)\n"
    "     -tv     TrkServer verbose\n"
    "     -tq     TrkServer quiet\n"
    "\n"
    "     trkserver simulator will be run unless COM is specified\n"
    "\n"
    "Bluetooth:\n"
    "     rfcomm listen /dev/rfcomm0 1 $PWD/run.pl -av -af -w {}\n";
}

void Runner::parseArguments()
{
    QStringList args = qApp->arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);
        if (arg.startsWith('-')) {
            if (arg == "-av") {
                m_adapterOptions.append("-v");
            } else if (arg == "-aq") {
                m_adapterOptions.append("-q");
            } else if (arg == "-af") {
                m_adapterOptions.append("-f");
            } else if (arg == "-au") {
                m_adapterOptions.append("-u");
            } else if (arg == "-w") {
                m_waitAdapter = 1;
            } else if (arg == "-tv") {
                m_trkServerOptions.append("-v");
            } else if (arg == "-tq") {
                m_trkServerOptions.append("-q");
            } else if (arg == "-h") {
                qDebug() << usage();
                qApp->exit(0);
            } else {
                qDebug() << usage();
                qApp->exit(1);
            }
        } else {
            m_trkServerName = arg;
            m_runTrkServer = 0;
        }
    }
}

void Runner::launchTrkServer()
{
    const QString dumpPostfix = ".bin";

    QString trkServerName;
    if (m_isUnix)
        trkServerName = QDir::currentPath() + "trkserver";
    else
	trkServerName = "cmd.exe";

    QStringList trkServerArgs;
    if (!m_isUnix) {
	trkServerArgs << "/c" << "start"
            << QDir::currentPath() + "/debug/trkserver.exe";
    }

    trkServerArgs << m_trkServerOptions;
    trkServerArgs << m_trkServerName;
    trkServerArgs << "TrkDump-78-6a-40-00 " + dumpPostfix;
    trkServerArgs << "0x00402000" + dumpPostfix;
    trkServerArgs << "0x786a4000" + dumpPostfix;
    trkServerArgs << "0x00600000" + dumpPostfix;

    qDebug() << "### Executing: " + trkServerArgs.join(" ");
    m_trkServerProc.start(trkServerName, trkServerArgs);
    m_trkServerProc.waitForStarted();
}

void Runner::launchAdapter()
{
    QString adapterName;
    if (m_isUnix)
        adapterName = QDir::currentPath() + "adapter";
    else
	adapterName = "cmd.exe";

    QStringList adapterArgs;
    if (!m_isUnix) {
	adapterArgs << "/c" << "start"
            << QDir::currentPath() + "/debug/adapter.exe";
    }
    adapterArgs << m_adapterOptions;
    adapterArgs << "-s";
    adapterArgs << m_trkServerName << m_gdbServerIP + ':' + m_gdbServerPort;

    qDebug() << "### Executing: " + adapterArgs.join(" ");

    m_adapterProc.start(adapterName, adapterArgs);
    m_adapterProc.waitForStarted();
    //die ('Unable to launch adapter') if $adapterpid == -1;

/*
    if ($m_waitAdapter > 0) {
        print '### kill -USR1 ',$adapterpid,"\n";    
        waitpid($adapterpid, 0);
    }    
*/
}

void Runner::writeGdbInit()
{
    QString gdbInitFile = ".gdbinit";
    QFile file(gdbInitFile);
    if (file.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot open "<<  gdbInitFile;
        return;
    }
    QTextStream ts(&file);
    ts << "# This is generated. Changes will be lost.";
    ts << "#set remote noack-packet on";
    ts << "set confirm off";
    ts << "set endian " + m_endianness;
    ts << "#set debug remote 1";
    ts << "#target remote $m_gdbServerIP:$m_gdbServerPort";
    ts << "target extended-remote $m_gdbServerIP:$m_gdbServerPort";
    ts << "#file filebrowseapp.sym";
    ts << "add-symbol-file filebrowseapp.sym 0x786A4000";
    ts << "symbol-file filebrowseapp.sym";
    ts << "print E32Main";
    ts << "break E32Main";
    ts << "#continue";
    ts << "#info files";
    ts << "#file filebrowseapp.sym -readnow";
    ts << "#add-symbol-file filebrowseapp.sym 0x786A4000";
}

void Runner::run()
{
    launchAdapter();

    if (m_isUnix) {
        QProcess::execute("killall -s USR adapter trkserver");
        QProcess::execute("killall adapter trkserver");
    }

    uid_t userId = getuid();
    if (m_trkServerName.isEmpty())
        m_trkServerName = QString("TRKSERVER-%").arg(userId);

    m_gdbServerIP = "127.0.0.1";
    m_gdbServerPort = QString::number(2222 + userId);

    if (m_isUnix) {
        QProcess fuserProc;
        fuserProc.start("fuser -n tcp -k " + m_gdbServerPort);
        qDebug() << "fuser: " << fuserProc.readAllStandardOutput();
        qDebug() << "fuser: " << fuserProc.readAllStandardError();
    }

    // Who writes that?
    //my $tempFile = File::Spec->catfile(File::Temp::tempdir(),
    //m_trkServerName);
    //unlink ($tempFile) if  -f $tempFile;

    if (m_runTrkServer)
        launchTrkServer();
    
    writeGdbInit();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Runner runner;

    return app.exec();
}

#include "runner.moc"
