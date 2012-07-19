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

#ifndef CPLUSPLUS_PP_CLIENT_H
#define CPLUSPLUS_PP_CLIENT_H

#include <CPlusPlusForwardDeclarations.h>
#include <QVector>

QT_BEGIN_NAMESPACE
class QByteArray;
class QString;
QT_END_NAMESPACE

namespace CPlusPlus {

class ByteArrayRef;
class Macro;

class CPLUSPLUS_EXPORT MacroArgumentReference
{
  unsigned _position;
  unsigned _length;

public:
  explicit MacroArgumentReference(unsigned position = 0, unsigned length = 0)
    : _position(position), _length(length)
  { }

  unsigned position() const
  { return _position; }

  unsigned length() const
  { return _length; }
};

class CPLUSPLUS_EXPORT Client
{
  Client(const Client &other);
  void operator=(const Client &other);

public:
  enum IncludeType {
    IncludeLocal,
    IncludeGlobal
  };

public:
  Client();
  virtual ~Client() = 0;

  virtual void macroAdded(const Macro &macro) = 0;

  virtual void passedMacroDefinitionCheck(unsigned offset, unsigned line, const Macro &macro) = 0;
  virtual void failedMacroDefinitionCheck(unsigned offset, const ByteArrayRef &name) = 0;

  virtual void notifyMacroReference(unsigned offset, unsigned line, const Macro &macro) = 0;

  virtual void startExpandingMacro(unsigned offset,
                                   unsigned line,
                                   const Macro &macro,
                                   const QVector<MacroArgumentReference> &actuals
                                            = QVector<MacroArgumentReference>()) = 0;
  virtual void stopExpandingMacro(unsigned offset, const Macro &macro) = 0;

  /// Start skipping from the given offset.
  virtual void startSkippingBlocks(unsigned offset) = 0;
  virtual void stopSkippingBlocks(unsigned offset) = 0;

  virtual void sourceNeeded(unsigned line, QString &fileName, IncludeType mode) = 0;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_PP_CLIENT_H
