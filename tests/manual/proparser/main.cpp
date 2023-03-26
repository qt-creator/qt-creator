// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakeglobals.h"
#include "qmakevfs.h"
#include "qmakeparser.h"
#include "qmakeevaluator.h"
#include "profileevaluator.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QString>
#include <QStringList>
#include <QTextCodec>

static void print(const QString &fileName, int lineNo, int type, const QString &msg)
{
    QString pfx = ((type & QMakeHandler::CategoryMask) == QMakeHandler::WarningMessage)
                  ? QString::fromLatin1("WARNING: ") : QString();
    if (lineNo > 0)
        qWarning("%s%s:%d: %s", qPrintable(pfx), qPrintable(fileName), lineNo, qPrintable(msg));
    else if (lineNo)
        qWarning("%s%s: %s", qPrintable(pfx), qPrintable(fileName), qPrintable(msg));
    else
        qWarning("%s%s", qPrintable(pfx), qPrintable(msg));
}

class EvalHandler : public QMakeHandler {
public:
    virtual void message(int type, const QString &msg, const QString &fileName, int lineNo)
        { print(fileName, lineNo, type, msg); }

    virtual void fileMessage(int /*type*/, const QString &msg)
        { qWarning("%s", qPrintable(msg)); }

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}
};

static EvalHandler evalHandler;

static int evaluate(const QString &fileName, const QString &in_pwd, const QString &out_pwd,
                    bool cumulative, QMakeGlobals *option, QMakeParser *parser, QMakeVfs *vfs,
                    int level)
{
    static QSet<QString> visited;
    if (visited.contains(fileName))
        return 0;
    visited.insert(fileName);

    ProFileEvaluator visitor(option, parser, vfs, &evalHandler);
#ifdef PROEVALUATOR_CUMULATIVE
    visitor.setCumulative(cumulative);
#endif
    visitor.setOutputDir(out_pwd);

    ProFile *pro = parser->parsedProFile(option->device_root, fileName);
    if (!pro) {
        if (!QFileInfo::exists(option->device_root + fileName)) {
            qCritical("Input file %s does not exist.", qPrintable(fileName));
            return 3;
        }
        return 2;
    }
    if (!visitor.accept(pro)) {
        pro->deref();
        return 2;
    }

    if (visitor.templateType() == ProFileEvaluator::TT_Subdirs) {
        QStringList subdirs = visitor.values(QLatin1String("SUBDIRS"));
        subdirs.removeDuplicates();
        for (const QString &subDirVar : std::as_const(subdirs)) {
            QString realDir;
            const QString subDirKey = subDirVar + QLatin1String(".subdir");
            const QString subDirFileKey = subDirVar + QLatin1String(".file");
            if (visitor.contains(subDirKey))
                realDir = QFileInfo(visitor.value(subDirKey)).filePath();
            else if (visitor.contains(subDirFileKey))
                realDir = QFileInfo(visitor.value(subDirFileKey)).filePath();
            else
                realDir = subDirVar;
            QFileInfo info(realDir);
            if (!info.isAbsolute())
                info.setFile(in_pwd + QLatin1Char('/') + realDir);
            if (info.isDir())
                info.setFile(QString::fromLatin1("%1/%2.pro").arg(info.filePath(), info.fileName()));
            if (!info.exists()) {
                qDebug() << "Could not find sub dir" << info.filePath();
                continue;
            }

            QString inFile = QDir::cleanPath(info.absoluteFilePath()),
                    inPwd = QDir::cleanPath(info.path()),
                    outPwd = QDir::cleanPath(QDir(out_pwd).absoluteFilePath(
                            QDir(in_pwd).relativeFilePath(info.path())));
            int nlevel = level;
            if (nlevel >= 0) {
                printf("%sReading %s%s\n", QByteArray().fill(' ', nlevel).constData(),
                       qPrintable(inFile), (inPwd == outPwd) ? "" :
                       qPrintable(QString(QLatin1String(" [") + outPwd + QLatin1Char(']'))));
                fflush(stdout);
                nlevel++;
            }
            evaluate(inFile, inPwd, outPwd, cumulative, option, parser, vfs, nlevel);
        }
    }

    pro->deref();
    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QMakeGlobals option;
    QString qmake = QString::fromLocal8Bit(qgetenv("TESTREADER_QMAKE"));
    if (qmake.isEmpty())
        qmake = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmake");
    option.qmake_abslocation = QDir::cleanPath(qmake);
    option.initProperties();

    QStringList args = app.arguments();
    args.removeFirst();
    int level = -1; // verbose
    bool cumulative = false;
    QString file;
    QString in_pwd;
    QString out_pwd;
    QMakeCmdLineParserState state(QDir::currentPath());
    for (int pos = 0; ; ) {
        QMakeGlobals::ArgumentReturn cmdRet = option.addCommandLineArguments(state, args, &pos);
        if (cmdRet == QMakeGlobals::ArgumentsOk)
            break;
        if (cmdRet == QMakeGlobals::ArgumentMalformed) {
            qCritical("argument %s needs a parameter", qPrintable(args.at(pos - 1)));
            return 3;
        }
        Q_ASSERT(cmdRet == QMakeGlobals::ArgumentUnknown);
        QString arg = args.at(pos++);
        if (arg == QLatin1String("-v")) {
            level = 0;
        } else if (arg == QLatin1String("-d")) {
            option.debugLevel++;
        } else if (arg == QLatin1String("-c")) {
            cumulative = true;
        } else if (arg.startsWith(QLatin1Char('-'))) {
            qCritical("unrecognized option %s", qPrintable(arg));
            return 3;
        } else if (file.isEmpty()) {
            QFileInfo infi(arg);
            file = QDir::cleanPath(infi.absoluteFilePath());
            in_pwd = QDir::cleanPath(infi.absolutePath());
        } else if (out_pwd.isEmpty()) {
            out_pwd = QDir::cleanPath(QFileInfo(arg).absoluteFilePath());
        } else {
            qCritical("excess argument '%s'", qPrintable(arg));
            return 3;
        }
    }
    if (file.isEmpty()) {
        qCritical("usage: testreader [-v] [-d [-d]] [-c] <filenme> [<out_pwd>] [<variable assignments>]");
        return 3;
    }
    option.commitCommandLineArguments(state);
    option.useEnvironment();
    if (out_pwd.isEmpty())
        out_pwd = in_pwd;
    option.setDirectories(in_pwd, out_pwd, {});

    QMakeVfs vfs;
    QMakeParser parser(0, &vfs, &evalHandler);
    return evaluate(file, in_pwd, out_pwd, cumulative, &option, &parser, &vfs, level);
}
