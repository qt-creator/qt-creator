// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <utils/filepath.h>

#include <QVersionNumber>

namespace QmlJSTools {

class QmlFormatProcess;

class QMLJSTOOLS_EXPORT QmlFormatSettings : public QObject
{
    Q_OBJECT

public:
    Utils::FilePath latestQmlFormatPath() const;
    QVersionNumber latestQmlFormatVersion() const;
    void evaluateLatestQmlFormat();

    static QmlFormatSettings &instance();
    static Utils::FilePath currentQmlFormatIniFile(const Utils::FilePath &path);
    static Utils::FilePath globalQmlFormatIniFile();

signals:
    void qmlformatIniCreated(Utils::FilePath iniFile);

private:
    QmlFormatSettings();
    ~QmlFormatSettings();

    void generateQmlFormatIniContent();

    Utils::FilePath m_latestQmlFormat;
    QVersionNumber m_latestVersion;
};

} // namespace QmlJSTools
