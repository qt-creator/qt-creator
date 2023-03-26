// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
