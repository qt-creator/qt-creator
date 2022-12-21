// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHttpMultiPart>
#include <QObject>

class DumpSender : public QObject
{
    Q_OBJECT

public:
    explicit DumpSender(QObject *parent = nullptr);

    int dumperSize() const;

    void sendDumpAndQuit();
    void restartCrashedApplication();
    void restartCrashedApplicationAndSendDump();
    void setEmailAddress(const QString &email);
    void setCommentText(const QString &comment);

signals:
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    QHttpMultiPart m_httpMultiPart;
    QByteArray m_formData;
    QString m_applicationFilePath;
    QString m_emailAddress;
    QString m_commentText;
};
