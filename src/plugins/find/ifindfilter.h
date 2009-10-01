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

#ifndef IFINDFILTER_H
#define IFINDFILTER_H

#include "find_global.h"

#include <QtGui/QTextDocument>

QT_BEGIN_NAMESPACE
class QKeySequence;
class QWidget;
class QSettings;
QT_END_NAMESPACE

namespace Find {

class FIND_EXPORT IFindFilter : public QObject
{
    Q_OBJECT
public:

    virtual ~IFindFilter() {}

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual bool isEnabled() const = 0;
    virtual QKeySequence defaultShortcut() const = 0;
    virtual void findAll(const QString &txt, QTextDocument::FindFlags findFlags) = 0;

    virtual QWidget *createConfigWidget() { return 0; }
    virtual void writeSettings(QSettings *settings) { Q_UNUSED(settings) }
    virtual void readSettings(QSettings *settings) { Q_UNUSED(settings) }

signals:
    void changed();
};

} // namespace Find

#endif // IFINDFILTER_H
