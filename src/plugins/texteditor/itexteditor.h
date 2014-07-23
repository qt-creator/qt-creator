/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ITEXTEDITOR_H
#define ITEXTEDITOR_H

#include "texteditor_global.h"

#include <coreplugin/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QMap>

QT_BEGIN_NAMESPACE
class QMenu;
class QPainter;
class QPoint;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace Utils { class CommentDefinition; }

namespace TextEditor {

class TEXTEDITOR_EXPORT BlockRange
{
public:
    BlockRange() : _first(0), _last(-1) {}
    BlockRange(int firstPosition, int lastPosition)
      : _first(firstPosition), _last(lastPosition)
    {}

    inline bool isNull() const { return _last < _first; }

    int first() const { return _first; }
    int last() const { return _last; }

private:
    int _first;
    int _last;
};


class TEXTEDITOR_EXPORT BaseTextEditorDocument : public Core::TextDocument
{
    Q_OBJECT
public:
    explicit BaseTextEditorDocument(QObject *parent = 0);

    virtual QString plainText() const = 0;
    virtual QString textAt(int pos, int length) const = 0;
    virtual QChar characterAt(int pos) const = 0;

    static QMap<QString, QString> openedTextDocumentContents();
    static QMap<QString, QTextCodec *> openedTextDocumentEncodings();

signals:
    void contentsChanged();
};

} // namespace TextEditor

#endif // ITEXTEDITOR_H
