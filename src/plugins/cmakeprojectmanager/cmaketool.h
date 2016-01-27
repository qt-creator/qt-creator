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

#include <texteditor/codeassist/keywordscompletionassist.h>
#include <utils/fileutils.h>
#include <coreplugin/id.h>

#include <QObject>
#include <QString>
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

    typedef std::function<QString (ProjectExplorer::Kit *, const QString &)> PathMapper;

    explicit CMakeTool(Detection d, const Core::Id &id = Core::Id());
    explicit CMakeTool(const QVariantMap &map, bool fromSdk);
    ~CMakeTool() override;

    enum State { Invalid, RunningBasic, RunningFunctionList, RunningFunctionDetails,
                 RunningPropertyList, RunningVariableList, RunningDone };
    void cancel();
    bool isValid() const;

    Core::Id id() const { return m_id; }
    QVariantMap toMap () const;

    void setCMakeExecutable(const Utils::FileName &executable);
    Utils::FileName cmakeExecutable() const;
    bool hasCodeBlocksMsvcGenerator() const;
    bool hasCodeBlocksNinjaGenerator() const;
    TextEditor::Keywords keywords();
    bool isAutoDetected() const;
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void setPathMapper(const PathMapper &includePathMapper);
    QString mapAllPaths(ProjectExplorer::Kit *kit, const QString &in) const;

private:
    void finished(int exitCode);

    void createId();
    void finishStep();
    void startNextStep();
    bool startProcess(const QStringList &args);
    void parseFunctionOutput(const QByteArray &output);
    void parseFunctionDetailsOutput(const QByteArray &output);
    void parseVariableOutput(const QByteArray &output);
    void parseDone();
    QString formatFunctionDetails(const QString &command, const QString &args);

    State m_state = Invalid;
    QProcess *m_process = 0;
    Utils::FileName m_executable;

    bool m_isAutoDetected;
    bool m_hasCodeBlocksMsvcGenerator = false;
    bool m_hasCodeBlocksNinjaGenerator = false;

    QMap<QString, QStringList> m_functionArgs;
    QStringList m_variables;
    QStringList m_functions;

    Core::Id m_id;
    QString m_displayName;
    PathMapper m_pathMapper;
};

} // namespace CMakeProjectManager

