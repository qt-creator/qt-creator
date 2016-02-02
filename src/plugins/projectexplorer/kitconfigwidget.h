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

#include "projectexplorer_export.h"

#include <coreplugin/id.h>

#include <QWidget>

namespace ProjectExplorer {

class Kit;
class KitInformation;

// --------------------------------------------------------------------------
// KitConfigWidget
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT KitConfigWidget : public QObject
{
    Q_OBJECT

public:
    KitConfigWidget(Kit *kit, const KitInformation *ki);

    Core::Id kitInformationId() const;

    virtual QString displayName() const = 0;
    virtual QString toolTip() const { return QString(); }
    virtual void makeReadOnly() = 0;
    virtual void refresh() = 0;
    virtual bool visibleInKit() { return true; }

    virtual QWidget *mainWidget() const = 0;
    virtual QWidget *buttonWidget() const { return 0; }

    bool isSticky() const { return m_isSticky; }
    bool isMutable() const;
    void setMutable(bool b);

    static QString msgManage();

    Kit *kit() const { return m_kit; }

    virtual void setPalette(const QPalette &p);
    virtual void setStyle(QStyle *s);

signals:
    void dirty();

protected:
    Kit *m_kit;
    const KitInformation *m_kitInformation;
    bool m_isSticky;
};

} // namespace ProjectExplorer
