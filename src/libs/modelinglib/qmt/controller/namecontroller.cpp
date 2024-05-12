// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "namecontroller.h"

#include <QDebug>

using Utils::FilePath;

namespace qmt {

NameController::NameController(QObject *parent)
    : QObject(parent)
{
}

NameController::~NameController()
{
}

QString NameController::convertFileNameToElementName(const FilePath &fileName)
{
    QString baseName = fileName.baseName().trimmed();
    QString elementName;
    bool makeTitlecase = true;
    bool insertSpace = false;
    for (int i = 0; i < baseName.size(); ++i) {
        if (baseName.at(i) == QLatin1Char('_')) {
            makeTitlecase = true;
            insertSpace = true;
        } else if (baseName.at(i) == QLatin1Char(' ') || baseName.at(i) == QLatin1Char('-')) {
            makeTitlecase = true;
            insertSpace = false;
            elementName += baseName.at(i);
        } else if (makeTitlecase) {
            if (insertSpace) {
                elementName += QLatin1Char(' ');
                insertSpace = false;
            }
            elementName += baseName.at(i).toTitleCase();
            makeTitlecase = false;
        } else {
            // insertSpace must be false here
            elementName += baseName.at(i);
        }
    }
    return elementName;
}

QString NameController::convertElementNameToBaseFileName(const QString &elementName)
{
    QString baseFileName;
    bool insertUnderscore = false;
    for (int i = 0; i < elementName.size(); ++i) {
        if (elementName.at(i) == QLatin1Char(' ')) {
            insertUnderscore = true;
        } else {
            if (insertUnderscore) {
                baseFileName += QLatin1Char('_');
                insertUnderscore = false;
            }
            baseFileName += elementName.at(i).toLower();
        }
    }
    return baseFileName;
}

FilePath NameController::calcRelativePath(const FilePath &absoluteFileName,
                                          const FilePath &anchorPath)
{
    // TODO try using Utils::FilePath::relativePath
    QString absoluteFilePath = absoluteFileName.path();
    QString anchorPathString = anchorPath.path();
    int secondLastSlashIndex = -1;
    int slashIndex = -1;
    int i = 0;
    while (i < absoluteFilePath.size() && i < anchorPathString.size()
           && absoluteFilePath.at(i) == anchorPathString.at(i))
    {
        if (absoluteFilePath.at(i) == QLatin1Char('/')) {
            secondLastSlashIndex = slashIndex;
            slashIndex = i;
        }
        ++i;
    }

    QString relativePath;

    if (slashIndex < 0) {
        relativePath = absoluteFilePath;
    } else if (i >= absoluteFilePath.size()) {
        // absoluteFileName is a substring of anchor path.
        if (slashIndex == i - 1) {
            if (secondLastSlashIndex < 0)
                relativePath = absoluteFilePath;
            else
                relativePath = absoluteFilePath.mid(secondLastSlashIndex + 1);
        } else {
            relativePath = absoluteFilePath.mid(slashIndex + 1);
        }
    } else {
        relativePath = absoluteFilePath.mid(slashIndex + 1);
    }

    return FilePath::fromString(relativePath);
}

QString NameController::calcElementNameSearchId(const QString &elementName)
{
    QString searchId;
    for (const QChar &c : elementName) {
        if (c.isLetterOrNumber())
            searchId += c.toLower();
    }
    return searchId;
}

QStringList NameController::buildElementsPath(const FilePath &filePath,
                                              bool ignoreLastFilePathPart)
{
    QList<QString> relativeElements;

    QStringList split = filePath.toString().split("/");
    QStringList::const_iterator splitEnd = split.constEnd();
    if (ignoreLastFilePathPart || split.last().isEmpty())
        --splitEnd;
    for (auto it = split.constBegin(); it != splitEnd; ++it) {
        QString packageName = qmt::NameController::convertFileNameToElementName(FilePath::fromString(*it));
        relativeElements.append(packageName);
    }
    return relativeElements;
}

bool NameController::parseClassName(const QString &fullClassName, QString *umlNamespace,
                                    QString *className, QStringList *templateParameters)
{
    enum Status {
        IdentifierFirstLetter,
        Identifier,
        FirstColon,
        FirstColonOrBeginTemplate,
        Template,
        FirstColonOrEndOfName,
    };

    if (umlNamespace)
        umlNamespace->clear();
    className->clear();
    templateParameters->clear();

    Status status = IdentifierFirstLetter;
    int identifierStart = -1;
    int identifierEnd = -1;
    int umlNamespaceStart = -1;
    int umlNamespaceEnd = -1;
    int templateDepth = 0;
    int templateArgumentStart = -1;
    int templateEnd = -1;

    for (int i = 0; i < fullClassName.size(); ++i) {
        QChar c = fullClassName.at(i);
        switch (status) {
        case IdentifierFirstLetter:
            if (c.isLetter() || c == QLatin1Char('_')) {
                identifierStart = i;
                identifierEnd = i;
                status = Identifier;
            } else if (!c.isSpace()) {
                return false;
            }
            break;
        case Identifier:
            if (c == QLatin1Char(':')) {
                if (umlNamespaceStart < 0)
                    umlNamespaceStart = identifierStart;
                umlNamespaceEnd = i - 1;
                identifierStart = -1;
                identifierEnd = -1;
                status = FirstColon;
            } else if (c == QLatin1Char('<')) {
                templateDepth = 1;
                templateArgumentStart = i + 1;
                status = Template;
            } else if (c.isSpace()) {
                status = FirstColonOrBeginTemplate;
            } else if (c.isLetterOrNumber() || c == QLatin1Char('_')) {
                identifierEnd = i;
            } else {
                return false;
            }
            break;
        case FirstColon:
            if (c == QLatin1Char(':'))
                status = IdentifierFirstLetter;
            else
                return false;
            break;
        case FirstColonOrBeginTemplate:
            if (c == QLatin1Char(':')) {
                if (umlNamespaceStart < 0)
                    umlNamespaceStart = identifierStart;
                umlNamespaceEnd = identifierEnd;
                identifierStart = -1;
                identifierEnd = -1;
                status = FirstColon;
            } else if (c == QLatin1Char('<')) {
                templateDepth = 1;
                templateArgumentStart = i + 1;
                status = Template;
            } else if (!c.isSpace()) {
                return false;
            }
            break;
        case Template:
            if (c == QLatin1Char('<')) {
                ++templateDepth;
            } else if (c == QLatin1Char('>')) {
                if (templateDepth == 0) {
                    return false;
                } else if (templateDepth == 1) {
                    QString arg = fullClassName.mid(templateArgumentStart, i - templateArgumentStart).trimmed();
                    if (!arg.isEmpty())
                        templateParameters->append(arg);
                    templateEnd = i;
                    status = FirstColonOrEndOfName;
                }
                --templateDepth;
            } else if (c == QLatin1Char(',') && templateDepth == 1) {
                QString arg = fullClassName.mid(templateArgumentStart, i - templateArgumentStart).trimmed();
                if (!arg.isEmpty())
                    templateParameters->append(arg);
                templateArgumentStart = i + 1;
            }
            break;
        case FirstColonOrEndOfName:
            if (c == QLatin1Char(':')) {
                templateParameters->clear();
                if (umlNamespaceStart < 0)
                    umlNamespaceStart = identifierStart;
                umlNamespaceEnd = templateEnd;
                identifierStart = -1;
                identifierEnd = -1;
                status = FirstColon;
            } else if (!c.isSpace()) {
                return false;
            }
        }
    }
    if (umlNamespace && umlNamespaceStart >= 0 && umlNamespaceEnd >= 0)
        *umlNamespace = fullClassName.mid(umlNamespaceStart, umlNamespaceEnd - umlNamespaceStart + 1);
    if (identifierStart >= 0 && identifierEnd >= 0)
        *className = fullClassName.mid(identifierStart, identifierEnd - identifierStart + 1);
    return true;
}

} // namespace qmt
