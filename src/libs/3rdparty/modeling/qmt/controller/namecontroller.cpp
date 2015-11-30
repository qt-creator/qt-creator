/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    bool makeUppercase = true;
    bool insertSpace = false;
    for (int i = 0; i < baseName.size(); ++i) {
        if (baseName.at(i) == QLatin1Char('_')) {
            makeUppercase = true;
            insertSpace = true;
        } else if (baseName.at(i) == QLatin1Char(' ') || baseName.at(i) == QLatin1Char('-')) {
            makeUppercase = true;
            insertSpace = false;
            elementName += baseName.at(i);
        } else if (makeUppercase) {
            if (insertSpace) {
                elementName += QLatin1Char(' ');
                insertSpace = false;
            }
            elementName += baseName.at(i).toUpper();
            makeUppercase = false;
        } else {
            if (insertSpace) {
                elementName += QLatin1Char(' ');
                insertSpace = false;
            }
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

QList<QString> NameController::buildElementsPath(const QString &filePath, bool ignoreLastFilePathPart)
{
    QList<QString> relativeElements;

    QStringList splitted = filePath.split(QStringLiteral("/"));
    QStringList::const_iterator splittedEnd = splitted.end();
    if (ignoreLastFilePathPart || splitted.last().isEmpty())
        splittedEnd = --splittedEnd;
    for (auto it = splitted.cbegin(); it != splittedEnd; ++it) {
        QString packageName = qmt::NameController::convertFileNameToElementName(*it);
        relativeElements.append(packageName);
    }
    return relativeElements;
}

} // namespace qmt
