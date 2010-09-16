/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
