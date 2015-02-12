/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IFINDFILTER_H
#define IFINDFILTER_H

#include "textfindconstants.h"

QT_BEGIN_NAMESPACE
class QWidget;
class QSettings;
class QKeySequence;
class Pixmap;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IFindFilter : public QObject
{
    Q_OBJECT
public:

    virtual ~IFindFilter() {}

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    ///
    virtual bool isEnabled() const = 0;
    virtual QKeySequence defaultShortcut() const;
    virtual bool isReplaceSupported() const { return false; }
    virtual FindFlags supportedFindFlags() const;

    virtual void findAll(const QString &txt, FindFlags findFlags) = 0;
    virtual void replaceAll(const QString &txt, FindFlags findFlags)
    { Q_UNUSED(txt) Q_UNUSED(findFlags) }

    virtual QWidget *createConfigWidget() { return 0; }
    virtual void writeSettings(QSettings *settings) { Q_UNUSED(settings) }
    virtual void readSettings(QSettings *settings) { Q_UNUSED(settings) }

    static QPixmap pixmapForFindFlags(FindFlags flags);
    static QString descriptionForFindFlags(FindFlags flags);
signals:
    void enabledChanged(bool enabled);
};

} // namespace Core

#endif // IFINDFILTER_H
