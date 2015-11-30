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

#ifndef QMT_NAMECONTROLLER_H
#define QMT_NAMECONTROLLER_H

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include <QString>
#include <QList>

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
    static QList<QString> buildElementsPath(const QString &filePath, bool ignoreLastFilePathPart);
};

} // namespace qmt

#endif // QMT_NAMECONTROLLER_H
