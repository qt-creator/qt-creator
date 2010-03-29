/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "protocol.h"

#include <cpptools/cpptoolsconstants.h>
#include <qmljseditor/qmljseditorconstants.h>

namespace CodePaster {

Protocol::Protocol()
        : QObject()
{
}

Protocol::~Protocol()
{
}

bool Protocol::canFetch() const
{
    return true;
}

bool Protocol::canPost() const
{
    return true;
}

bool Protocol::hasSettings() const
{
    return false;
}

Core::IOptionsPage *Protocol::settingsPage()
{
    return 0;
}

void Protocol::list()
{
    qFatal("Base Protocol list() called");
}

Protocol::ContentType Protocol::contentType(const QString &mt)
{
    if (mt  == QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
        || mt == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE))
        return C;
    if (mt == QLatin1String(QmlJSEditor::Constants::QML_MIMETYPE)
        || mt == QLatin1String(QmlJSEditor::Constants::JS_MIMETYPE))
        return JavaScript;
    if (mt == QLatin1String("text/x-patch"))
        return Diff;
    if (mt == QLatin1String("text/xml") || mt == QLatin1String("application/xml"))
        return Xml;
    return Text;
}

QString Protocol::fixNewLines(QString data)
{
    // Copied from cpaster. Otherwise lineendings will screw up
    // HTML requires "\r\n".
    if (data.contains(QLatin1String("\r\n")))
        return data;
    if (data.contains(QLatin1Char('\n'))) {
        data.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
        return data;
    }
    if (data.contains(QLatin1Char('\r')))
        data.replace(QLatin1Char('\r'), QLatin1String("\r\n"));
    return data;
}

QString Protocol::textFromHtml(QString data)
{
    data.remove(QLatin1Char('\r'));
    data.replace(QLatin1String("&lt;"), QString(QLatin1Char('<')));
    data.replace(QLatin1String("&gt;"), QString(QLatin1Char('>')));
    data.replace(QLatin1String("&amp;"), QString(QLatin1Char('&')));
    return data;
}
} //namespace CodePaster
