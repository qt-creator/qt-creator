// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditor_global.h"

#include <utils/filepath.h>
#include <QMutex>
#include <QObject>

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT QmllsSettingsManager : public QObject
{
    Q_OBJECT

public:
    static QmllsSettingsManager *instance();

    Utils::FilePath latestQmlls();
    void setupAutoupdate();

    bool useQmlls() const;
    bool useLatestQmlls() const;

public slots:
    void checkForChanges();
signals:
    void settingsChanged();

private:
    QMutex m_mutex;
    bool m_useQmlls = true;
    bool m_useLatestQmlls = false;
    bool m_disableBuiltinCodemodel = false;
    bool m_generateQmllsIniFiles = false;
    Utils::FilePath m_latestQmlls;
};

} // namespace QmlJSEditor
