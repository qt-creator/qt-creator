// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsutils.h"

#include "parser/qmljsast_p.h"

#include <utils/filepath.h>
#include <utils/stringutils.h>

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
        const int alpha = qmlColorString.mid(1, 2).toInt(&ok, 16);
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

        result += iter->name.toString();
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
    Returns the value of the 'id:' binding in \a object.

    \a idBinding is optional out parameter to get the UiScriptBinding for the id binding.
*/
QString QmlJS::idOfObject(Node *object, UiScriptBinding **idBinding)
{
    if (idBinding)
        *idBinding = nullptr;

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
    return nullptr;
}

UiQualifiedId *QmlJS::qualifiedTypeNameId(Node *node)
{
    if (UiObjectBinding *binding = AST::cast<UiObjectBinding *>(node))
        return binding->qualifiedTypeNameId;
    else if (UiObjectDefinition *binding = AST::cast<UiObjectDefinition *>(node))
        return binding->qualifiedTypeNameId;
    return nullptr;
}

DiagnosticMessage QmlJS::errorMessage(const SourceLocation &loc, const QString &message)
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
    QRegularExpression re(QLatin1String("^\\d+\\.-?\\d+$"));
    return version.isEmpty() || version == undefinedVersion || re.match(version).hasMatch();
}

const QStringList QmlJS::splitVersion(const QString &version)
{
    // Successively removing minor and major version numbers.
    QStringList result;
    int versionEnd = version.length();
    while (versionEnd > 0) {
        result.append(version.left(versionEnd));
        // remove numbers and then potential . at the end
        const int oldVersionEnd = versionEnd;
        while (versionEnd > 0 && version.at(versionEnd - 1).isDigit())
            --versionEnd;
        // handle e.g. -1, because an import "QtQuick 2" results in version "2.-1"
        if (versionEnd > 0 && version.at(versionEnd - 1) == '-')
            --versionEnd;
        if (versionEnd > 0 && version.at(versionEnd - 1) == '.')
            --versionEnd;
        // bail out if we didn't proceed because version string contains invalid characters
        if (versionEnd == oldVersionEnd)
            break;
    }
    return result;
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
QList<Utils::FilePath> QmlJS::modulePaths(const QString &name,
                                          const QString &version,
                                          const QList<Utils::FilePath> &importPaths)
{
    Q_ASSERT(maybeModuleVersion(version));
    if (importPaths.isEmpty())
        return {};

    const QString sanitizedVersion = version == undefinedVersion ? QString() : version;
    const QStringList parts = name.split('.', Qt::SkipEmptyParts);
    auto mkpath = [](const QStringList &xs) -> QString { return xs.join(QLatin1Char('/')); };

    QList<Utils::FilePath> result;
    Utils::FilePath candidate;

    for (const QString &versionPart : splitVersion(sanitizedVersion)) {
        for (const Utils::FilePath &path : importPaths) {
            for (int i = parts.count() - 1; i >= 0; --i) {
                candidate = path.pathAppended(QString::fromLatin1("%2.%3/%4")
                                                  .arg(mkpath(parts.mid(0, i + 1)),
                                                       versionPart,
                                                       mkpath(parts.mid(i + 1))))
                                .cleanPath();
                if (candidate.exists())
                    result << candidate;
            }
        }
    }

    // Version is empty
    for (const Utils::FilePath &path : importPaths) {
        candidate = path.pathAppended(mkpath(parts)).cleanPath();
        if (candidate.exists())
            result << candidate;
    }

    return result;
}

bool QmlJS::isValidBuiltinPropertyType(const QString &name)
{
    return sharedData()->validBuiltinPropertyNames.contains(name);
}

