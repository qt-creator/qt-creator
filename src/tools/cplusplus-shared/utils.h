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

#include <QString>
#include <QStringList>
#include <QMap>

namespace CplusplusToolsUtils {

QString portableExecutableName(const QString &executable);
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
