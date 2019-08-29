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

#include "clangtoolslogfilereader.h"

#include <QString>
#include <QProcess>

#include <memory>

namespace Utils { class Environment; }

namespace ClangTools {
namespace Internal {

using ArgsCreator = std::function<QStringList(const QStringList &baseOptions)>;

class ClangToolRunner : public QObject
{
    Q_OBJECT

public:
    ClangToolRunner(QObject *parent = nullptr) : QObject(parent) {}
    ~ClangToolRunner() override;

    void init(const QString &outputDirPath, const Utils::Environment &environment);
    void setName(const QString &name) { m_name = name; }
    void setExecutable(const QString &executable) { m_executable = executable; }
    void setArgsCreator(const ArgsCreator &argsCreator) { m_argsCreator = argsCreator; }
    void setOutputFileFormat(const OutputFileFormat &format) { m_outputFileFormat = format; }

    QString name() const { return m_name; }
    QString executable() const { return m_executable; }
    OutputFileFormat outputFileFormat() const { return m_outputFileFormat; }
    QString fileToAnalyze() const { return m_fileToAnalyze; }
    QString outputFilePath() const { return m_outputFilePath; }

    // compilerOptions is expected to contain everything except:
    //   (1) file to analyze
    //   (2) -o output-file
    bool run(const QString &fileToAnalyze, const QStringList &compilerOptions = QStringList());

signals:
    void finishedWithSuccess(const QString &fileToAnalyze);
    void finishedWithFailure(const QString &errorMessage, const QString &errorDetails);

private:
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

    QString commandlineAndOutput() const;

private:
    QString m_outputDirPath;
    QProcess m_process;
    QByteArray m_processOutput;

    QString m_name;
    QString m_executable;
    ArgsCreator m_argsCreator;
    OutputFileFormat m_outputFileFormat = OutputFileFormat::Yaml;

    QString m_fileToAnalyze;
    QString m_outputFilePath;
    QString m_commandLine;
};

} // namespace Internal
} // namespace ClangTools
