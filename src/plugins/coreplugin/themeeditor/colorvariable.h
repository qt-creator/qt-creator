/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
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

#ifndef COLORVARIABLE_H
#define COLORVARIABLE_H

#include <QSharedPointer>
#include <QColor>
#include <QString>
#include <QSet>

namespace Core {
namespace Internal {
namespace ThemeEditor {

class ColorRole;
class ThemeColors;

class ColorVariable
{
public:
    friend class ThemeColors;
    typedef QSharedPointer<ColorVariable> Ptr;

    ~ColorVariable();

    // name of this variable
    QString variableName() const { return m_variableName; }

    // value of this variable
    QColor color() const { return m_variableValue; }
    void setColor(const QColor &color);

    // which theme colors are referencing this variable?
    void addReference(ColorRole *t);
    void removeReference(ColorRole *t);
    QSet<ColorRole *> references() const;

private:
    explicit ColorVariable(const QColor &color, const QString &variableName = QString());
    QColor m_variableValue;
    QString m_variableName;
    QSet<ColorRole *> m_references;
};

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core

#endif // COLORVARIABLE_H
