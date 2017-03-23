/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlerror.h"


#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvector.h>



QT_BEGIN_NAMESPACE

/*!
    \class QmlError
    \since 5.0
    \inmodule QtQml
    \brief The QmlError class encapsulates a QML error.

    QmlError includes a textual description of the error, as well
    as location information (the file, line, and column). The toString()
    method creates a single-line, human-readable string containing all of
    this information, for example:
    \code
    file:///home/user/test.qml:7:8: Invalid property assignment: double expected
    \endcode

    You can use qDebug(), qInfo(), or qWarning() to output errors to the console.
    This method will attempt to open the file indicated by the error
    and include additional contextual information.
    \code
    file:///home/user/test.qml:7:8: Invalid property assignment: double expected
            y: "hello"
               ^
    \endcode

    Note that the \l {Qt Quick 1} version is named QDeclarativeError

    \sa QQuickView::errors(), QmlComponent::errors()
*/

static quint16 qmlSourceCoordinate(int n)
{
    return (n > 0 && n <= static_cast<int>(USHRT_MAX)) ? static_cast<quint16>(n) : 0;
}

class QmlErrorPrivate
{
public:
    QmlErrorPrivate();

    QUrl url;
    QString description;
    quint16 line;
    quint16 column;
    QtMsgType messageType;
    QObject *object;
};

QmlErrorPrivate::QmlErrorPrivate()
: line(0), column(0), messageType(QtMsgType::QtWarningMsg), object()
{
}

/*!
    Creates an empty error object.
*/
QmlError::QmlError()
: d(0)
{
}

/*!
    Creates a copy of \a other.
*/
QmlError::QmlError(const QmlError &other)
: d(0)
{
    *this = other;
}

/*!
    Assigns \a other to this error object.
*/
QmlError &QmlError::operator=(const QmlError &other)
{
    if (!other.d) {
        delete d;
        d = 0;
    } else {
        if (!d)
            d = new QmlErrorPrivate;
        d->url = other.d->url;
        d->description = other.d->description;
        d->line = other.d->line;
        d->column = other.d->column;
        d->object = other.d->object;
        d->messageType = other.d->messageType;
    }
    return *this;
}

/*!
    \internal
*/
QmlError::~QmlError()
{
    delete d; d = 0;
}

/*!
    Returns true if this error is valid, otherwise false.
*/
bool QmlError::isValid() const
{
    return d != 0;
}

/*!
    Returns the url for the file that caused this error.
*/
QUrl QmlError::url() const
{
    if (d)
        return d->url;
    return QUrl();
}

/*!
    Sets the \a url for the file that caused this error.
*/
void QmlError::setUrl(const QUrl &url)
{
    if (!d)
        d = new QmlErrorPrivate;
    d->url = url;
}

/*!
    Returns the error description.
*/
QString QmlError::description() const
{
    if (d)
        return d->description;
    return QString();
}

/*!
    Sets the error \a description.
*/
void QmlError::setDescription(const QString &description)
{
    if (!d)
        d = new QmlErrorPrivate;
    d->description = description;
}

/*!
    Returns the error line number.
*/
int QmlError::line() const
{
    if (d)
        return qmlSourceCoordinate(d->line);
    return -1;
}

/*!
    Sets the error \a line number.
*/
void QmlError::setLine(int line)
{
    if (!d)
        d = new QmlErrorPrivate;
    d->line = qmlSourceCoordinate(line);
}

/*!
    Returns the error column number.
*/
int QmlError::column() const
{
    if (d)
        return qmlSourceCoordinate(d->column);
    return -1;
}

/*!
    Sets the error \a column number.
*/
void QmlError::setColumn(int column)
{
    if (!d)
        d = new QmlErrorPrivate;
    d->column = qmlSourceCoordinate(column);
}

/*!
    Returns the nearest object where this error occurred.
    Exceptions in bound property expressions set this to the object
    to which the property belongs. It will be 0 for all
    other exceptions.
 */
QObject *QmlError::object() const
{
    if (d)
        return d->object;
    return 0;
}

/*!
    Sets the nearest \a object where this error occurred.
 */
void QmlError::setObject(QObject *object)
{
    if (!d)
        d = new QmlErrorPrivate;
    d->object = object;
}

/*!
    \since 5.9

    Returns the message type.
 */
QtMsgType QmlError::messageType() const
{
    if (d)
        return d->messageType;
    return QtMsgType::QtWarningMsg;
}

/*!
    \since 5.9

    Sets the \a messageType for this message. The message type determines which
    QDebug handlers are responsible for recieving the message.
 */
void QmlError::setMessageType(QtMsgType messageType)
{
    if (!d)
        d = new QmlErrorPrivate;
    d->messageType = messageType;
}

/*!
    Returns the error as a human readable string.
*/
QString QmlError::toString() const
{
    QString rv;

    QUrl u(url());
    int l(line());

    if (u.isEmpty() || (u.isLocalFile() && u.path().isEmpty()))
        rv += QLatin1String("<Unknown File>");
    else
        rv += u.toString();

    if (l != -1) {
        rv += QLatin1Char(':') + QString::number(l);

        int c(column());
        if (c != -1)
            rv += QLatin1Char(':') + QString::number(c);
    }

    rv += QLatin1String(": ") + description();

    return rv;
}

/*!
    \relates QmlError
    \fn QDebug operator<<(QDebug debug, const QmlError &error)

    Outputs a human readable version of \a error to \a debug.
*/

QDebug operator<<(QDebug debug, const QmlError &error)
{
    debug << qPrintable(error.toString());

    QUrl url = error.url();

    if (error.line() > 0 && url.scheme() == QLatin1String("file")) {
        QString file = url.toLocalFile();
        QFile f(file);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            QTextStream stream(data, QIODevice::ReadOnly);
            stream.setCodec("UTF-8");
            const QString code = stream.readAll();
            const auto lines = code.splitRef(QLatin1Char('\n'));

            if (lines.count() >= error.line()) {
                const QStringRef &line = lines.at(error.line() - 1);
                debug << "\n    " << line.toLocal8Bit().constData();

                if (error.column() > 0) {
                    int column = qMax(0, error.column() - 1);
                    column = qMin(column, line.length());

                    QByteArray ind;
                    ind.reserve(column);
                    for (int i = 0; i < column; ++i) {
                        const QChar ch = line.at(i);
                        if (ch.isSpace())
                            ind.append(ch.unicode());
                        else
                            ind.append(' ');
                    }
                    ind.append('^');
                    debug << "\n    " << ind.constData();
                }
            }
        }
    }
    return debug;
}

QT_END_NAMESPACE
