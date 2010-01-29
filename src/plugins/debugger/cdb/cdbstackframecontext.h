/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CDBSTACKFRAMECONTEXT_H
#define CDBSTACKFRAMECONTEXT_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

namespace Debugger {
namespace Internal {

class WatchData;
class WatchHandler;
class CdbSymbolGroupContext;
class CdbDumperHelper;

/* CdbStackFrameContext manages a symbol group context and
 * a dumper context. It dispatches calls between the local items
 * that are handled by the symbol group and those that are handled by the dumpers. */

class CdbStackFrameContext
{
    Q_DISABLE_COPY(CdbStackFrameContext)
public:
    // Mask bits for the source field of watch data.
    enum { SourceMask = 0xFF, ChildrenKnownBit = 0x0100 };

    explicit CdbStackFrameContext(const QSharedPointer<CdbDumperHelper> &dumper,
                                  CdbSymbolGroupContext *symbolContext);
    ~CdbStackFrameContext();

    bool assignValue(const QString &iname, const QString &value,
                     QString *newValue /* = 0 */, QString *errorMessage);
    bool editorToolTip(const QString &iname, QString *value, QString *errorMessage);

    bool populateModelInitially(WatchHandler *wh, QString *errorMessage);

    bool completeData(const WatchData &incompleteLocal,
                      WatchHandler *wh,
                      QString *errorMessage);

private:
    const bool m_useDumpers;
    const QSharedPointer<CdbDumperHelper> m_dumper;
    CdbSymbolGroupContext *m_symbolContext;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBSTACKFRAMECONTEXT_H
