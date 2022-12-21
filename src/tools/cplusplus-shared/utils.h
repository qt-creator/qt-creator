// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

namespace CplusplusToolsUtils {

void executeCommand(const QString &command, const QStringList &arguments, const QString &outputFile,
                    bool verbose = false);

// Preprocess a file by calling an external compiler in preprocessor mode (-E, /E).
class SystemPreprocessor
{
public:
    SystemPreprocessor(bool verbose = false);
    void preprocessFile(const QString &inputFile, const QString &outputFile) const;

private:
    void check() const;

    QMap<QString, QString> m_knownCompilers;
    QString m_compiler; // Compiler that will be called in preprocessor mode
    QStringList m_compilerArguments;
    bool m_verbose;
};

} // namespace
