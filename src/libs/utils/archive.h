// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "commandline.h"

#include <QObject>

namespace Utils {

class FilePath;
class QtcProcess;

class QTCREATOR_UTILS_EXPORT Archive : public QObject
{
    Q_OBJECT
public:
    Archive(const FilePath &src, const FilePath &dest);
    ~Archive();

    bool isValid() const;
    void unarchive();

    static bool supportsFile(const FilePath &filePath, QString *reason = nullptr);

signals:
    void outputReceived(const QString &output);
    void finished(bool success);

private:
    CommandLine m_commandLine;
    FilePath m_workingDirectory;
    std::unique_ptr<QtcProcess> m_process;
};

} // namespace Utils
