/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef LINECOLUMNLABEL_H
#define LINECOLUMNLABEL_H

#include "utils_global.h"
#include <QLabel>

namespace Utils {

class  QTCREATOR_UTILS_EXPORT LineColumnLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString maxText READ maxText WRITE setMaxText DESIGNABLE true)

public:
    explicit LineColumnLabel(QWidget *parent = 0);

    void setText(const QString &text, const QString &maxText);
    QSize sizeHint() const;

    QString maxText() const;
    void setMaxText(const QString &maxText);

private:
    QString m_maxText;
    void *m_unused;
};

} // namespace Utils

#endif // LINECOLUMNLABEL_H
