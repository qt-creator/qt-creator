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
    virtual bool isValid() const { return true; }
    virtual QKeySequence defaultShortcut() const;
    virtual bool isReplaceSupported() const { return false; }
    virtual bool showSearchTermInput() const { return true; }
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
    void validChanged(bool valid);
    void displayNameChanged();
};

} // namespace Core
