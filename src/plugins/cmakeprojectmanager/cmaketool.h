/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cmake_global.h"

#include <coreplugin/id.h>
#include <texteditor/codeassist/keywordscompletionassist.h>

#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>

#include <QObject>
#include <QMap>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace ProjectExplorer { class Kit; }

namespace CMakeProjectManager {

class CMAKE_EXPORT CMakeTool : public QObject
{
    Q_OBJECT
public:
    enum Detection {
        ManualDetection,
        AutoDetection
    };

    struct Version
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        QByteArray fullVersion;
    };

    class Generator
    {
    public:
        Generator(const QString &n, const QStringList &eg, bool pl = true, bool ts = true) :
            name(n), extraGenerators(eg), supportsPlatform(pl), supportsToolset(ts)
        { }

        QString name;
        QStringList extraGenerators;
        bool supportsPlatform = true;
        bool supportsToolset = true;

        bool matches(const QString &n, const QString &ex) const;
    };

    typedef std::function<Utils::FileName (const Utils::FileName &)> PathMapper;

    explicit CMakeTool(Detection d, const Core::Id &id);
    explicit CMakeTool(const QVariantMap &map, bool fromSdk);
    ~CMakeTool() override = default;

    static Core::Id createId();

    bool isValid() const;

    Core::Id id() const { return m_id; }
    QVariantMap toMap () const;

    void setCMakeExecutable(const Utils::FileName &executable);
    void setAutorun(bool autoRun);

    Utils::FileName cmakeExecutable() const;
    bool isAutoRun() const;
    QList<Generator> supportedGenerators() const;
    TextEditor::Keywords keywords();
    bool hasServerMode() const;
    Version version() const;

    bool isAutoDetected() const;
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void setPathMapper(const PathMapper &includePathMapper);
    PathMapper pathMapper() const;

private:
    enum class QueryType {
        GENERATORS,
        SERVER_MODE,
        VERSION
    };
    void readInformation(QueryType type) const;

    Utils::SynchronousProcessResponse run(const QStringList &args, bool mayFail = false) const;
    void parseFunctionDetailsOutput(const QString &output);
    QStringList parseVariableOutput(const QString &output);

    void fetchGeneratorsFromHelp() const;
    void fetchVersionFromVersionOutput() const;
    void fetchFromCapabilities() const;

    Core::Id m_id;
    QString m_displayName;
    Utils::FileName m_executable;

    bool m_isAutoRun = true;
    bool m_isAutoDetected = false;

    mutable bool m_didAttemptToRun = false;
    mutable bool m_didRun = false;
    mutable bool m_hasServerMode = false;

    mutable bool m_queriedServerMode = false;
    mutable bool m_triedCapabilities = false;

    mutable QList<Generator> m_generators;
    mutable QMap<QString, QStringList> m_functionArgs;
    mutable QStringList m_variables;
    mutable QStringList m_functions;
    mutable Version m_version;

    PathMapper m_pathMapper;
};

} // namespace CMakeProjectManager
