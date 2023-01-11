// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangtoolruncontrol.h"

#include <cppeditor/clangdiagnosticconfig.h>

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
    AnalyzeUnit unit;
    QString overlayFilePath = {};
};
class ClangToolRunner : public QObject
{
    Q_OBJECT

public:
    ClangToolRunner(const AnalyzeInputData &input, QObject *parent = nullptr);

    QString name() const { return m_name; }
    Utils::FilePath executable() const { return m_executable; }
    QString fileToAnalyze() const { return m_input.unit.file; }
    QString outputFilePath() const { return m_outputFilePath; }
    bool supportsVFSOverlay() const;

    // compilerOptions is expected to contain everything except:
    //   (1) file to analyze
    //   (2) -o output-file
    bool run();

signals:
    void finishedWithSuccess(const QString &fileToAnalyze);
    void finishedWithFailure(const QString &errorMessage, const QString &errorDetails);

private:
    void onProcessOutput();
    void onProcessDone();

    QStringList mainToolArguments() const;
    QString commandlineAndOutput() const;

    const AnalyzeInputData m_input;
    Utils::QtcProcess m_process;

    QString m_name;
    Utils::FilePath m_executable;
    ArgsCreator m_argsCreator;

    QString m_outputFilePath;
};

} // namespace Internal
} // namespace ClangTools
