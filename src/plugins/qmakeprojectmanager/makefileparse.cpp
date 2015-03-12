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

#include "makefileparse.h"

#include <qtsupport/qtversionmanager.h>
#include <qtsupport/baseqtversion.h>
#include <utils/qtcprocess.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QLoggingCategory>

using namespace QmakeProjectManager;
using namespace Internal;

using Utils::FileName;
using Utils::QtcProcess;
using QtSupport::QtVersionManager;
using QtSupport::BaseQtVersion;

static QString findQMakeLine(const QString &makefile, const QString &key)
{
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            if (line.startsWith(key))
                return line;
        }
    }
    return QString();
}

/// This function trims the "#Command /path/to/qmake" from the line
static QString trimLine(const QString &line)
{

    // Actually the first space after #Command: /path/to/qmake
    const int firstSpace = line.indexOf(QLatin1Char(' '), 11);
    return line.mid(firstSpace).trimmed();
}

void MakeFileParse::parseArgs(const QString &args, QList<QMakeAssignment> *assignments, QList<QMakeAssignment> *afterAssignments)
{
    QRegExp regExp(QLatin1String("([^\\s\\+-]*)\\s*(\\+=|=|-=|~=)(.*)"));
    bool after = false;
    bool ignoreNext = false;
    m_unparsedArguments = args;
    QtcProcess::ArgIterator ait(&m_unparsedArguments);
    while (ait.next()) {
        if (ignoreNext) {
            // Ignoring
            ignoreNext = false;
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-after")) {
            after = true;
            ait.deleteArg();
        } else if (ait.value().contains(QLatin1Char('='))) {
            if (regExp.exactMatch(ait.value())) {
                QMakeAssignment qa;
                qa.variable = regExp.cap(1);
                qa.op = regExp.cap(2);
                qa.value = regExp.cap(3).trimmed();
                if (after)
                    afterAssignments->append(qa);
                else
                    assignments->append(qa);
            } else {
                qDebug()<<"regexp did not match";
            }
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-o")) {
            ignoreNext = true;
            ait.deleteArg();
#if defined(Q_OS_WIN32)
        } else if (ait.value() == QLatin1String("-win32")) {
#elif defined(Q_OS_MAC)
        } else if (ait.value() == QLatin1String("-macx")) {
#elif defined(Q_OS_QNX6)
        } else if (ait.value() == QLatin1String("-qnx6")) {
#else
        } else if (ait.value() == QLatin1String("-unix")) {
#endif
            ait.deleteArg();
        }
    }
    ait.deleteArg();  // The .pro file is always the last arg
}

void dumpQMakeAssignments(const QList<QMakeAssignment> &list)
{
    foreach (const QMakeAssignment &qa, list) {
        qCDebug(MakeFileParse::logging()) << "    " << qa.variable << qa.op << qa.value;
    }
}

void MakeFileParse::parseAssignments(QList<QMakeAssignment> *assignments)
{
    bool foundSeparateDebugInfo = false;
    bool foundForceDebugInfo = false;
    QList<QMakeAssignment> oldAssignments = *assignments;
    assignments->clear();
    foreach (const QMakeAssignment &qa, oldAssignments) {
        if (qa.variable == QLatin1String("CONFIG")) {
            QStringList values = qa.value.split(QLatin1Char(' '));
            QStringList newValues;
            foreach (const QString &value, values) {
                if (value == QLatin1String("debug")) {
                    if (qa.op == QLatin1String("+=")) {
                        m_qmakeBuildConfig.explicitDebug = true;
                        m_qmakeBuildConfig.explicitRelease = false;
                    } else {
                        m_qmakeBuildConfig.explicitDebug = false;
                        m_qmakeBuildConfig.explicitRelease = true;
                    }
                } else if (value == QLatin1String("release")) {
                    if (qa.op == QLatin1String("+=")) {
                        m_qmakeBuildConfig.explicitDebug = false;
                        m_qmakeBuildConfig.explicitRelease = true;
                    } else {
                        m_qmakeBuildConfig.explicitDebug = true;
                        m_qmakeBuildConfig.explicitRelease = false;
                    }
                } else if (value == QLatin1String("debug_and_release")) {
                    if (qa.op == QLatin1String("+=")) {
                        m_qmakeBuildConfig.explicitBuildAll = true;
                        m_qmakeBuildConfig.explicitNoBuildAll = false;
                    } else {
                        m_qmakeBuildConfig.explicitBuildAll = false;
                        m_qmakeBuildConfig.explicitNoBuildAll = true;
                    }
                } else if (value == QLatin1String("x86")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::X86;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("x86_64")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::X86_64;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("ppc")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::PPC;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("ppc64")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::PPC64;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("iphonesimulator")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.osType = QMakeStepConfig::IphoneSimulator;
                    else
                        m_config.osType = QMakeStepConfig::NoOsType;
                } else if (value == QLatin1String("iphoneos")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.osType = QMakeStepConfig::IphoneOS;
                    else
                        m_config.osType = QMakeStepConfig::NoOsType;
                } else if (value == QLatin1String("declarative_debug")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.linkQmlDebuggingQQ1 = true;
                    else
                        m_config.linkQmlDebuggingQQ1 = false;
                } else if (value == QLatin1String("qml_debug")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.linkQmlDebuggingQQ2 = true;
                    else
                        m_config.linkQmlDebuggingQQ2 = false;
                } else if (value == QLatin1String("qtquickcompiler")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.useQtQuickCompiler = true;
                    else
                        m_config.useQtQuickCompiler = false;
                } else if (value == QLatin1String("force_debug_info")) {
                    if (qa.op == QLatin1String("+="))
                        foundForceDebugInfo = true;
                    else
                        foundForceDebugInfo = false;
                } else if (value == QLatin1String("separate_debug_info")) {
                    if (qa.op == QLatin1String("+="))
                        foundSeparateDebugInfo = true;
                    else
                        foundSeparateDebugInfo = false;
                } else {
                    newValues.append(value);
                }
                QMakeAssignment newQA = qa;
                newQA.value = newValues.join(QLatin1Char(' '));
                if (!newValues.isEmpty())
                    assignments->append(newQA);
            }
        } else {
            assignments->append(qa);
        }
    }

    if (foundForceDebugInfo && foundSeparateDebugInfo) {
        m_config.separateDebugInfo = true;
    } else if (foundForceDebugInfo) {
        // Found only force_debug_info, so readd it
        QMakeAssignment newQA;
        newQA.variable = QLatin1String("CONFIG");
        newQA.op = QLatin1String("+=");
        newQA.value = QLatin1String("force_debug_info");
        assignments->append(newQA);
    } else if (foundSeparateDebugInfo) {
        // Found only separate_debug_info, so readd it
        QMakeAssignment newQA;
        newQA.variable = QLatin1String("CONFIG");
        newQA.op = QLatin1String("+=");
        newQA.value = QLatin1String("separate_debug_info");
        assignments->append(newQA);
    }
}

static FileName findQMakeBinaryFromMakefile(const QString &makefile)
{
    bool debugAdding = false;
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        QRegExp r1(QLatin1String("QMAKE\\s*=(.*)"));
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (r1.exactMatch(line)) {
                if (debugAdding)
                    qDebug()<<"#~~ QMAKE is:"<<r1.cap(1).trimmed();
                QFileInfo qmake(r1.cap(1).trimmed());
                QString qmakePath = qmake.filePath();
                if (!QString::fromLatin1(QTC_HOST_EXE_SUFFIX).isEmpty()
                        && !qmakePath.endsWith(QLatin1String(QTC_HOST_EXE_SUFFIX))) {
                    qmakePath.append(QLatin1String(QTC_HOST_EXE_SUFFIX));
                }
                // Is qmake still installed?
                QFileInfo fi(qmakePath);
                if (fi.exists())
                    return FileName(fi);
            }
        }
    }
    return FileName();
}

MakeFileParse::MakeFileParse(const QString &makefile)
{
    qCDebug(logging()) << "Parsing makefile" << makefile;
    if (!QFileInfo::exists(makefile)) {
        qCDebug(logging()) << "**doesn't exist";
        m_state = MakefileMissing;
        return;
    }

    // Qt Version!
    m_qmakePath = findQMakeBinaryFromMakefile(makefile);
    qCDebug(logging()) << "  qmake:" << m_qmakePath;

    QString line = findQMakeLine(makefile, QLatin1String("# Project:")).trimmed();
    if (line.isEmpty()) {
        m_state = CouldNotParse;
        qCDebug(logging()) << "**No Project line";
        return;
    }

    line.remove(0, line.indexOf(QLatin1Char(':')) + 1);
    line = line.trimmed();

    // Src Pro file
    m_srcProFile = QDir::cleanPath(QFileInfo(makefile).absoluteDir().filePath(line));
    qCDebug(logging()) << "  source .pro file:" << m_srcProFile;

    line = findQMakeLine(makefile, QLatin1String("# Command:"));
    if (line.trimmed().isEmpty()) {
        m_state = CouldNotParse;
        qCDebug(logging()) << "**No Command line found";
        return;
    }

    line = trimLine(line);

    QList<QMakeAssignment> assignments;
    QList<QMakeAssignment> afterAssignments;
    // Split up args into assignments and other arguments, writes m_unparsedArguments
    parseArgs(line, &assignments, &afterAssignments);
    qCDebug(logging()) << "  Initial assignments:";
    dumpQMakeAssignments(assignments);

    // Filter out CONFIG arguments we know into m_qmakeBuildConfig and m_config
    parseAssignments(&assignments);
    qCDebug(logging()) << "  After parsing";
    dumpQMakeAssignments(assignments);

    qCDebug(logging()) << "  Explicit Debug" << m_qmakeBuildConfig.explicitDebug;
    qCDebug(logging()) << "  Explicit Release" << m_qmakeBuildConfig.explicitRelease;
    qCDebug(logging()) << "  Explicit BuildAll" << m_qmakeBuildConfig.explicitBuildAll;
    qCDebug(logging()) << "  Explicit NoBuildAll" << m_qmakeBuildConfig.explicitNoBuildAll;
    qCDebug(logging()) << "  TargetArch" << m_config.archConfig;
    qCDebug(logging()) << "  OsType" << m_config.osType;
    qCDebug(logging()) << "  LinkQmlDebuggingQQ1" << m_config.linkQmlDebuggingQQ1;
    qCDebug(logging()) << "  LinkQmlDebuggingQQ2" << m_config.linkQmlDebuggingQQ2;
    qCDebug(logging()) << "  Qt Quick Compiler" << m_config.useQtQuickCompiler;
    qCDebug(logging()) << "  Separate Debug Info" << m_config.separateDebugInfo;


    // Create command line of all unfiltered arguments
    foreach (const QMakeAssignment &qa, assignments)
        QtcProcess::addArg(&m_unparsedArguments, qa.variable + qa.op + qa.value);
    if (!afterAssignments.isEmpty()) {
        QtcProcess::addArg(&m_unparsedArguments, QLatin1String("-after"));
        foreach (const QMakeAssignment &qa, afterAssignments)
            QtcProcess::addArg(&m_unparsedArguments, qa.variable + qa.op + qa.value);
    }
    m_state = Okay;
}

MakeFileParse::MakefileState MakeFileParse::makeFileState() const
{
    return m_state;
}

Utils::FileName MakeFileParse::qmakePath() const
{
    return m_qmakePath;
}

QString MakeFileParse::srcProFile() const
{
    return m_srcProFile;
}

QMakeStepConfig MakeFileParse::config() const
{
    return m_config;
}


QString MakeFileParse::unparsedArguments() const
{
    return m_unparsedArguments;
}

BaseQtVersion::QmakeBuildConfigs MakeFileParse::effectiveBuildConfig(BaseQtVersion::QmakeBuildConfigs defaultBuildConfig) const
{
    BaseQtVersion::QmakeBuildConfigs buildConfig = defaultBuildConfig;
    if (m_qmakeBuildConfig.explicitDebug)
        buildConfig = buildConfig | BaseQtVersion::DebugBuild;
    else if (m_qmakeBuildConfig.explicitRelease)
        buildConfig = buildConfig & ~BaseQtVersion::DebugBuild;
    if (m_qmakeBuildConfig.explicitBuildAll)
        buildConfig = buildConfig | BaseQtVersion::BuildAll;
    else if (m_qmakeBuildConfig.explicitNoBuildAll)
        buildConfig = buildConfig &~ BaseQtVersion::BuildAll;
    return buildConfig;
}

const QLoggingCategory &MakeFileParse::logging()
{
    static const QLoggingCategory category("qtc.qmakeprojectmanager.import");
    return category;
}

