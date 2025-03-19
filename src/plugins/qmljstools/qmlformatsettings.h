// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"
#include <utils/filepath.h>

#include <qtsupport/qtversionmanager.h>

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT QmlFormatSettings : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QmlFormatSettings)
public:
    Utils::FilePath latestQmlFormatPath() const;
    QVersionNumber latestQmlFormatVersion() const;
    void generateQmlFormatIniContent();
    void evaluateLatestQmlFormat();

    static QmlFormatSettings &instance();
    static Utils::FilePath currentQmlFormatIniFile(const Utils::FilePath &path);
    static Utils::FilePath globalQmlFormatIniFile();

signals:
    void versionEvaluated();
    void qmlformatIniCreated(Utils::FilePath iniFile);
private:
    QmlFormatSettings();
    Utils::FilePath m_latestQmlFormat;
    QVersionNumber m_latestVersion;
};

} // namespace QmlJSTools
