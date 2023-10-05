// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <QDebug>
#include <QJsonObject>
#include <QObject>

#include "nodeinstanceglobal.h"
#include "qmldesignercorelib_global.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT GlobalAnnotationStatus
{
public:
    enum Status {
        NoStatus = -1,
        InProgress = 0,
        InReview = 1,
        Done = 2
    };

    GlobalAnnotationStatus();
    GlobalAnnotationStatus(Status status);

    void setStatus(int statusId);
    void setStatus(Status status);
    Status status() const;

    QString toQString() const;
    void fromQString(const QString &str);

private:
    Status m_status;
};

class QMLDESIGNERCORE_EXPORT Comment
{
public:
    Comment();
    Comment(const QString &title, const QString &author = QString(), const QString &text = QString(), qint64 timestamp = 0);

    ~Comment() = default;

    QString title() const;
    void setTitle(const QString &title);

    QString text() const;
    void setText(const QString &text);

    QString deescapedText() const;

    QString author() const;
    void setAuthor(const QString &author);

    QString timestampStr() const;
    QString timestampStr(const QString &format) const;
    qint64 timestamp() const;
    void setTimestamp(qint64 timestamp);
    void updateTimestamp();

    bool sameContent(const Comment &comment) const; //everything is similar besides timestamp
    static bool sameContent(const Comment &a, const Comment &b);
    bool operator==(const Comment &comment) const; //everything is similar.

    bool isEmpty() const;

    QString toQString() const;
    QJsonValue toJsonValue() const;
    bool fromJsonValue(QJsonValue const &);

    friend QDebug &operator<<(QDebug &stream, const Comment &comment);

    QMLDESIGNERCORE_EXPORT friend QDataStream &operator<<(QDataStream &stream,
                                                          const Comment &comment);
    QMLDESIGNERCORE_EXPORT friend QDataStream &operator>>(QDataStream &stream, Comment &comment);

private:
    QString m_title;
    QString m_author;
    QString m_text;
    qint64 m_timestamp;
};

class QMLDESIGNERCORE_EXPORT Annotation
{
public:
    Annotation();
    Annotation(const QString &string) { fromQString(string); }
    ~Annotation() = default;

    QVector<Comment> comments() const;
    bool hasComments() const;
    void setComments(const QVector<Comment> &comments);
    void removeComments();
    int commentsSize() const;

    Comment comment(int n) const;
    void addComment(const Comment &comment);
    bool updateComment(const Comment &comment, int n);
    bool removeComment(int n);

    QString toQString() const;
    void fromQString(const QString &str);
    QJsonValue toJsonValue() const;
    bool fromJsonValue(QJsonValue const &);

    friend QDebug &operator<<(QDebug &stream, const Annotation &annotation);

    QMLDESIGNERCORE_EXPORT friend QDataStream &operator<<(QDataStream &stream,
                                                          const Annotation &annotation);
    QMLDESIGNERCORE_EXPORT friend QDataStream &operator>>(QDataStream &stream,
                                                          Annotation &annotation);

private:
    QVector<Comment> m_comments;
};

QDebug &operator<<(QDebug &stream, const Comment &comment);
QDebug &operator<<(QDebug &stream, const Annotation &annotation);

QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream, const Comment &comment);
QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream, Comment &comment);
QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream, const Annotation &annotation);
QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream, Annotation &annotation);
} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Comment);
Q_DECLARE_METATYPE(QmlDesigner::Annotation);
