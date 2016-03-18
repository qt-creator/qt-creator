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

#pragma once

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include <QString>
#include <QStringList>

namespace qmt {

class QMT_EXPORT NameController : public QObject
{
    Q_OBJECT

private:
    explicit NameController(QObject *parent = 0);
    ~NameController() override;

public:
    static QString convertFileNameToElementName(const QString &fileName);
    static QString convertElementNameToBaseFileName(const QString &elementName);
    static QString calcRelativePath(const QString &absoluteFileName, const QString &anchorPath);
    static QString calcElementNameSearchId(const QString &elementName);
    static QStringList buildElementsPath(const QString &filePath, bool ignoreLastFilePathPart);
    static bool parseClassName(const QString &fullClassName, QString *umlNamespace,
                               QString *className, QStringList *templateParameters);
};

} // namespace qmt
