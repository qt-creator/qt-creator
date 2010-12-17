/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "rvcttoolchain.h"
#include "rvctparser.h"

#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

static const char rvctBinaryC[] = "armcc";

static inline QStringList headerPathToStringList(const QList<ProjectExplorer::HeaderPath> &hl)
{
    QStringList rc;
    foreach (const ProjectExplorer::HeaderPath &hp, hl)
        rc.push_back(hp.path());
    return rc;
}

// ==========================================================================
// RVCTToolChain
// ==========================================================================

RVCTToolChain::RVCTToolChain(const S60Devices::Device &device, ProjectExplorer::ToolChainType type) :
    m_mixin(device),
    m_type(type),
    m_versionUpToDate(false),
    m_major(0),
    m_minor(0),
    m_build(0)
{
}

QSet<QPair<int, int> > RVCTToolChain::configuredRvctVersions()
{
    static QSet<QPair<int, int> > result;

    if (result.isEmpty()) {
        QRegExp regex(QLatin1String("^RVCT(\\d)(\\d)BIN=.*$"));
        Q_ASSERT(regex.isValid());
        QStringList environment = QProcessEnvironment::systemEnvironment().toStringList();
        foreach (const QString &v, environment) {
            if (regex.exactMatch(v)) {
                int major = regex.cap(1).toInt();
                int minor = regex.cap(2).toInt();
                result.insert(qMakePair(major, minor));
            }
        }
    }
    return result;
}

QStringList RVCTToolChain::configuredEnvironment()
{
    updateVersion();

    if (m_additionalEnvironment.isEmpty()) {
        const QString binVarName = QString::fromLocal8Bit(rvctBinEnvironmentVariable());
        const QString varName = binVarName.left(binVarName.count() - 3 /* BIN */);
        QStringList environment = QProcessEnvironment::systemEnvironment().toStringList();
        foreach (const QString &v, environment) {
            if ((v.startsWith(varName) && !v.startsWith(binVarName))
                 || v.startsWith(QLatin1String("ARMLMD_LICENSE_FILE="))) {
                m_additionalEnvironment.append(v);
            }
        }
    }
    return m_additionalEnvironment;
}

// Return the environment variable indicating the RVCT version
// 'RVCT<major><minor>BIN'
QByteArray RVCTToolChain::rvctBinEnvironmentVariableForVersion(int major)
{
    QSet<QPair<int, int> > versions = configuredRvctVersions();

    for (QSet<QPair<int, int> >::const_iterator it = versions.constBegin();
         it != versions.constEnd(); ++it) {
        if (it->first == major) {
            if (it->first < 0 || it->first > 9) continue;
            if (it->second < 0 || it->second > 9) continue;
            QByteArray result = "RVCT..BIN";
            result[4] = '0' + it->first;
            result[5] = '0' + it->second;
            return result;
        }
    }
    return QByteArray();
}

QString RVCTToolChain::rvctBinPath()
{
    if (m_binPath.isEmpty()) {
        const QByteArray binVar = rvctBinEnvironmentVariable();
        if (!binVar.isEmpty()) {
            const QByteArray binPathB = qgetenv(binVar);
            if (!binPathB.isEmpty()) {
                const QFileInfo fi(QString::fromLocal8Bit(binPathB));
                if (fi.isDir())
                    m_binPath = fi.absoluteFilePath();
            }
        }
    }
    return m_binPath;
}

// Return binary expanded by path or resort to PATH
QString RVCTToolChain::rvctBinary()
{
    QString executable = QLatin1String(rvctBinaryC);
#ifdef Q_OS_WIN
    executable += QLatin1String(".exe");
#endif
    const QString binPath = rvctBinPath();
    return binPath.isEmpty() ? executable : (binPath + QLatin1Char('/') + executable);
}

ProjectExplorer::ToolChainType RVCTToolChain::type() const
{
    return m_type;
}

void RVCTToolChain::updateVersion()
{
    if (m_versionUpToDate)
        return;

    m_versionUpToDate = true;
    m_major = 0;
    m_minor = 0;
    m_build = 0;
    QProcess armcc;
    Utils::Environment env = Utils::Environment::systemEnvironment();
    addToEnvironment(env);
    armcc.setEnvironment(env.toStringList());
    const QString binary = rvctBinary();
    armcc.start(binary, QStringList());
    if (!armcc.waitForStarted()) {
        qWarning("Unable to run rvct binary '%s' when trying to determine version.", qPrintable(binary));
        return;
    }
    armcc.closeWriteChannel();
    if (!armcc.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(armcc);
        qWarning("Timeout running rvct binary '%s' trying to determine version.", qPrintable(binary));
        return;
    }
    if (armcc.exitStatus() != QProcess::NormalExit) {
        qWarning("A crash occurred when running rvct binary '%s' trying to determine version.", qPrintable(binary));
        return;
    }
    QString versionLine = QString::fromLocal8Bit(armcc.readAllStandardOutput());
    versionLine += QString::fromLocal8Bit(armcc.readAllStandardError());
    const QRegExp versionRegExp(QLatin1String("RVCT(\\d*)\\.(\\d*).*\\[Build.(\\d*)\\]"),
                                Qt::CaseInsensitive);
    QTC_ASSERT(versionRegExp.isValid(), return);
    if (versionRegExp.indexIn(versionLine) != -1) {
        m_major = versionRegExp.cap(1).toInt();
        m_minor = versionRegExp.cap(2).toInt();
        m_build = versionRegExp.cap(3).toInt();
    }
}

QByteArray RVCTToolChain::predefinedMacros()
{
    // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0205f/Babbacdb.html (version 2.2)
    // and http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491b/BABJFEFG.html (version 4.0)
    updateVersion();
    QByteArray ba("#define __ARRAY_OPERATORS\n"
                  "#define _BOOL\n"
                  "#define __cplusplus\n"
                  "#define __CC_ARM 1\n"
                  "#define __EDG__\n"
                  "#define __STDC__\n"
                  "#define __STDC_VERSION__\n"
                  "#define __sizeof_int 4"
                  "#define __sizeof_long 4"
                  "#define __sizeof_ptr 4"
                  "#define __TARGET_FEATURE_DOUBLEWORD\n"
                  "#define __TARGET_FEATURE_DSPMUL\n"
                  "#define __TARGET_FEATURE_HALFWORD\n"
                  "#define __TARGET_FEATURE_THUMB\n"
                  "#define _WCHAR_T\n"
                  "#define __SYMBIAN32__\n");
    return ba;
}

QList<HeaderPath> RVCTToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        updateVersion();
        QString rvctInclude = qgetenv(QString::fromLatin1("RVCT%1%2INC").arg(m_major).arg(m_minor).toLatin1());
        if (!rvctInclude.isEmpty())
            m_systemHeaderPaths.append(HeaderPath(rvctInclude, HeaderPath::GlobalHeaderPath));
        switch (m_type) {
        case ProjectExplorer::ToolChain_RVCT_ARMV5_GNUPOC:
            m_systemHeaderPaths += m_mixin.gnuPocRvctHeaderPaths(m_major, m_minor);
            break;
        default:
            m_systemHeaderPaths += m_mixin.epocHeaderPaths();
            break;
        }
    }
    return m_systemHeaderPaths;
}

// Expand an RVCT variable, such as RVCT22BIN, by some new values
void RVCTToolChain::addToRVCTPathVariable(const QString &postfix, const QStringList &values,
                                          Utils::Environment &env) const
{
    // get old values
    const QChar separator = QLatin1Char(',');
    const QString variable = QString::fromLatin1("RVCT%1%2%3").arg(m_major).arg(m_minor).arg(postfix);
    const QString oldValueS = env.value(variable);
    const QStringList oldValue = oldValueS.isEmpty() ? QStringList() : oldValueS.split(separator);
    // merge new values
    QStringList newValue = oldValue;
    foreach(const QString &v, values) {
        const QString normalized = QDir::toNativeSeparators(v);
        if (!newValue.contains(normalized))
            newValue.push_back(normalized);
    }
    if (newValue != oldValue)
        env.set(variable, newValue.join(QString(separator)));
}

// Figure out lib path via
QStringList RVCTToolChain::libPaths()
{
    const QByteArray binLocation = qgetenv(rvctBinEnvironmentVariable());
    if (binLocation.isEmpty())
        return QStringList();
    const QString pathRoot = QFileInfo(QString::fromLocal8Bit(binLocation)).path();
    QStringList rc;
    rc.push_back(pathRoot + QLatin1String("/lib"));
    rc.push_back(pathRoot + QLatin1String("/lib/armlib"));
    return rc;
}

void RVCTToolChain::addToEnvironment(Utils::Environment &env)
{
    updateVersion();

    // Push additional configuration variables for the compiler through:
    QStringList additionalVariables = configuredEnvironment();
    foreach (const QString &var, additionalVariables) {
        int pos = var.indexOf(QLatin1Char('='));
        Q_ASSERT(pos >= 0);
        const QString key = var.left(pos);
        const QString value = var.mid(pos + 1);
        env.set(key, value);
    }

    switch (m_type) {
    case ProjectExplorer::ToolChain_RVCT_ARMV5_GNUPOC: {
        m_mixin.addGnuPocToEnvironment(&env);
        // setup RVCT22INC, LIB
        addToRVCTPathVariable(QLatin1String("INC"),
                      headerPathToStringList(m_mixin.gnuPocRvctHeaderPaths(m_major, m_minor)),
                      env);
        addToRVCTPathVariable(QLatin1String("LIB"),
                              libPaths() + m_mixin.gnuPocRvctLibPaths(5, true),
                              env);
        }
        break;
    default:
        m_mixin.addEpocToEnvironment(&env);
        break;
    }

    const QString binPath = rvctBinPath();
    env.set(rvctBinEnvironmentVariable(), QDir::toNativeSeparators(binPath));

    // Add rvct to path and set locale to 'C'
    if (!binPath.isEmpty())
        env.prependOrSetPath(binPath);
    env.set(QLatin1String("LANG"), QString(QLatin1Char('C')));

    env.set(QLatin1String("QT_RVCT_VERSION"), QString::fromLatin1("%1.%2").arg(m_major).arg(m_minor));
}

QString RVCTToolChain::makeCommand() const
{
#if defined(Q_OS_WIN)
    return QLatin1String("make.exe");
#else
    return QLatin1String("make");
#endif
}

ProjectExplorer::IOutputParser *RVCTToolChain::outputParser() const
{
    return new RvctParser;
}

// ==========================================================================
// RVCT2ToolChain
// ==========================================================================

RVCT2ToolChain::RVCT2ToolChain(const S60Devices::Device &device, ProjectExplorer::ToolChainType type) :
    RVCTToolChain(device, type)
{ }

QByteArray RVCT2ToolChain::rvctBinEnvironmentVariable()
{
    return rvctBinEnvironmentVariableForVersion(2);
}

QByteArray RVCT2ToolChain::predefinedMacros()
{
    QByteArray result = RVCTToolChain::predefinedMacros();
    result.append(QString::fromLatin1("#define __arm__arm__\n"
                                      "#define __ARMCC_VERSION %1%2%3%4\n"
                                      "#define c_plusplus\n"
                                      )
                  .arg(m_major, 1, 10, QLatin1Char('0'))
                  .arg(m_minor, 1, 10, QLatin1Char('0'))
                  .arg("0")
                  .arg(m_build, 3, 10, QLatin1Char('0')).toLatin1());
    return result;
}

bool RVCT2ToolChain::equals(const ToolChain *otherIn) const
{
    if (otherIn->type() != type())
        return false;
    const RVCT2ToolChain *other = static_cast<const RVCT2ToolChain *>(otherIn);
    return other->m_mixin == m_mixin;
}

// ==========================================================================
// RVCT4ToolChain
// ==========================================================================

RVCT4ToolChain::RVCT4ToolChain(const S60Devices::Device &device,
                               ProjectExplorer::ToolChainType type) :
    RVCT2ToolChain(device, type)
{ }

QByteArray RVCT4ToolChain::rvctBinEnvironmentVariable()
{
    return rvctBinEnvironmentVariableForVersion(4);
}

QByteArray RVCT4ToolChain::predefinedMacros()
{
    QByteArray result = RVCTToolChain::predefinedMacros();
    result.append(QString::fromLatin1("#define __arm__\n"
                                      "#define __ARMCC_VERSION %1%2%3\n")
                  .arg(m_major, 1, 10, QLatin1Char('0'))
                  .arg(m_minor, 1, 10, QLatin1Char('0'))
                  .arg(m_build, 3, 10, QLatin1Char('0')).toLatin1());
    return result;
}


bool RVCT4ToolChain::equals(const ToolChain *otherIn) const
{
    if (otherIn->type() != type())
        return false;
    const RVCT4ToolChain *other = static_cast<const RVCT4ToolChain *>(otherIn);
    return other->m_mixin == m_mixin;
}
