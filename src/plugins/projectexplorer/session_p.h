// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "session.h"

#include <QFutureInterface>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {

class SessionManagerPrivate
{
public:
    void restoreValues(const PersistentSettingsReader &reader);
    void restoreEditors(const PersistentSettingsReader &reader);
    void sessionLoadingProgress();

    static QString windowTitleAddition(const FilePath &filePath);
    static QString sessionTitle(const FilePath &filePath);

    QString m_sessionName = "default";
    bool m_virginSession = true;
    bool m_loadingSession = false;

    mutable QStringList m_sessions;
    mutable QHash<QString, QDateTime> m_sessionDateTimes;
    QHash<QString, QDateTime> m_lastActiveTimes;

    QMap<QString, QVariant> m_values;
    QFutureInterface<void> m_future;
    PersistentSettingsWriter *m_writer = nullptr;
};

extern SessionManagerPrivate *sb_d;

} // namespace ProjectExplorer
