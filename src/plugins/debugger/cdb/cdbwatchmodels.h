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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef CDBWATCHMODELS_H
#define CDBWATCHMODELS_H

#include "abstractsyncwatchmodel.h"
#include <QtCore/QSharedPointer>

namespace Debugger {
namespace Internal {

class CdbSymbolGroupContext;
class CdbDumperHelper;
class CdbDebugEngine;

class CdbLocalsModel : public AbstractSyncWatchModel {
    Q_DISABLE_COPY(CdbLocalsModel)
    Q_OBJECT
public:
    explicit CdbLocalsModel(const QSharedPointer<CdbDumperHelper> &dh,
                            WatchHandler *handler, WatchType type, QObject *parent = 0);
    ~CdbLocalsModel();

    // Set a symbolgroup context, thus activating a stack frame.
    // Set 0 to clear out.
    void setSymbolGroupContext(CdbSymbolGroupContext *sg = 0);
    CdbSymbolGroupContext *symbolGroupContext() const { return m_symbolGroupContext; }

    void setUseDumpers(bool d) { m_useDumpers = d; }

protected:
    virtual bool fetchChildren(WatchItem *wd, QString *errorMessage);
    virtual bool complete(WatchItem *wd, QString *errorMessage);

private:
    const QSharedPointer<CdbDumperHelper> m_dumperHelper;
    CdbSymbolGroupContext *m_symbolGroupContext;
    bool m_useDumpers;
};

class CdbWatchModel : public AbstractSyncWatchModel {
   Q_DISABLE_COPY(CdbWatchModel)
   Q_OBJECT
public:
    explicit CdbWatchModel(CdbDebugEngine *engine,
                           const QSharedPointer<CdbDumperHelper> &dh,
                           WatchHandler *handler,
                           WatchType type, QObject *parent = 0);
   ~CdbWatchModel();

public slots:
   void addWatcher(const WatchData &d);
   void refresh();

protected:
    virtual bool fetchChildren(WatchItem *wd, QString *errorMessage);
    virtual bool complete(WatchItem *wd, QString *errorMessage);

private:
    bool evaluateWatchExpression(WatchData *wd, QString *errorMessage);

   const QSharedPointer<CdbDumperHelper> m_dumperHelper;
   CdbDebugEngine *m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBWATCHMODELS_H
