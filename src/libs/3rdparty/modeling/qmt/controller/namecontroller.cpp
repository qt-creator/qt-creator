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

QString NameController::convertFileNameToElementName(const QString &file_name)
{
    QFileInfo file_info(file_name);
    QString base_name = file_info.baseName().trimmed();
    QString element_name;
    bool make_uppercase = true;
    bool insert_space = false;
    for (int i = 0; i < base_name.size(); ++i) {
        if (base_name.at(i) == QLatin1Char('_')) {
            make_uppercase = true;
            insert_space = true;
        } else if (base_name.at(i) == QLatin1Char(' ') || base_name.at(i) == QLatin1Char('-')) {
            make_uppercase = true;
            insert_space = false;
            element_name += base_name.at(i);
        } else if (make_uppercase) {
            if (insert_space) {
                element_name += QLatin1Char(' ');
                insert_space = false;
            }
            element_name += base_name.at(i).toUpper();
            make_uppercase = false;
        } else {
            if (insert_space) {
                element_name += QLatin1Char(' ');
                insert_space = false;
            }
            element_name += base_name.at(i);
        }
    }
    return element_name;
}

QString NameController::convertElementNameToBaseFileName(const QString &element_name)
{
    QString base_file_name;
    bool insert_underscore = false;
    for (int i = 0; i < element_name.size(); ++i) {
        if (element_name.at(i) == QLatin1Char(' ')) {
            insert_underscore = true;
        } else {
            if (insert_underscore) {
                base_file_name += QLatin1Char('_');
                insert_underscore = false;
            }
            base_file_name += element_name.at(i).toLower();
        }
    }
    return base_file_name;
}

QString NameController::calcRelativePath(const QString &absolute_file_name, const QString &anchor_path)
{
    int second_last_slash_index = -1;
    int slash_index = -1;
    int i = 0;
    while (i < absolute_file_name.size() && i < anchor_path.size() && absolute_file_name.at(i) == anchor_path.at(i)) {
        if (absolute_file_name.at(i) == QLatin1Char('/')) {
            second_last_slash_index = slash_index;
            slash_index = i;
        }
        ++i;
    }

    QString relative_path;

    if (slash_index < 0) {
        relative_path = absolute_file_name;
    } else if (i >= absolute_file_name.size()) {
        // absolute_file_name is a substring of anchor path.
        if (slash_index == i - 1) {
            if (second_last_slash_index < 0) {
                relative_path = absolute_file_name;
            } else {
                relative_path = absolute_file_name.mid(second_last_slash_index + 1);
            }
        } else {
            relative_path = absolute_file_name.mid(slash_index + 1);
        }
    } else {
        relative_path = absolute_file_name.mid(slash_index + 1);
    }

    return relative_path;
}

QString NameController::calcElementNameSearchId(const QString &element_name)
{
    QString search_id;
    foreach (const QChar &c, element_name) {
        if (c.isLetterOrNumber()) {
            search_id += c.toLower();
        }
    }
    return search_id;
}

QList<QString> NameController::buildElementsPath(const QString &file_path, bool ignore_last_file_path_part)
{
    QList<QString> relative_elements;

    QStringList splitted = file_path.split(QStringLiteral("/"));
    QStringList::const_iterator splitted_end = splitted.end();
    if (ignore_last_file_path_part || splitted.last().isEmpty()) {
        splitted_end = --splitted_end;
    }
    for (QStringList::const_iterator it = splitted.cbegin(); it != splitted_end; ++it) {
        QString package_name = qmt::NameController::convertFileNameToElementName(*it);
        relative_elements.append(package_name);
    }
    return relative_elements;
}


}
