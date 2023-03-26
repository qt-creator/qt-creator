// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegeneration.h"

#include "algorithm.h"

#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <QTextStream>

namespace Utils {

QTCREATOR_UTILS_EXPORT QString fileNameToCppIdentifier(const QString &s)
{
    QString rc;
    const int len = s.size();
    const QChar underscore =  QLatin1Char('_');
    const QChar dot =  QLatin1Char('.');

    for (int i = 0; i < len; i++) {
        const QChar c = s.at(i);
        if (c == underscore || c.isLetterOrNumber())
            rc += c;
        else if (c == dot)
            rc += underscore;
    }
    return rc;
}

QTCREATOR_UTILS_EXPORT QString headerGuard(const QString &file)
{
    return headerGuard(file, QStringList());
}

QTCREATOR_UTILS_EXPORT QString headerGuard(const QString &file, const QStringList &namespaceList)
{
    const QChar underscore = QLatin1Char('_');
    QString rc;
    for (int i = 0; i < namespaceList.count(); i++)
        rc += namespaceList.at(i).toUpper() + underscore;

    const QFileInfo fi(file);
    rc += fileNameToCppIdentifier(fi.fileName()).toUpper();
    return rc;
}

QTCREATOR_UTILS_EXPORT
void writeIncludeFileDirective(const QString &file, bool globalInclude,
                               QTextStream &str)
{
    const QChar opening = globalInclude ?  QLatin1Char('<') : QLatin1Char('"');
    const QChar closing = globalInclude ?  QLatin1Char('>') : QLatin1Char('"');
    str << QLatin1String("#include ") << opening << file <<  closing << QLatin1Char('\n');
}

QTCREATOR_UTILS_EXPORT void writeBeginQtVersionCheck(QTextStream &str)
{
    str << QLatin1String("#if QT_VERSION >= 0x050000\n");
}

static void qtSection(const QStringList &qtIncludes, QTextStream &str)
{
    const QStringList sorted = Utils::sorted(qtIncludes);
    for (const QString &inc : sorted) {
        if (!inc.isEmpty())
            str << QStringLiteral("#include <%1>\n").arg(inc);
    }
}

QTCREATOR_UTILS_EXPORT
void writeQtIncludeSection(const QStringList &qt4,
                           const QStringList &qt5,
                           bool addQtVersionCheck,
                           bool includeQtModule,
                           QTextStream &str)
{
    std::function<QString(const QString &)> trans;
    if (includeQtModule)
        trans = [](const QString &i) { return i; };
    else
        trans = [](const QString &i) { return i.mid(i.indexOf(QLatin1Char('/')) + 1); };

    QSet<QString> qt4Only = transform<QSet>(qt4, trans);
    QSet<QString> qt5Only = transform<QSet>(qt5, trans);

    if (addQtVersionCheck) {
        QSet<QString> common = qt4Only;
        common.intersect(qt5Only);

        // qglobal.h is needed for QT_VERSION
        if (includeQtModule)
            common.insert(QLatin1String("QtCore/qglobal.h"));
        else
            common.insert(QLatin1String("qglobal.h"));

        qt4Only.subtract(common);
        qt5Only.subtract(common);

        qtSection(toList(common), str);

        if (!qt4Only.isEmpty() || !qt5Only.isEmpty()) {
            if (addQtVersionCheck)
                writeBeginQtVersionCheck(str);
            qtSection(toList(qt5Only), str);
            if (addQtVersionCheck)
                str << QLatin1String("#else\n");
            qtSection(toList(qt4Only), str);
            if (addQtVersionCheck)
                str << QLatin1String("#endif\n");
        }
    } else {
        if (!qt5Only.isEmpty()) // default to Qt5
            qtSection(toList(qt5Only), str);
        else
            qtSection(toList(qt4Only), str);
    }
}

QTCREATOR_UTILS_EXPORT
QString writeOpeningNameSpaces(const QStringList &l, const QString &indent,
                               QTextStream &str)
{
    const int count = l.size();
    QString rc;
    if (count) {
        str << '\n';
        for (int i = 0; i < count; i++) {
            str << rc << "namespace " << l.at(i) << " {\n";
            rc += indent;
        }
    }
    return rc;
}

QTCREATOR_UTILS_EXPORT
void writeClosingNameSpaces(const QStringList &l, const QString &indent,
                            QTextStream &str)
{
    if (!l.empty())
        str << '\n';
    for (int i = l.size() - 1; i >= 0; i--) {
        if (i)
            str << QString(indent.size() * i, QLatin1Char(' '));
        str << "} // namespace " <<  l.at(i) << '\n';
    }
}

} // namespace Utils
