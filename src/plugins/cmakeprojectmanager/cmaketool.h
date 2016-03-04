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

    typedef std::function<QString (const ProjectExplorer::Kit *, const QString &)> PathMapper;

    explicit CMakeTool(Detection d, const Core::Id &id);
    explicit CMakeTool(const QVariantMap &map, bool fromSdk);
    ~CMakeTool() override = default;

    static Core::Id createId();

    bool isValid() const;

    Core::Id id() const { return m_id; }
    QVariantMap toMap () const;

    void setCMakeExecutable(const Utils::FileName &executable);

    Utils::FileName cmakeExecutable() const;
    QStringList supportedGenerators() const;
    TextEditor::Keywords keywords();

    bool isAutoDetected() const;
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void setPathMapper(const PathMapper &includePathMapper);
    QString mapAllPaths(const ProjectExplorer::Kit *kit, const QString &in) const;

private:
    Utils::SynchronousProcessResponse run(const QString &arg) const;
    void parseFunctionDetailsOutput(const QString &output);
    QStringList parseVariableOutput(const QString &output);

    Core::Id m_id;
    QString m_displayName;
    Utils::FileName m_executable;

    bool m_isAutoDetected;

    mutable bool m_didAttemptToRun;
    mutable bool m_didRun;

    mutable QStringList m_generators;
    mutable QMap<QString, QStringList> m_functionArgs;
    mutable QStringList m_variables;
    mutable QStringList m_functions;

    PathMapper m_pathMapper;
};

} // namespace CMakeProjectManager
