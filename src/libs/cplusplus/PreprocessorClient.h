/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <QStringList>
#include <QVector>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace CPlusPlus {

class ByteArrayRef;
class Macro;

class CPLUSPLUS_EXPORT MacroArgumentReference
{
  unsigned _bytesOffset;
  unsigned _bytesLength;
  unsigned _utf16charsOffset;
  unsigned _utf16charsLength;

public:
  explicit MacroArgumentReference(unsigned bytesOffset = 0, unsigned bytesLength = 0,
                                  unsigned utf16charsOffset = 0, unsigned utf16charsLength = 0)
    : _bytesOffset(bytesOffset)
    , _bytesLength(bytesLength)
    , _utf16charsOffset(utf16charsOffset)
    , _utf16charsLength(utf16charsLength)
  { }

  unsigned bytesOffset() const
  { return _bytesOffset; }

  unsigned bytesLength() const
  { return _bytesLength; }

  unsigned utf16charsOffset() const
  { return _utf16charsOffset; }

  unsigned utf16charsLength() const
  { return _utf16charsLength; }
};

class CPLUSPLUS_EXPORT Client
{
  Client(const Client &other);
  void operator=(const Client &other);

public:
  enum IncludeType {
    IncludeLocal,
    IncludeGlobal,
    IncludeNext
  };

public:
  Client();
  virtual ~Client() = 0;

  virtual void macroAdded(const Macro &macro) = 0;

  virtual void passedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                          unsigned line, const Macro &macro) = 0;
  virtual void failedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                          const ByteArrayRef &name) = 0;

  virtual void notifyMacroReference(unsigned bytesOffset, unsigned utf16charsOffset,
                                    unsigned line, const Macro &macro) = 0;

  virtual void startExpandingMacro(unsigned bytesOffset, unsigned utf16charsOffset,
                                   unsigned line, const Macro &macro,
                                   const QVector<MacroArgumentReference> &actuals
                                            = QVector<MacroArgumentReference>()) = 0;
  virtual void stopExpandingMacro(unsigned bytesOffset, const Macro &macro) = 0; // TODO: ?!

  /// Mark the given macro name as the include guard for the current file.
  virtual void markAsIncludeGuard(const QByteArray &macroName) = 0;

  /// Start skipping from the given utf16charsOffset.
  virtual void startSkippingBlocks(unsigned utf16charsOffset) = 0;
  virtual void stopSkippingBlocks(unsigned utf16charsOffset) = 0;

  virtual void sourceNeeded(unsigned line, const QString &fileName, IncludeType mode,
                            const QStringList &initialIncludes = QStringList()) = 0;

  static inline bool isInjectedFile(const QString &fileName)
  {
      return fileName.startsWith(QLatin1Char('<')) && fileName.endsWith(QLatin1Char('>'));
  }
};

} // namespace CPlusPlus
