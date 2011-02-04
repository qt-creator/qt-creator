/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef CPLUSPLUS_OBJC_TYPEQUALIFIERS_H
#define CPLUSPLUS_OBJC_TYPEQUALIFIERS_H

#include "CPlusPlusForwardDeclarations.h"


namespace CPlusPlus {

enum {
  Token_in,
  Token_out,
  Token_copy,
  Token_byref,
  Token_inout,
  Token_assign,
  Token_bycopy,
  Token_getter,
  Token_retain,
  Token_setter,
  Token_oneway,
  Token_readonly,
  Token_nonatomic,
  Token_readwrite,
  Token_identifier
};

CPLUSPLUS_EXPORT int classifyObjectiveCContextKeyword(const char *s, int n);

} // namespace CPlusPlus


#endif // CPLUSPLUS_OBJC_TYPEQUALIFIERS_H
