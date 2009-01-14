/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPLUSPLUS_PP_CLIENT_H
#define CPLUSPLUS_PP_CLIENT_H

#include <CPlusPlusForwardDeclarations.h>
#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QByteArray;
class QString;
QT_END_NAMESPACE

namespace CPlusPlus {

class Macro;

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
  virtual ~Client();

  virtual void macroAdded(const Macro &macro) = 0;
  virtual void sourceNeeded(QString &fileName, IncludeType mode,
                            unsigned line) = 0; // ### FIX the signature.

  virtual void startExpandingMacro(unsigned offset,
                                   const Macro &macro,
                                   const QByteArray &originalTextt) = 0;

  virtual void stopExpandingMacro(unsigned offset,
                                  const Macro &macro) = 0;

  virtual void startSkippingBlocks(unsigned offset) = 0;
  virtual void stopSkippingBlocks(unsigned offset) = 0;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_PP_CLIENT_H
