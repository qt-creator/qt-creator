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

#include "annotation.h"

#include <QDateTime>
#include <QString>

namespace QmlDesigner {

static const QString s_sep = " //;;// "; //separator

Comment::Comment()
    : m_title(QString())
    , m_author(QString())
    , m_text(QString())
    , m_timestamp(0)
{}

Comment::Comment(const QString &title, const QString &author, const QString &text, qint64 timestamp)
    : m_title(title)
    , m_author(author)
    , m_text(text)
    , m_timestamp(timestamp)
{}

QString Comment::title() const
{
    return m_title;
}

void Comment::setTitle(const QString &title)
{
    m_title = title;
}

QString Comment::author() const
{
    return m_author;
}

void Comment::setAuthor(const QString &author)
{
    m_author = author;
}

QString Comment::text() const
{
    return m_text;
}

void Comment::setText(const QString &text)
{
    m_text = text;
}

QString Comment::deescapedText() const
{
    QString result = m_text;

    result.replace(QStringLiteral("*\\/"), QStringLiteral("*/"));
    result.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    result.replace(QStringLiteral("\\r"), QStringLiteral("\r"));
    result.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
    result.replace(QStringLiteral("\\\""), QStringLiteral("\""));
    result.replace(QStringLiteral("\\\'"), QStringLiteral("\'"));
    result.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));

    return result;
}

QString Comment::timestampStr() const
{
    return QDateTime::fromSecsSinceEpoch(m_timestamp).toString();
}

QString Comment::timestampStr(const QString &format) const
{
    return QDateTime::fromSecsSinceEpoch(m_timestamp).toString(format);
}

qint64 Comment::timestamp() const
{
    return m_timestamp;
}

void Comment::setTimestamp(qint64 timestamp)
{
    m_timestamp = timestamp;
}

void Comment::updateTimestamp()
{
    m_timestamp = QDateTime::currentSecsSinceEpoch();
}

bool Comment::sameContent(const Comment &comment) const
{
    return sameContent(*this, comment);
}

bool Comment::sameContent(const Comment &a, const Comment &b)
{
    return ((a.title() == b.title())
            && (a.author() == b.author())
            && (a.text() == b.text()));
}

bool Comment::operator==(const Comment &comment) const
{
    return (sameContent(comment) && (m_timestamp == comment.timestamp()));
}

bool Comment::isEmpty()
{
    return sameContent(Comment());
}

QString Comment::toQString() const
{
    QStringList result;

    result.push_back(m_title);
    result.push_back(m_author);
    result.push_back(m_text);
    result.push_back(QString::number(m_timestamp));

    return result.join(s_sep);
}

QDebug &operator<<(QDebug &stream, const Comment &comment)
{
    stream << "\"title: " << comment.m_title << "\" ";
    stream << "\"author: " << comment.m_author << "\" ";
    stream << "\"text: " << comment.m_text << "\" ";
    stream << "\"timestamp: " << comment.m_timestamp << "\" ";
    stream << "\"date/time: " << QDateTime::fromSecsSinceEpoch(comment.m_timestamp).toString() << "\" ";

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const Comment &comment)
{
    stream << comment.m_title;
    stream << comment.m_author;
    stream << comment.m_text;
    stream << comment.m_timestamp;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Comment &comment)
{
    stream >> comment.m_title;
    stream >> comment.m_author;
    stream >> comment.m_text;
    stream >> comment.m_timestamp;

    return stream;
}


//Annotation

Annotation::Annotation()
    : m_comments()
{

}

QVector<Comment> Annotation::comments() const
{
    return m_comments;
}


bool Annotation::hasComments() const
{
    return !m_comments.isEmpty();
}

void Annotation::setComments(const QVector<Comment> &comments)
{
    m_comments = comments;
}

void Annotation::removeComments()
{
    m_comments.clear();
}

int Annotation::commentsSize() const
{
    return m_comments.size();
}

Comment Annotation::comment(int n) const
{
    if (m_comments.size() > n)
        return m_comments.at(n);
    else
        return Comment();
}

void Annotation::addComment(const Comment &comment)
{
    m_comments.push_back(comment);
}

bool Annotation::updateComment(const Comment &comment, int n)
{
    bool result = false;

    if ((m_comments.size() > n) && (n > 0)) {
        m_comments[n] = comment;
        result = true;
    }

    return result;
}

bool Annotation::removeComment(int n)
{
    bool result = false;

    if (m_comments.size() > n) {
        m_comments.remove(n);
        result = true;
    }

    return result;
}

QString Annotation::toQString() const
{
    QStringList result;

    result.push_back(QString::number(m_comments.size()));

    for (const Comment &com : m_comments)
        result.push_back(com.toQString());

    return result.join(s_sep);
}

void Annotation::fromQString(const QString &str)
{
    QStringList strl (str.split(s_sep, QString::SplitBehavior::KeepEmptyParts));
    removeComments();

    const int intro = 1;
    const int comSize = 4;

    if (!strl.isEmpty()) {

        if (strl.size() >= intro) {

            int size = strl.at(0).toInt();

            if (size > 0) {
                if (strl.size() == (size*comSize) + intro)
                {
                    for (int i = 0; i < size; i++)
                    {
                        const int offset = intro + (i * comSize);
                        Comment com;
                        com.setTitle(strl.at(offset + 0));
                        com.setAuthor(strl.at(offset + 1));
                        com.setText(strl.at(offset + 2));
                        com.setTimestamp(strl.at(offset + 3).toLongLong());

                        m_comments.push_back(com);
                    }
                }
            }
        }
    }
}

QDebug &operator<<(QDebug &stream, const Annotation &annotation)
{
    stream << "\"Annotation: " << annotation.m_comments << "\" ";

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const Annotation &annotation)
{
    stream << annotation.m_comments;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Annotation &annotation)
{
    stream >> annotation.m_comments;

    return stream;
}

GlobalAnnotationStatus::GlobalAnnotationStatus()
    : m_status(GlobalAnnotationStatus::Status::NoStatus)
{ }

GlobalAnnotationStatus::GlobalAnnotationStatus(GlobalAnnotationStatus::Status status)
    : m_status(status)
{ }

void GlobalAnnotationStatus::setStatus(int statusId)
{
    switch (statusId) {
    case 0: m_status = GlobalAnnotationStatus::Status::InProgress; break;
    case 1: m_status = GlobalAnnotationStatus::Status::InReview; break;
    case 2: m_status = GlobalAnnotationStatus::Status::Done; break;
    case -1:
    default: m_status = GlobalAnnotationStatus::Status::NoStatus; break;
    }
}

void GlobalAnnotationStatus::setStatus(GlobalAnnotationStatus::Status status)
{
    m_status = status;
}

GlobalAnnotationStatus::Status GlobalAnnotationStatus::status() const
{
    return m_status;
}

QString GlobalAnnotationStatus::toQString() const
{
    return QString::number(static_cast<int>(m_status));
}

void GlobalAnnotationStatus::fromQString(const QString &str)
{
    bool result = false;
    int conversion = str.toInt(&result);

    if (result) {
        setStatus(conversion);
    }
    else {
        m_status = GlobalAnnotationStatus::Status::NoStatus;
    }
}

} // QmlDesigner namespace
