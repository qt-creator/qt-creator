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

#include "qmljsutils.h"

#include "parser/qmljsast_p.h"

#include <QColor>
#include <QDir>
#include <QRegularExpression>

using namespace QmlJS;
using namespace QmlJS::AST;

/*!
  \namespace QmlJS
  QML and JavaScript language support library
*/

namespace {
class SharedData
{
public:
    SharedData()
    {
        validBuiltinPropertyNames.insert(QLatin1String("action"));
        validBuiltinPropertyNames.insert(QLatin1String("bool"));
        validBuiltinPropertyNames.insert(QLatin1String("color"));
        validBuiltinPropertyNames.insert(QLatin1String("date"));
        validBuiltinPropertyNames.insert(QLatin1String("double"));
        validBuiltinPropertyNames.insert(QLatin1String("enumeration"));
        validBuiltinPropertyNames.insert(QLatin1String("font"));
        validBuiltinPropertyNames.insert(QLatin1String("int"));
        validBuiltinPropertyNames.insert(QLatin1String("list"));
        validBuiltinPropertyNames.insert(QLatin1String("point"));
        validBuiltinPropertyNames.insert(QLatin1String("real"));
        validBuiltinPropertyNames.insert(QLatin1String("rect"));
        validBuiltinPropertyNames.insert(QLatin1String("size"));
        validBuiltinPropertyNames.insert(QLatin1String("string"));
        validBuiltinPropertyNames.insert(QLatin1String("time"));
        validBuiltinPropertyNames.insert(QLatin1String("url"));
        validBuiltinPropertyNames.insert(QLatin1String("var"));
        validBuiltinPropertyNames.insert(QLatin1String("variant")); // obsolete in Qt 5
        validBuiltinPropertyNames.insert(QLatin1String("vector2d"));
        validBuiltinPropertyNames.insert(QLatin1String("vector3d"));
        validBuiltinPropertyNames.insert(QLatin1String("vector4d"));
        validBuiltinPropertyNames.insert(QLatin1String("quaternion"));
        validBuiltinPropertyNames.insert(QLatin1String("matrix4x4"));
        validBuiltinPropertyNames.insert(QLatin1String("alias"));
    }

    QSet<QString> validBuiltinPropertyNames;
};
} // anonymous namespace
Q_GLOBAL_STATIC(SharedData, sharedData)

QColor QmlJS::toQColor(const QString &qmlColorString)
{
    QColor color;
    if (qmlColorString.size() == 9 && qmlColorString.at(0) == QLatin1Char('#')) {
        bool ok;
        const int alpha = qmlColorString.midRef(1, 2).toInt(&ok, 16);
        if (ok) {
            const QString name = qmlColorString.at(0) + qmlColorString.right(6);
            if (QColor::isValidColor(name)) {
                color.setNamedColor(name);
                color.setAlpha(alpha);
            }
        }
    } else {
        if (QColor::isValidColor(qmlColorString))
            color.setNamedColor(qmlColorString);
    }
    return color;
}

QString QmlJS::toString(UiQualifiedId *qualifiedId, QChar delimiter)
{
    QString result;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter != qualifiedId)
            result += delimiter;

        result += iter->name;
    }

    return result;
}


SourceLocation QmlJS::locationFromRange(const SourceLocation &start,
                                        const SourceLocation &end)
{
    return SourceLocation(start.offset,
                          end.end() - start.begin(),
                          start.startLine,
                          start.startColumn);
}

SourceLocation QmlJS::fullLocationForQualifiedId(AST::UiQualifiedId *qualifiedId)
{
    SourceLocation start = qualifiedId->identifierToken;
    SourceLocation end = qualifiedId->identifierToken;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter->identifierToken.isValid())
            end = iter->identifierToken;
    }

    return locationFromRange(start, end);
}

/*!
    \returns the value of the 'id:' binding in \a object
    \param idBinding optional out parameter to get the UiScriptBinding for the id binding
*/
QString QmlJS::idOfObject(Node *object, UiScriptBinding **idBinding)
{
    if (idBinding)
        *idBinding = 0;

    UiObjectInitializer *initializer = initializerOfObject(object);
    if (!initializer) {
        initializer = cast<UiObjectInitializer *>(object);
        if (!initializer)
            return QString();
    }

    for (UiObjectMemberList *iter = initializer->members; iter; iter = iter->next) {
        if (UiScriptBinding *script = cast<UiScriptBinding*>(iter->member)) {
            if (!script->qualifiedId)
                continue;
            if (script->qualifiedId->next)
                continue;
            if (script->qualifiedId->name != QLatin1String("id"))
                continue;
            if (ExpressionStatement *expstmt = cast<ExpressionStatement *>(script->statement)) {
                if (IdentifierExpression *idexp = cast<IdentifierExpression *>(expstmt->expression)) {
                    if (idBinding)
                        *idBinding = script;
                    return idexp->name.toString();
                }
            }
        }
    }

    return QString();
}

/*!
    \returns the UiObjectInitializer if \a object is a UiObjectDefinition or UiObjectBinding, otherwise 0
*/
UiObjectInitializer *QmlJS::initializerOfObject(Node *object)
{
    if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(object))
        return definition->initializer;
    if (UiObjectBinding *binding = cast<UiObjectBinding *>(object))
        return binding->initializer;
    return 0;
}

UiQualifiedId *QmlJS::qualifiedTypeNameId(Node *node)
{
    if (UiObjectBinding *binding = AST::cast<UiObjectBinding *>(node))
        return binding->qualifiedTypeNameId;
    else if (UiObjectDefinition *binding = AST::cast<UiObjectDefinition *>(node))
        return binding->qualifiedTypeNameId;
    return 0;
}

DiagnosticMessage QmlJS::errorMessage(const AST::SourceLocation &loc, const QString &message)
{
    return DiagnosticMessage(Severity::Error, loc, message);
}

namespace {
const QString undefinedVersion = QLatin1String("-1.-1");
}

/*!
 * \brief Permissive validation of a string representing a module version.
 * \param version
 * \return True if \p version is a valid version format (<digit(s)>.<digit(s)>), if it is the
 *         undefined version (-1.-1) or if it is empty.  False otherwise.
 */
bool QmlJS::maybeModuleVersion(const QString &version) {
    QRegularExpression re(QLatin1String("^\\d+\\.\\d+$"));
    return version.isEmpty() || version == undefinedVersion || re.match(version).hasMatch();
}

/*!
 * \brief Get the path of a module
 * \param name
 * \param version
 * \param importPaths
 *
 * Given the qualified \p name and \p version of a module, look for a valid path in \p importPaths.
 * Most specific version are searched first, the version is searched also in parent modules.
 * For example, given the \p name QtQml.Models and \p version 2.0, the following directories are
 * searched in every element of \p importPath:
 *
 * - QtQml/Models.2.0
 * - QtQml.2.0/Models
 * - QtQml/Models.2
 * - QtQml.2/Models
 * - QtQml/Models
 *
 * \return The module paths if found, an empty string otherwise
 * \see qmlimportscanner in qtdeclarative/tools
 */
QString QmlJS::modulePath(const QString &name, const QString &version,
                          const QStringList &importPaths)
{
    Q_ASSERT(maybeModuleVersion(version));
    if (importPaths.isEmpty())
        return QString();

    const QString sanitizedVersion = version == undefinedVersion ? QString() : version;
    const QStringList parts = name.split(QLatin1Char('.'), QString::SkipEmptyParts);
    auto mkpath = [] (const QStringList &xs) -> QString { return xs.join(QLatin1Char('/')); };

    // Regular expression for building candidates by successively removing minor and major
    // version numbers.  It does not match the undefined version, so it has to be applied to the
    // sanitized version.
    const QRegularExpression re("\\.?\\d+$");

    QString candidate;

    for (QString ver = sanitizedVersion; !ver.isEmpty(); ver.remove(re)) {
        for (const QString &path: importPaths) {
            for (int i = parts.count() - 1; i >= 0; --i) {
                candidate = QDir::cleanPath(
                            QString::fromLatin1("%1/%2.%3/%4").arg(path,
                                                                   mkpath(parts.mid(0, i + 1)),
                                                                   ver,
                                                                   mkpath(parts.mid(i + 1))));
                if (QDir(candidate).exists())
                    return candidate;
            }
        }
    }

    // Version is empty
    for (const QString &path: importPaths) {
        candidate = QDir::cleanPath(QString::fromLatin1("%1/%2").arg(path, mkpath(parts)));
        if (QDir(candidate).exists())
            return candidate;
    }

    return QString();
}

bool QmlJS::isValidBuiltinPropertyType(const QString &name)
{
    return sharedData()->validBuiltinPropertyNames.contains(name);
}

