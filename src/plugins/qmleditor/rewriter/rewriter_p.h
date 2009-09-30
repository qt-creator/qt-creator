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

#ifndef REWRITER_H
#define REWRITER_H

#include <QtCore/QList>
#include <QtCore/QString>

#include "textwriter_p.h"
#include "qmljsastvisitor_p.h"

QT_BEGIN_HEADER
QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

////////////////////////////////////////////////////////////////////////////////
// Replacement
////////////////////////////////////////////////////////////////////////////////
class Replacement
{
	int _offset;
	int _length;
	QString _text;

public:
	Replacement(int offset = 0, int length = 0, const QString &text = QString())
		: _offset(offset), _length(length), _text(text)
	{ }

	bool isNull() const { return _offset == _length; }
	operator bool() const { return ! isNull(); }

	int offset() const { return _offset; }
	int length() const { return _length; }
	QString text() const { return _text; }
};



////////////////////////////////////////////////////////////////////////////////
// Rewriter
////////////////////////////////////////////////////////////////////////////////
class Rewriter: public AST::Visitor
{
protected:
	TextWriter textWriter;
public:
	//
	// Token based API
	//

	/// Returns the text of the token at the given \a location.
	QString textAt(const AST::SourceLocation &location) const;

	QString textAt(const AST::SourceLocation &firstLoc,
				   const AST::SourceLocation &lastLoc) const;

	/// Replace the token at \a loc with the given \a text.
	void replace(const AST::SourceLocation &loc, const QString &text);

	/// Remove the token at the given \a location.
	void remove(const AST::SourceLocation &location);

	/// Remove all tokens in the range [\a firstLoc, \a lastLoc].
	void remove(const AST::SourceLocation &firstLoc, const AST::SourceLocation &lastLoc);

	/// Insert \a text before the token at the given \a location.
	void insertTextBefore(const AST::SourceLocation &location, const QString &text);

	/// Insert \a text after the token at the given \a location.
	void insertTextAfter(const AST::SourceLocation &loc, const QString &text);

	void moveTextBefore(const AST::SourceLocation &firstLoc,
						const AST::SourceLocation &lastLoc,
						const AST::SourceLocation &loc);

	void moveTextAfter(const AST::SourceLocation &firstLoc,
					   const AST::SourceLocation &lastLoc,
					   const AST::SourceLocation &loc);

	//
	// low-level offset based API
	//
	void replace(int offset, int length, const QString &text);
	void insertText(int offset, const QString &text);
	void removeText(int offset, int length);

	/// Visit the given \a node.
	void accept(AST::Node *node);

	/// Returns the original unchanged source code.
	QString code() const { return _code; }

	/// Returns the list of replacements.
	QList<Replacement> replacementList() const { return _replacementList; }

protected:
	/// \internal
	void setCode(const QString &code) { _code = code; }

private:
	QString _code;
	QList<Replacement> _replacementList;
};

} // end of namespace QmlJS

QT_QML_END_NAMESPACE
QT_END_HEADER

#endif // REWRITER_H
