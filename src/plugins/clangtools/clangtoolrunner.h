// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"

#include <cppeditor/clangdiagnosticconfig.h>

#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <memory>

namespace ClangTools {
namespace Internal {

struct AnalyzeUnit {
    AnalyzeUnit(const FileInfo &fileInfo,
                const Utils::FilePath &clangResourceDir,
                const QString &clangVersion);

    QString file;
    QStringList arguments; // without file itself and "-o somePath"
};
using AnalyzeUnits = QList<AnalyzeUnit>;

struct AnalyzeInputData
{
    CppEditor::ClangToolType tool = CppEditor::ClangToolType::Tidy;
    CppEditor::ClangDiagnosticConfig config;
    Utils::FilePath outputDirPath;
    Utils::Environment environment;
    AnalyzeUnit unit;
    QString overlayFilePath = {};
};

struct AnalyzeOutputData
{
    bool success = true;
    QString fileToAnalyze;
    QString outputFilePath;
    QString toolName;
    QString errorMessage = {};
    QString errorDetails = {};
};

class ClangToolRunner : public QObject
{
    Q_OBJECT

public:
    ClangToolRunner(const AnalyzeInputData &input, QObject *parent = nullptr);

    QString name() const { return m_name; }
    QString fileToAnalyze() const { return m_input.unit.file; }
    bool supportsVFSOverlay() const;

    // compilerOptions is expected to contain everything except:
    //   (1) file to analyze
    //   (2) -o output-file
    bool run();

signals:
    void done(const AnalyzeOutputData &output);

private:
    void onProcessDone();

    QStringList mainToolArguments() const;

    const AnalyzeInputData m_input;
    Utils::QtcProcess m_process;

    QString m_name;
    Utils::FilePath m_executable;
    QString m_outputFilePath;
};

} // namespace Internal
} // namespace ClangTools
