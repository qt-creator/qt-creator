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

#include <coreplugin/core_global.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/id.h>

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QSharedPointer>
#include <QTextCodec>
#include <QMetaType>

namespace Utils { class QtcProcess; }
namespace Core {

class CORE_EXPORT ExternalTool : public QObject
{
    Q_OBJECT

public:
    enum OutputHandling {
        Ignore,
        ShowInPane,
        ReplaceSelection
    };

    ExternalTool();
    explicit ExternalTool(const ExternalTool *other);
    ~ExternalTool() override;

    QString id() const;
    QString description() const;
    QString displayName() const;
    QString displayCategory() const;
    int order() const;
    OutputHandling outputHandling() const;
    OutputHandling errorHandling() const;
    bool modifiesCurrentDocument() const;

    Utils::FilePaths executables() const;
    QString arguments() const;
    QString input() const;
    Utils::FilePath workingDirectory() const;
    Utils::Id baseEnvironmentProviderId() const;
    Utils::Environment baseEnvironment() const;
    Utils::EnvironmentItems environmentUserChanges() const;

    void setFileName(const Utils::FilePath &fileName);
    void setPreset(QSharedPointer<ExternalTool> preset);
    Utils::FilePath fileName() const;
    // all tools that are preset (changed or unchanged) have the original value here:
    QSharedPointer<ExternalTool> preset() const;

    static ExternalTool *createFromXml(const QByteArray &xml, QString *errorMessage = nullptr, const QString &locale = QString());
    static ExternalTool *createFromFile(const Utils::FilePath &fileName, QString *errorMessage = nullptr,
                                        const QString &locale = QString());

    bool save(QString *errorMessage = nullptr) const;

    bool operator==(const ExternalTool &other) const;
    bool operator!=(const ExternalTool &other) const { return !((*this) == other); }
    ExternalTool &operator=(const ExternalTool &other);

    void setId(const QString &id);
    void setDisplayCategory(const QString &category);
    void setDisplayName(const QString &name);
    void setDescription(const QString &description);
    void setOutputHandling(OutputHandling handling);
    void setErrorHandling(OutputHandling handling);
    void setModifiesCurrentDocument(bool modifies);
    void setExecutables(const Utils::FilePaths &executables);
    void setArguments(const QString &arguments);
    void setInput(const QString &input);
    void setWorkingDirectory(const Utils::FilePath &workingDirectory);
    void setBaseEnvironmentProviderId(Utils::Id id);
    void setEnvironmentUserChanges(const Utils::EnvironmentItems &items);

private:
    QString m_id;
    QString m_description;
    QString m_displayName;
    QString m_displayCategory;
    int m_order = -1;
    Utils::FilePaths m_executables;
    QString m_arguments;
    QString m_input;
    Utils::FilePath m_workingDirectory;
    Utils::Id m_baseEnvironmentProviderId;
    Utils::EnvironmentItems m_environment;
    OutputHandling m_outputHandling = ShowInPane;
    OutputHandling m_errorHandling = ShowInPane;
    bool m_modifiesCurrentDocument = false;

    Utils::FilePath m_filePath;
    Utils::FilePath m_presetFileName;
    QSharedPointer<ExternalTool> m_presetTool;
};

class CORE_EXPORT ExternalToolRunner : public QObject
{
    Q_OBJECT
public:
    ExternalToolRunner(const ExternalTool *tool);
    ~ExternalToolRunner() override;

    bool hasError() const;
    QString errorString() const;

private:
    void finished();
    void error(QProcess::ProcessError error);
    void readStandardOutput();
    void readStandardError();

    void run();
    bool resolve();

    const ExternalTool *m_tool; // is a copy of the tool that was passed in
    Utils::FilePath m_resolvedExecutable;
    QString m_resolvedArguments;
    QString m_resolvedInput;
    Utils::FilePath m_resolvedWorkingDirectory;
    Utils::Environment m_resolvedEnvironment;
    Utils::QtcProcess *m_process;
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;
    QString m_processOutput;
    Utils::FilePath m_expectedFilePath;
    bool m_hasError;
    QString m_errorString;
};

} // Core

Q_DECLARE_METATYPE(Core::ExternalTool *)
