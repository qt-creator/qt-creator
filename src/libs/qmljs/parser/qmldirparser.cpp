/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmldirparser_p.h"
#include "qmlerror.h"

#include <QtCore/QTextStream>
#include <QtCore/QtDebug>

QT_BEGIN_NAMESPACE

QmlDirParser::QmlDirParser()
    : _isParsed(false)
{
}

QmlDirParser::~QmlDirParser()
{
}

QUrl QmlDirParser::url() const
{
    return _url;
}

void QmlDirParser::setUrl(const QUrl &url)
{
    _url = url;
}

QString QmlDirParser::source() const
{
    return _source;
}

void QmlDirParser::setSource(const QString &source)
{
    _isParsed = false;
    _source = source;
}

bool QmlDirParser::isParsed() const
{
    return _isParsed;
}

bool QmlDirParser::parse()
{
    if (_isParsed)
        return true;

    _isParsed = true;
    _errors.clear();
    _plugins.clear();
    _components.clear();

    QTextStream stream(&_source);
    int lineNumber = 0;

    forever {
        ++lineNumber;

        const QString line = stream.readLine();
        if (line.isNull())
            break;

        QString sections[3];
        int sectionCount = 0;

        int index = 0;
        const int length = line.length();

        while (index != length) {
            const QChar ch = line.at(index);

            if (ch.isSpace()) {
                do { ++index; }
                while (index != length && line.at(index).isSpace());

            } else if (ch == QLatin1Char('#')) {
                // recognized a comment
                break;

            } else {
                const int start = index;

                do { ++index; }
                while (index != length && !line.at(index).isSpace());

                const QString lexeme = line.mid(start, index - start);

                if (sectionCount >= 3) {
                    reportError(lineNumber, start, QLatin1String("unexpected token"));

                } else {
                    sections[sectionCount++] = lexeme;
                }
            }
        }

        if (sectionCount == 0) {
            continue; // no sections, no party.

        } else if (sections[0] == QLatin1String("plugin")) {
            if (sectionCount < 2) {
                reportError(lineNumber, -1,
                            QString::fromUtf8("plugin directive requires 2 arguments, but %1 were provided").arg(sectionCount + 1));

                continue;
            }

            const Plugin entry(sections[1], sections[2]);

            _plugins.append(entry);

        } else if (sectionCount == 3) {
            const QString &version = sections[1];
            const int dotIndex = version.indexOf(QLatin1Char('.'));

            if (dotIndex == -1) {
                qWarning() << "expected '.'"; // ### use reportError

            } else if (version.indexOf(QLatin1Char('.'), dotIndex + 1) != -1) {
                qWarning() << "unexpected '.'"; // ### use reportError

            } else {
                bool validVersionNumber = false;
                const int majorVersion = version.left(dotIndex).toInt(&validVersionNumber);

                if (validVersionNumber) {
                    const int minorVersion = version.mid(dotIndex + 1).toInt(&validVersionNumber);

                    if (validVersionNumber) {
                        const Component entry(sections[0], sections[2], majorVersion, minorVersion);

                        _components.append(entry);
                    }
                }
            }
        } else {
             // ### use reportError
            qWarning() << "a component declaration requires 3 arguments, but" << (sectionCount + 1) << "were provided";
        }
    }

    return hasError();
}

void QmlDirParser::reportError(int line, int column, const QString &description)
{
    QmlError error;
    error.setUrl(_url);
    error.setLine(line);
    error.setColumn(column);
    error.setDescription(description);
    _errors.append(error);
}

bool QmlDirParser::hasError() const
{
    if (! _errors.isEmpty())
        return true;

    return false;
}

QList<QmlError> QmlDirParser::errors() const
{
    return _errors;
}

QList<QmlDirParser::Plugin> QmlDirParser::plugins() const
{
    return _plugins;
}

QList<QmlDirParser::Component> QmlDirParser::components() const
{
    return _components;
}

QT_END_NAMESPACE
