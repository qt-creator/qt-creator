/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SNIPPETSPEC_H
#define SNIPPETSPEC_H

#include <QtCore/QMap>
#include <QtCore/QString>

namespace Snippets {
namespace Internal {

class SnippetSpec
{
public:
    bool load(const QString &fileName);

    QString contents() const;
    QString name() const;
    QString description() const;
    QString category() const;
    QString completionShortcut() const;
    QString argumentDescription(int id) const;
    QString argumentDefault(int id) const;

private:
    QString m_contents;
    QString m_name;
    QString m_description;
    QString m_category;
    QString m_completionShortcut;
    QMap<int, QString> m_argumentDescription;
    QMap<int, QString> m_argumentDefault;
};

} //namespace Internal
} //namespace Snippets

#endif // SNIPPETSPEC_H
