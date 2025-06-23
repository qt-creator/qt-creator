// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <utils/filepath.h>

#include <QStringList>

namespace CPlusPlus {

class ByteArrayRef;
class Macro;
class Pragma;

class CPLUSPLUS_EXPORT MacroArgumentReference
{
public:
    explicit MacroArgumentReference(int bytesOffset = 0, int bytesLength = 0,
                                    int utf16charsOffset = 0, int utf16charsLength = 0)
        : bytesOffset(bytesOffset)
        , bytesLength(bytesLength)
        , utf16charsOffset(utf16charsOffset)
        , utf16charsLength(utf16charsLength)
    {}

    const int bytesOffset;
    const int bytesLength;
    const int utf16charsOffset;
    const int utf16charsLength;
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
  virtual void pragmaAdded(const Pragma &pragma) = 0;

  virtual void passedMacroDefinitionCheck(int bytesOffset, int utf16charsOffset,
                                          int line, const Macro &macro) = 0;
  virtual void failedMacroDefinitionCheck(int bytesOffset, int utf16charsOffset,
                                          const ByteArrayRef &name) = 0;

  virtual void notifyMacroReference(int bytesOffset, int utf16charsOffset,
                                    int line, const Macro &macro) = 0;

  virtual void startExpandingMacro(int bytesOffset, int utf16charsOffset,
                                   int line, const Macro &macro,
                                   const QList<MacroArgumentReference> &actuals = {}) = 0;
  virtual void stopExpandingMacro(int bytesOffset, const Macro &macro) = 0; // TODO: ?!

  /// Mark the given macro name as the include guard for the current file.
  virtual void markAsIncludeGuard(const QByteArray &macroName) = 0;

  /// Start skipping from the given utf16charsOffset.
  virtual void startSkippingBlocks(int utf16charsOffset) = 0;
  virtual void stopSkippingBlocks(int utf16charsOffset) = 0;

  virtual void sourceNeeded(int line, const Utils::FilePath &fileName, IncludeType mode,
                            const Utils::FilePaths &initialIncludes = {}) = 0;

  static inline bool isInjectedFile(const Utils::FilePath &filePath)
  {
      const QStringView path = filePath.pathView();
      return path.startsWith(QLatin1Char('<')) && path.endsWith(QLatin1Char('>'));
  }
};

} // namespace CPlusPlus
