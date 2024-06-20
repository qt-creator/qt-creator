// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QMutex>
#include <QObject>
#include <QVersionNumber>

namespace QmlJSEditor::Internal {

class QmllsSettingsManager : public QObject
{
    Q_OBJECT

public:
    static const inline QVersionNumber mininumQmllsVersion = QVersionNumber(6, 8);

    static QmllsSettingsManager *instance();

    Utils::FilePath latestQmlls();
    void setupAutoupdate();

    bool useQmlls() const;
    bool useLatestQmlls() const;
    bool ignoreMinimumQmllsVersion() const;

public slots:
    void checkForChanges();
signals:
    void settingsChanged();

private:
    QMutex m_mutex;
    bool m_useQmlls = true;
    bool m_useLatestQmlls = false;
    bool m_ignoreMinimumQmllsVersion = false;
    bool m_disableBuiltinCodemodel = false;
    bool m_generateQmllsIniFiles = false;
    Utils::FilePath m_latestQmlls;
};

} // QmlJSEditor::Internal
