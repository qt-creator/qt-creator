/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rewriter_p.h"
#include "qmljsast_p.h"

QT_QML_BEGIN_NAMESPACE

using namespace QmlJS;

void Rewriter::replace(const AST::SourceLocation &loc, const QString &text)
{ replace(loc.offset, loc.length, text); }

void Rewriter::remove(const AST::SourceLocation &loc)
{ return replace(loc.offset, loc.length, QString()); }

void Rewriter::remove(const AST::SourceLocation &firstLoc, const AST::SourceLocation &lastLoc)
{ return replace(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset, QString()); }

void Rewriter::insertTextBefore(const AST::SourceLocation &loc, const QString &text)
{ replace(loc.offset, 0, text); }

void Rewriter::insertTextAfter(const AST::SourceLocation &loc, const QString &text)
{ replace(loc.offset + loc.length, 0, text); }

void Rewriter::replace(int offset, int length, const QString &text)
{ textWriter.replace(offset, length, text); }

void Rewriter::insertText(int offset, const QString &text)
{ replace(offset, 0, text); }

void Rewriter::removeText(int offset, int length)
{ replace(offset, length, QString()); }

QString Rewriter::textAt(const AST::SourceLocation &loc) const
{ return _code.mid(loc.offset, loc.length); }

QString Rewriter::textAt(const AST::SourceLocation &firstLoc, const AST::SourceLocation &lastLoc) const
{ return _code.mid(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset); }

void Rewriter::accept(QmlJS::AST::Node *node)
{ QmlJS::AST::Node::acceptChild(node, this); }

void Rewriter::moveTextBefore(const AST::SourceLocation &firstLoc,
                              const AST::SourceLocation &lastLoc,
                              const AST::SourceLocation &loc)
{
    textWriter.move(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset, loc.offset);
}

void Rewriter::moveTextAfter(const AST::SourceLocation &firstLoc,
                             const AST::SourceLocation &lastLoc,
                             const AST::SourceLocation &loc)
{
    textWriter.move(firstLoc.offset, lastLoc.offset + lastLoc.length - firstLoc.offset, loc.offset + loc.length);
}

QT_QML_END_NAMESPACE
