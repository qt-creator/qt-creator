/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QList>

#include <utils/filepath.h>

#include "../core_global.h"

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {

class IWizardFactory;

class CORE_EXPORT NewDialog {
public:
    NewDialog();
    virtual ~NewDialog() = 0;
    virtual QWidget *widget() = 0;
    virtual void setWizardFactories(QList<IWizardFactory *> factories,
                                    const Utils::FilePath &defaultLocation,
                                    const QVariantMap &extraVariables) = 0;
    virtual void setWindowTitle(const QString &title) = 0;
    virtual void showDialog() = 0;

    static QWidget *currentDialog()
    {
        return m_currentDialog ? m_currentDialog->widget() : nullptr;
    }

private:
    inline static NewDialog *m_currentDialog = nullptr;
};

} // namespace Core
