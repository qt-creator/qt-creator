/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CALLGRINDTEXTMARK_H
#define CALLGRINDTEXTMARK_H

#include <texteditor/basetextmark.h>

#include <QtCore/QPersistentModelIndex>

namespace Valgrind {

namespace Callgrind {
class Function;
}

namespace Internal {

class CallgrindTextMark : public TextEditor::BaseTextMark
{
    Q_OBJECT

public:
    /**
     * This creates a callgrind text mark for a specific Function
     *
     * \param index DataModel model index
     * \note The index parameter must refer to one of the DataModel cost columns
     */
    explicit CallgrindTextMark(const QPersistentModelIndex &index,
                               const QString &fileName, int lineNumber);

    const Valgrind::Callgrind::Function *function() const;

    virtual double widthFactor() const;
    virtual void paint(QPainter *painter, const QRect &paintRect) const;

private:
    QPersistentModelIndex m_modelIndex;
};

} // namespace Internal
} // namespace Valgrind

#endif // CALLGRINDTEXTMARK_H
