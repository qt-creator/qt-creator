// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <memory>

namespace ClangTools {
namespace Internal {

using ArgsCreator = std::function<QStringList(const QStringList &baseOptions)>;

struct AnalyzeInputData
{
    CppEditor::ClangToolType tool = CppEditor::ClangToolType::Tidy;
    CppEditor::ClangDiagnosticConfig config;
    Utils::FilePath outputDirPath;
    Utils::Environment environment;
};
class ClangToolRunner : public QObject
{
    Q_OBJECT

public:
    ClangToolRunner(const AnalyzeInputData &input, QObject *parent = nullptr);

    void setVFSOverlay(const QString overlayFilePath) { m_overlayFilePath = overlayFilePath; }

    QString name() const { return m_name; }
    Utils::FilePath executable() const { return m_executable; }
    QString fileToAnalyze() const { return m_fileToAnalyze; }
    QString outputFilePath() const { return m_outputFilePath; }
    bool supportsVFSOverlay() const;

    // compilerOptions is expected to contain everything except:
    //   (1) file to analyze
    //   (2) -o output-file
    bool run(const QString &fileToAnalyze, const QStringList &compilerOptions = {});

signals:
    void finishedWithSuccess(const QString &fileToAnalyze);
    void finishedWithFailure(const QString &errorMessage, const QString &errorDetails);

private:
    void onProcessOutput();
    void onProcessDone();

    QStringList mainToolArguments() const;
    QString commandlineAndOutput() const;

    QString m_overlayFilePath;
    Utils::FilePath m_outputDirPath;
    Utils::QtcProcess m_process;

    QString m_name;
    Utils::FilePath m_executable;
    ArgsCreator m_argsCreator;

    QString m_fileToAnalyze;
    QString m_outputFilePath;
    Utils::CommandLine m_commandLine;
};

} // namespace Internal
} // namespace ClangTools
