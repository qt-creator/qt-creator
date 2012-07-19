/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qmldirparser_p.h"
#include "qmlerror.h"
bool Qml_isFileCaseCorrect(const QString &) { return true; }

#include <QTextStream>
#include <QFile>
#include <QtDebug>

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

QString QmlDirParser::fileSource() const
{
    return _filePathSouce;
}

void QmlDirParser::setFileSource(const QString &filePath)
{
    _filePathSouce = filePath;
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

    if (_source.isEmpty() && !_filePathSouce.isEmpty()) {
        QFile file(_filePathSouce);
        if (!Qml_isFileCaseCorrect(_filePathSouce)) {
            QmlError error;
            error.setDescription(QString::fromUtf8("cannot load module \"$$URI$$\": File name case mismatch for \"%1\"").arg(_filePathSouce));
            _errors.prepend(error);
            return false;
        } else if (file.open(QFile::ReadOnly)) {
            _source = QString::fromUtf8(file.readAll());
        } else {
            QmlError error;
            error.setDescription(QString::fromUtf8("module \"$$URI$$\" definition \"%1\" not readable").arg(_filePathSouce));
            _errors.prepend(error);
            return false;
        }
    }

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
                            QString::fromUtf8("plugin directive requires one or two arguments, but %1 were provided").arg(sectionCount - 1));

                continue;
            }

            const Plugin entry(sections[1], sections[2]);

            _plugins.append(entry);

        } else if (sections[0] == QLatin1String("internal")) {
            if (sectionCount != 3) {
                reportError(lineNumber, -1,
                            QString::fromUtf8("internal types require 2 arguments, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
            Component entry(sections[1], sections[2], -1, -1);
            entry.internal = true;
            _components.append(entry);
        } else if (sections[0] == QLatin1String("typeinfo")) {
            if (sectionCount != 2) {
                reportError(lineNumber, -1,
                            QString::fromUtf8("typeinfo requires 1 argument, but %1 were provided").arg(sectionCount - 1));
                continue;
            }
#ifdef QT_CREATOR
            TypeInfo typeInfo(sections[1]);
            _typeInfos.append(typeInfo);
#endif

        } else if (sectionCount == 2) {
            // No version specified (should only be used for relative qmldir files)
            const Component entry(sections[0], sections[1], -1, -1);
            _components.append(entry);
        } else if (sectionCount == 3) {
            const QString &version = sections[1];
            const int dotIndex = version.indexOf(QLatin1Char('.'));

            if (dotIndex == -1) {
                reportError(lineNumber, -1, QLatin1String("expected '.'"));
            } else if (version.indexOf(QLatin1Char('.'), dotIndex + 1) != -1) {
                reportError(lineNumber, -1, QLatin1String("unexpected '.'"));
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
            reportError(lineNumber, -1, 
                        QString::fromUtf8("a component declaration requires two or three arguments, but %1 were provided").arg(sectionCount));
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

QList<QmlError> QmlDirParser::errors(const QString &uri) const
{
    QList<QmlError> errors = _errors;
    for (int i = 0; i < errors.size(); ++i) {
        QmlError &e = errors[i];
        QString description = e.description();
        description.replace(QLatin1String("$$URI$$"), uri);
        e.setDescription(description);
    }
    return errors;
}

QList<QmlDirParser::Plugin> QmlDirParser::plugins() const
{
    return _plugins;
}

QList<QmlDirParser::Component> QmlDirParser::components() const
{
    return _components;
}

#ifdef QT_CREATOR
QList<QmlDirParser::TypeInfo> QmlDirParser::typeInfos() const
{
    return _typeInfos;
}
#endif

QT_END_NAMESPACE
