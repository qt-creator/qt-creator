/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QObject>
#include <QDebug>
#include <QDataStream>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

static const PropertyName customIdProperty = {("customId")};
static const PropertyName annotationProperty = {("annotation")};
static const PropertyName globalAnnotationProperty = {("globalAnnotation")};
static const PropertyName globalAnnotationStatus = {("globalAnnotationStatus")};

class GlobalAnnotationStatus
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

    ~GlobalAnnotationStatus() = default;

    void setStatus(int statusId);
    void setStatus(Status status);
    Status status() const;

    QString toQString() const;
    void fromQString(const QString &str);

private:
    Status m_status;
};

class Comment
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

    bool isEmpty();

    QString toQString() const;

    friend QDebug &operator<<(QDebug &stream, const Comment &comment);

    friend QDataStream &operator<<(QDataStream &stream, const Comment &comment);
    friend QDataStream &operator>>(QDataStream &stream, Comment &comment);

private:
    QString m_title;
    QString m_author;
    QString m_text;
    qint64 m_timestamp;
};

class Annotation
{
public:
    Annotation();
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

    friend QDebug &operator<<(QDebug &stream, const Annotation &annotation);

    friend QDataStream &operator<<(QDataStream &stream, const Annotation &annotation);
    friend QDataStream &operator>>(QDataStream &stream, Annotation &annotation);

private:
    QVector<Comment> m_comments;
};

QDebug &operator<<(QDebug &stream, const Comment &comment);
QDebug &operator<<(QDebug &stream, const Annotation &annotation);

QDataStream &operator<<(QDataStream &stream, const Comment &comment);
QDataStream &operator>>(QDataStream &stream, Comment &comment);
QDataStream &operator<<(QDataStream &stream, const Annotation &annotation);
QDataStream &operator>>(QDataStream &stream, Annotation &annotation);

}

Q_DECLARE_METATYPE(QmlDesigner::Comment);
Q_DECLARE_METATYPE(QmlDesigner::Annotation);
