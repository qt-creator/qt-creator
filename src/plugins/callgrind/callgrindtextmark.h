/**************************************************************************
**
** This file is part of Qt Creator Analyzer Tools
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CALLGRINDTEXTMARK_H
#define CALLGRINDTEXTMARK_H

#include <texteditor/basetextmark.h>

#include <QtCore/QPersistentModelIndex>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {
class Function;
}
}

namespace Callgrind {
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
    virtual ~CallgrindTextMark();

    const Valgrind::Callgrind::Function *function() const;

    virtual double widthFactor() const;
    virtual void paint(QPainter *painter, const QRect &paintRect) const;

private:
    QPersistentModelIndex m_modelIndex;
};

} // namespace Internal
} // namespace Callgrind

#endif // CALLGRINDTEXTMARK_H
