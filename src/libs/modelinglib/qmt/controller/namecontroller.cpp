/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "namecontroller.h"

#include <QFileInfo>
#include <QDebug>

namespace qmt {

NameController::NameController(QObject *parent)
    : QObject(parent)
{
}

NameController::~NameController()
{
}

QString NameController::convertFileNameToElementName(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.baseName().trimmed();
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

QString NameController::calcRelativePath(const QString &absoluteFileName, const QString &anchorPath)
{
    int secondLastSlashIndex = -1;
    int slashIndex = -1;
    int i = 0;
    while (i < absoluteFileName.size() && i < anchorPath.size() && absoluteFileName.at(i) == anchorPath.at(i)) {
        if (absoluteFileName.at(i) == QLatin1Char('/')) {
            secondLastSlashIndex = slashIndex;
            slashIndex = i;
        }
        ++i;
    }

    QString relativePath;

    if (slashIndex < 0) {
        relativePath = absoluteFileName;
    } else if (i >= absoluteFileName.size()) {
        // absoluteFileName is a substring of anchor path.
        if (slashIndex == i - 1) {
            if (secondLastSlashIndex < 0)
                relativePath = absoluteFileName;
            else
                relativePath = absoluteFileName.mid(secondLastSlashIndex + 1);
        } else {
            relativePath = absoluteFileName.mid(slashIndex + 1);
        }
    } else {
        relativePath = absoluteFileName.mid(slashIndex + 1);
    }

    return relativePath;
}

QString NameController::calcElementNameSearchId(const QString &elementName)
{
    QString searchId;
    foreach (const QChar &c, elementName) {
        if (c.isLetterOrNumber())
            searchId += c.toLower();
    }
    return searchId;
}

QStringList NameController::buildElementsPath(const QString &filePath, bool ignoreLastFilePathPart)
{
    QList<QString> relativeElements;

    QStringList split = filePath.split("/");
    QStringList::const_iterator splitEnd = split.constEnd();
    if (ignoreLastFilePathPart || split.last().isEmpty())
        splitEnd = --splitEnd;
    for (auto it = split.constBegin(); it != splitEnd; ++it) {
        QString packageName = qmt::NameController::convertFileNameToElementName(*it);
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
