/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef LINECOLUMNLABEL_H
#define LINECOLUMNLABEL_H

#include "utils_global.h"
#include <QtGui/QLabel>

namespace Core {
namespace Utils {

/* A label suitable for displaying cursor positions, etc. with a fixed
 * with derived from a sample text. */

class  QWORKBENCH_UTILS_EXPORT LineColumnLabel : public QLabel
{
    Q_DISABLE_COPY(LineColumnLabel)
    Q_OBJECT
    Q_PROPERTY(QString maxText READ maxText WRITE setMaxText DESIGNABLE true)

public:
    explicit LineColumnLabel(QWidget *parent = 0);
    virtual ~LineColumnLabel();

    void setText(const QString &text, const QString &maxText);
    QSize sizeHint() const;

    QString maxText() const;
    void setMaxText(const QString &maxText);

private:
    QString m_maxText;
    void *m_unused;
};

} // namespace Utils
} // namespace Core

#endif // LINECOLUMNLABEL_H
