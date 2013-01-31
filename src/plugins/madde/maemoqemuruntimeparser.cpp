/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "maemoqemuruntimeparser.h"

#include "maemoglobal.h"
#include "maemoqemusettings.h"

#include <qtsupport/baseqtversion.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QProcess>
#include <QStringList>
#include <QTextStream>

namespace Madde {
namespace Internal {

class MaemoQemuRuntimeParserV1 : public MaemoQemuRuntimeParser
{
public:
    MaemoQemuRuntimeParserV1(const QString &madInfoOutput,
        const QString &targetName, const QString &maddeRoot);
    MaemoQemuRuntime parseRuntime();

private:
    void fillRuntimeInformation(MaemoQemuRuntime *runtime) const;
    void setEnvironment(MaemoQemuRuntime *runTime, const QString &envSpec) const;
};

class MaemoQemuRuntimeParserV2 : public MaemoQemuRuntimeParser
{
public:
    MaemoQemuRuntimeParserV2(const QString &madInfoOutput,
        const QString &targetName, const QString &maddeRoot);
    MaemoQemuRuntime parseRuntime();

private:
    struct Port {
        Port() : port(-1), ssh(false) {}
        int port;
        bool ssh;
    };

    void handleTargetTag(QString &runtimeName);
    MaemoQemuRuntime handleRuntimeTag();
    void handleEnvironmentTag(MaemoQemuRuntime &runtime);
    void handleVariableTag(MaemoQemuRuntime &runtime);
    QList<Port> handleTcpPortListTag();
    Port handlePortTag();
    MaemoQemuSettings::OpenGlMode openGlTagToEnum(const QString &tag) const;
};

MaemoQemuRuntimeParser::MaemoQemuRuntimeParser(const QString &madInfoOutput,
    const QString &targetName, const QString &maddeRoot)
    : m_targetName(targetName),
      m_maddeRoot(maddeRoot),
      m_madInfoReader(madInfoOutput)
{
}

MaemoQemuRuntime MaemoQemuRuntimeParser::parseRuntime(const QtSupport::BaseQtVersion *qtVersion)
{
    MaemoQemuRuntime runtime;
    const QString maddeRootPath = MaemoGlobal::maddeRoot(qtVersion->qmakeCommand().toString());
    QProcess madProc;
    if (!MaemoGlobal::callMad(madProc, QStringList() << QLatin1String("info"), qtVersion->qmakeCommand().toString(), false))
        return runtime;
    if (!madProc.waitForStarted() || !madProc.waitForFinished())
        return runtime;
    QString madInfoOutput = QString::fromLocal8Bit(madProc.readAllStandardOutput());
    const QString &targetName = MaemoGlobal::targetName(qtVersion->qmakeCommand().toString());
    runtime = MaemoQemuRuntimeParserV2(madInfoOutput, targetName, maddeRootPath)
        .parseRuntime();
    if (!runtime.m_name.isEmpty()) {
        runtime.m_root = maddeRootPath + QLatin1String("/runtimes/")
            + runtime.m_name;
    } else {
        runtime = MaemoQemuRuntimeParserV1(madInfoOutput, targetName,
            maddeRootPath).parseRuntime();
    }
    runtime.m_watchPath = runtime.m_root
        .left(runtime.m_root.lastIndexOf(QLatin1Char('/')));

    return runtime;
}

MaemoQemuRuntimeParserV1::MaemoQemuRuntimeParserV1(const QString &madInfoOutput,
    const QString &targetName, const QString &maddeRoot)
    : MaemoQemuRuntimeParser(madInfoOutput, targetName, maddeRoot)
{
}

MaemoQemuRuntime MaemoQemuRuntimeParserV1::parseRuntime()
{
    QStringList installedRuntimes;
    QString targetRuntime;
    while (!m_madInfoReader.atEnd() && !installedRuntimes.contains(targetRuntime)) {
        if (m_madInfoReader.readNext() == QXmlStreamReader::StartElement) {
            if (targetRuntime.isEmpty()
                && m_madInfoReader.name() == QLatin1String("target")) {
                const QXmlStreamAttributes &attrs = m_madInfoReader.attributes();
                if (attrs.value(QLatin1String("target_id")) == m_targetName)
                    targetRuntime = attrs.value(QLatin1String("runtime_id")).toString();
            } else if (m_madInfoReader.name() == QLatin1String("runtime")) {
                const QXmlStreamAttributes attrs = m_madInfoReader.attributes();
                while (!m_madInfoReader.atEnd()) {
                    if (m_madInfoReader.readNext() == QXmlStreamReader::EndElement
                         && m_madInfoReader.name() == QLatin1String("runtime"))
                        break;
                    if (m_madInfoReader.tokenType() == QXmlStreamReader::StartElement
                        && m_madInfoReader.name() == QLatin1String("installed")) {
                        if (m_madInfoReader.readNext() == QXmlStreamReader::Characters
                            && m_madInfoReader.text() == QLatin1String("true")) {
                            if (attrs.hasAttribute(QLatin1String("runtime_id")))
                                installedRuntimes << attrs.value(QLatin1String("runtime_id")).toString();
                            else if (attrs.hasAttribute(QLatin1String("id"))) {
                                // older MADDE seems to use only id
                                installedRuntimes << attrs.value(QLatin1String("id")).toString();
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    MaemoQemuRuntime runtime;
    if (installedRuntimes.contains(targetRuntime)) {
        runtime.m_name = targetRuntime;
        runtime.m_root = m_maddeRoot + QLatin1String("/runtimes/")
            + targetRuntime;
        fillRuntimeInformation(&runtime);
    }
    return runtime;

}

void MaemoQemuRuntimeParserV1::fillRuntimeInformation(MaemoQemuRuntime *runtime) const
{
    const QStringList files = QDir(runtime->m_root).entryList(QDir::Files
        | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    const QLatin1String infoFile("information");
    if (files.contains(infoFile)) {
        QFile file(runtime->m_root + QLatin1Char('/') + infoFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMap<QString, QString> map;
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                const QString &line = stream.readLine().trimmed();
                const int index = line.indexOf(QLatin1Char('='));
                map.insert(line.mid(0, index).remove(QLatin1Char('\'')),
                    line.mid(index + 1).remove(QLatin1Char('\'')));
            }

            runtime->m_bin = map.value(QLatin1String("qemu"));
            runtime->m_args = map.value(QLatin1String("qemu_args"));
            setEnvironment(runtime, map.value(QLatin1String("libpath")));
            runtime->m_sshPort = map.value(QLatin1String("sshport"));
            runtime->m_freePorts = Utils::PortList();
            int i = 2;
            while (true) {
                const QString port = map.value(QLatin1String("redirport")
                    + QString::number(i++));
                if (port.isEmpty())
                    break;
                runtime->m_freePorts.addPort(port.toInt());
            }

            // This is complex because of extreme MADDE weirdness.
            const QString root = m_maddeRoot + QLatin1Char('/');
            const bool pathIsRelative = QFileInfo(runtime->m_bin).isRelative();
            if (Utils::HostOsInfo::isWindowsHost()) {
                runtime->m_bin =
                        root + (pathIsRelative
                                ? QLatin1String("madlib/") + runtime->m_bin // Fremantle.
                                : runtime->m_bin)                           // Harmattan.
                        + QLatin1String(".exe");
            } else {
                runtime->m_bin = pathIsRelative
                        ? root + QLatin1String("madlib/") + runtime->m_bin // Fremantle.
                        : runtime->m_bin;                                  // Harmattan.
            }
        }
    }
}

void MaemoQemuRuntimeParserV1::setEnvironment(MaemoQemuRuntime *runTime,
    const QString &envSpec) const
{
    QString remainingEnvSpec = envSpec;
    QString currentKey;
    while (true) {
        const int nextEqualsSignPos
            = remainingEnvSpec.indexOf(QLatin1Char('='));
        if (nextEqualsSignPos == -1) {
            if (!currentKey.isEmpty())
                runTime->m_normalVars << MaemoQemuRuntime::Variable(currentKey,
                    remainingEnvSpec);
            break;
        }
        const int keyStartPos
            = remainingEnvSpec.lastIndexOf(QRegExp(QLatin1String("\\s")),
                nextEqualsSignPos) + 1;
        if (!currentKey.isEmpty()) {
            const int valueEndPos
                = remainingEnvSpec.lastIndexOf(QRegExp(QLatin1String("\\S")),
                    qMax(0, keyStartPos - 1)) + 1;
            runTime->m_normalVars << MaemoQemuRuntime::Variable(currentKey,
                remainingEnvSpec.left(valueEndPos));
        }
        currentKey = remainingEnvSpec.mid(keyStartPos,
            nextEqualsSignPos - keyStartPos);
        remainingEnvSpec.remove(0, nextEqualsSignPos + 1);
    }
}


MaemoQemuRuntimeParserV2::MaemoQemuRuntimeParserV2(const QString &madInfoOutput,
    const QString &targetName, const QString &maddeRoot)
    : MaemoQemuRuntimeParser(madInfoOutput, targetName, maddeRoot)
{
}

MaemoQemuRuntime MaemoQemuRuntimeParserV2::parseRuntime()
{
    QString runtimeName;
    QList<MaemoQemuRuntime> runtimes;
    while (m_madInfoReader.readNextStartElement()) {
        if (m_madInfoReader.name() == QLatin1String("madde")) {
            while (m_madInfoReader.readNextStartElement()) {
                if (m_madInfoReader.name() == QLatin1String("targets")) {
                    while (m_madInfoReader.readNextStartElement())
                        handleTargetTag(runtimeName);
                } else if (m_madInfoReader.name() == QLatin1String("runtimes")) {
                    while (m_madInfoReader.readNextStartElement()) {
                        const MaemoQemuRuntime &rt = handleRuntimeTag();
                        if (!rt.m_name.isEmpty() && !rt.m_bin.isEmpty()
                                && !rt.m_args.isEmpty()) {
                            runtimes << rt;
                        }
                    }
                } else {
                    m_madInfoReader.skipCurrentElement();
                }
            }
        }
    }
    foreach (const MaemoQemuRuntime &rt, runtimes) {
        if (rt.m_name == runtimeName)
            return rt;
    }
    return MaemoQemuRuntime();
}

void MaemoQemuRuntimeParserV2::handleTargetTag(QString &runtimeName)
{
    const QXmlStreamAttributes &attrs = m_madInfoReader.attributes();
    if (m_madInfoReader.name() == QLatin1String("target") && runtimeName.isEmpty()
            && attrs.value(QLatin1String("name")) == m_targetName
            && attrs.value(QLatin1String("installed")) == QLatin1String("true")) {
        while (m_madInfoReader.readNextStartElement()) {
            if (m_madInfoReader.name() == QLatin1String("runtime"))
                runtimeName = m_madInfoReader.readElementText();
            else
                m_madInfoReader.skipCurrentElement();
        }
    } else {
        m_madInfoReader.skipCurrentElement();
    }
}

MaemoQemuRuntime MaemoQemuRuntimeParserV2::handleRuntimeTag()
{
    MaemoQemuRuntime runtime;
    const QXmlStreamAttributes &attrs = m_madInfoReader.attributes();
    if (m_madInfoReader.name() != QLatin1String("runtime")
            || attrs.value(QLatin1String("installed")) != QLatin1String("true")) {
        m_madInfoReader.skipCurrentElement();
        return runtime;
    }
    runtime.m_name = attrs.value(QLatin1String("name")).toString();
    while (m_madInfoReader.readNextStartElement()) {
        if (m_madInfoReader.name() == QLatin1String("exec-path")) {
            runtime.m_bin = m_madInfoReader.readElementText();
        } else if (m_madInfoReader.name() == QLatin1String("args")) {
            runtime.m_args = m_madInfoReader.readElementText();
        } else if (m_madInfoReader.name() == QLatin1String("environment")) {
            handleEnvironmentTag(runtime);
        } else if (m_madInfoReader.name() == QLatin1String("tcpportmap")) {
            const QList<Port> &ports = handleTcpPortListTag();
            foreach (const Port &port, ports) {
                if (port.ssh)
                    runtime.m_sshPort = QString::number(port.port);
                else
                    runtime.m_freePorts.addPort(port.port);
            }
        } else {
            m_madInfoReader.skipCurrentElement();
        }
    }
    return runtime;
}

void MaemoQemuRuntimeParserV2::handleEnvironmentTag(MaemoQemuRuntime &runtime)
{
    while (m_madInfoReader.readNextStartElement())
        handleVariableTag(runtime);

    if (Utils::HostOsInfo::isWindowsHost()) {
        const QString root = QDir::toNativeSeparators(m_maddeRoot)
                + QLatin1Char('/');
        const QLatin1Char colon(';');
        const QLatin1String key("PATH");
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        runtime.m_normalVars << MaemoQemuRuntime::Variable(key,
                root + QLatin1String("bin") + colon + env.value(key));
        runtime.m_normalVars << MaemoQemuRuntime::Variable(key,
                root + QLatin1String("madlib") + colon + env.value(key));
    }
}

void MaemoQemuRuntimeParserV2::handleVariableTag(MaemoQemuRuntime &runtime)
{
    if (m_madInfoReader.name() != QLatin1String("variable")) {
        m_madInfoReader.skipCurrentElement();
        return;
    }

    const bool isGlBackend = m_madInfoReader.attributes().value(QLatin1String("purpose"))
        == QLatin1String("glbackend");
    QString varName;
    QString varValue;
    while (m_madInfoReader.readNextStartElement()) {
        const QXmlStreamAttributes &attrs = m_madInfoReader.attributes();
        if (m_madInfoReader.name() == QLatin1String("name")) {
            varName = m_madInfoReader.readElementText();
        } else if (m_madInfoReader.name() == QLatin1String("value")
                   && attrs.value(QLatin1String("set")) != QLatin1String("false")) {
            varValue = m_madInfoReader.readElementText();
            if (isGlBackend) {
                MaemoQemuSettings::OpenGlMode openGlMode
                    = openGlTagToEnum(attrs.value(QLatin1String("option")).toString());
                runtime.m_openGlBackendVarValues.insert(openGlMode, varValue);
            }
        } else {
            m_madInfoReader.skipCurrentElement();
        }
    }

    if (varName.isEmpty())
        return;
    if (isGlBackend)
        runtime.m_openGlBackendVarName = varName;
    else
        runtime.m_normalVars << MaemoQemuRuntime::Variable(varName, varValue);
}

QList<MaemoQemuRuntimeParserV2::Port> MaemoQemuRuntimeParserV2::handleTcpPortListTag()
{
    QList<Port> ports;
    while (m_madInfoReader.readNextStartElement()) {
        const Port &port = handlePortTag();
        if (port.port != -1)
            ports << port;
    }
    return ports;
}

MaemoQemuRuntimeParserV2::Port MaemoQemuRuntimeParserV2::handlePortTag()
{
    Port port;
    if (m_madInfoReader.name() == QLatin1String("port")) {
        const QXmlStreamAttributes &attrs = m_madInfoReader.attributes();
        port.ssh = attrs.value(QLatin1String("service")) == QLatin1String("ssh");
        while (m_madInfoReader.readNextStartElement()) {
            if (m_madInfoReader.name() == QLatin1String("host"))
                port.port = m_madInfoReader.readElementText().toInt();
            else
                m_madInfoReader.skipCurrentElement();
        }
    }
    return port;
}

MaemoQemuSettings::OpenGlMode MaemoQemuRuntimeParserV2::openGlTagToEnum(const QString &tag) const
{
    if (tag == QLatin1String("hardware-acceleration"))
        return MaemoQemuSettings::HardwareAcceleration;
    if (tag == QLatin1String("software-rendering"))
        return MaemoQemuSettings::SoftwareRendering;
    if (tag == QLatin1String("autodetect"))
        return MaemoQemuSettings::AutoDetect;
    QTC_CHECK(false);
    return MaemoQemuSettings::AutoDetect;
}

}   // namespace Internal
}   // namespace Madde
