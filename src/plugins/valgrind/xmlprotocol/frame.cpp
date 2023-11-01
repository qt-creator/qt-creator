// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "frame.h"

#include "../valgrindtr.h"

#include <QList>
#include <QPair>
#include <QString>

#include <algorithm>

namespace Valgrind::XmlProtocol {

class Frame::Private : public QSharedData
{
public:
    bool operator==(const Private &other) const
    {
        return ip == other.ip
                && object == other.object
                && functionName == other.functionName
                && fileName == other.fileName
                && directory == other.directory
                && line == other.line;
    }

    quint64 ip = 0;
    QString object;
    QString functionName;
    QString fileName;
    QString directory;
    int line = -1;
};

Frame::Frame() : d(new Private)
{
}

Frame::~Frame() = default;

Frame::Frame(const Frame &other) = default;

Frame &Frame::operator=(const Frame &other)
{
    Frame tmp(other);
    swap(tmp);
    return *this;
}

bool Frame::operator==( const Frame &other ) const
{
    return *d == *other.d;
}

bool Frame::operator!=(const Frame &other) const
{
    return !(*this == other);
}

void Frame::swap(Frame &other)
{
    std::swap(d, other.d);
}

quint64 Frame::instructionPointer() const
{
    return d->ip;
}

void Frame::setInstructionPointer(quint64 ip)
{
    d->ip = ip;
}

QString Frame::object() const
{
    return d->object;
}

void Frame::setObject(const QString &obj)
{
    d->object = obj;
}

QString Frame::functionName() const
{
    return d->functionName;
}

void Frame::setFunctionName(const QString &functionName)
{
    d->functionName = functionName;
}

QString Frame::fileName() const
{
    return d->fileName;
}

void Frame::setFileName(const QString &file)
{
    d->fileName = file;
}

QString Frame::directory() const
{
    return d->directory;
}

void Frame::setDirectory(const QString &directory)
{
    d->directory = directory;
}

QString Frame::filePath() const
{
    QString f;
    if (!directory().isEmpty())
        f.append(directory()).append('/');
    return f.append(fileName());
}

int Frame::line() const
{
    return d->line;
}

void Frame::setLine(int line)
{
    d->line = line;
}

QString Frame::toolTip() const
{
    QString location;
    if (!fileName().isEmpty()) {
        location = filePath();
        if (line() > 0)
            location += ':' + QString::number(line());
    }

    using StringPair = QPair<QString, QString>;
    QList<StringPair> lines;

    if (!functionName().isEmpty())
        lines.append({Tr::tr("Function:"), functionName()});
    if (!location.isEmpty())
        lines.append({Tr::tr("Location:"), location});
    if (instructionPointer()) {
        lines.append({Tr::tr("Instruction pointer:"),
                      QString("0x%1").arg(instructionPointer(), 0, 16)});
    }
    if (!object().isEmpty())
        lines.append({Tr::tr("Object:"), object()});

    QString html = "<html><head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "</head><body><dl>";

    for (const StringPair &pair : std::as_const(lines)) {
        html += "<dt>";
        html += pair.first;
        html += "</dt><dd>";
        html += pair.second;
        html += "</dd>\n";
    }
    html += "</dl></body></html>";
    return html;
}

} // namespace Valgrind::XmlProtocol
