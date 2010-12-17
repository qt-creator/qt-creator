/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OUTPUTPANE_H
#define OUTPUTPANE_H

#include "core_global.h"

#include <QtGui/QWidget>

#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE
class QSplitter;
QT_END_NAMESPACE

namespace Core {

class IMode;

namespace Internal {
class OutputPaneManager;
}
struct OutputPanePlaceHolderPrivate;

class CORE_EXPORT OutputPanePlaceHolder : public QWidget
{
    friend class Core::Internal::OutputPaneManager; // needs to set m_visible and thus access m_current
    Q_OBJECT
public:
    explicit OutputPanePlaceHolder(Core::IMode *mode, QSplitter *parent = 0);
    ~OutputPanePlaceHolder();

    void setCloseable(bool b);
    bool closeable();
    static OutputPanePlaceHolder *getCurrent();
    static bool isCurrentVisible();

    void unmaximize();
    bool isMaximized() const;

private slots:
    void currentModeChanged(Core::IMode *);

private:
    bool canMaximizeOrMinimize() const;
    void maximizeOrMinimize(bool maximize);

    QScopedPointer<OutputPanePlaceHolderPrivate> d;
};

} // namespace Core

#endif // OUTPUTPANE_H
