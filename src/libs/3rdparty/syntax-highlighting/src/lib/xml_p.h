/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_XML_P_H
#define KSYNTAXHIGHLIGHTING_XML_P_H

#include <QString>

namespace KSyntaxHighlighting
{
/** Utilities for XML parsing. */
namespace Xml
{
/** Parse a xs:boolean attribute. */
inline bool attrToBool(const QStringRef &str)
{
    return str == QLatin1String("1") || str.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
}

}
}

#endif
