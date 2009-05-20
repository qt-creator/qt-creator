/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstringbuilder.h"

/*!
    \class QLatin1Literal
    \reentrant

    \brief The QLatin1Literal class provides a thin wrapper of string literal
    used in source codes.

    The main use of \c QLatin1Literal is in conjunction with \c QStringBuilder
    to reduce the number of reallocations needed to build up a string from
    smaller chunks.

    Contrary to \c QLatin1String, a \c QLatin1Literal can retrieve its size
    without iterating over the literal.

    \ingroup tools
    \ingroup shared
    \ingroup text
    \mainclass

    \sa QStringBuilder, QLatin1String, QString

*/

/*! \fn QLatin1Literal::QLatin1Literal(const char(&literal)[])
 
    The only constructor of the class. 
*/

/*! \fn int QLatin1Literal::size() const
 
    Returns the number of characters in the literal \i{not} including
    the trailing NUL char.
*/

/*! \fn char *QLatin1Literal::size() const
 
    Returns a pointer to the first character of the string literal.
    The string literal is terminated by a NUL character.
*/

/*! \fn QLatin1Literal::operator QString() const
 
    Converts the \c QLatin1Literal into a \c QString object.
*/


/*!
    \class QStringBuilderPair
    \reentrant

    \brief QStringBuilderPair is a helper class template for building
    QStringBuilder objects wrapping two smaller QStringBuilder object.
*/


/*!
    \class QStringBuilder
    \reentrant

    \brief QStringBuilder is a template class providing a facility to build
    up QStrings from smaller chunks.

    \ingroup tools
    \ingroup shared
    \ingroup text
    \mainclass

    When creating strings from smaller chunks, typically \c QString::operator+()
    is used, resulting \i{n - 1} reallocations when operating on \i{n} chunks.

    QStringBuilder uses expression
    templates to collect the individual parts, compute the total size,
    allocate memory for the resulting QString object, and copy the contents
    of the chunks into the result.

    Using \c QStringBuilder::operator%() yield generally better performance then
    using \c QString::operator+() on the same chunks if there are three or
    more of them, and equal performance otherwise.

    \sa QLatin1Literal, QString
*/

/* !fn template <class A, class B> QStringBuilder< QStringBuilderPair<A, B> > operator%(const A &a, const B &b)
 
   Creates a helper object containing both parameters. 

   This is the main function to build up complex QStringBuilder objects from
   smaller QStringBuilder objects.
*/

