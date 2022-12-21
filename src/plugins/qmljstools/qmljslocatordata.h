// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <qmljs/qmljsdocument.h>

#include <QObject>
#include <QHash>
#include <QMutex>

namespace QmlJSTools {
namespace Internal {

class LocatorData : public QObject
{
    Q_OBJECT
public:
    LocatorData();
    ~LocatorData() override;

    enum EntryType
    {
        Function
    };

    class Entry
    {
    public:
        EntryType type;
        QString symbolName;
        QString displayName;
        QString extraInfo;
        Utils::FilePath fileName;
        int line;
        int column;
    };

    QHash<Utils::FilePath, QList<Entry>> entries() const;

private:
    void onDocumentUpdated(const QmlJS::Document::Ptr &doc);
    void onAboutToRemoveFiles(const Utils::FilePaths &files);

    mutable QMutex m_mutex;
    QHash<Utils::FilePath, QList<Entry>> m_entries;
};

} // namespace Internal
} // namespace QmlJSTools
