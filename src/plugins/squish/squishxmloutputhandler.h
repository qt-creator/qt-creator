// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QXmlStreamReader>

namespace Squish {
namespace Internal {

class SquishResultItem;

class SquishXmlOutputHandler : public QObject
{
    Q_OBJECT
public:
    explicit SquishXmlOutputHandler(QObject *parent = nullptr);
    void clearForNextRun();

    static void mergeResultFiles(const QStringList &reportFiles,
                                 const QString &resultsDirectory,
                                 const QString &suiteName,
                                 QString *error = nullptr);

signals:
    void resultItemCreated(SquishResultItem *resultItem);
    void updateStatus(const QString &text);
    void increasePassCounter();
    void increaseFailCounter();

public slots:
    void outputAvailable(const QByteArray &output);

private:
    QXmlStreamReader m_xmlReader;
};

} // namespace Internal
} // namespace Squish
