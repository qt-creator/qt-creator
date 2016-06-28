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

#include "utils_global.h"

#include "wizardpage.h"

namespace Utils {

class FileWizardPagePrivate;

class QTCREATOR_UTILS_EXPORT FileWizardPage : public WizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName DESIGNABLE true)

public:
    explicit FileWizardPage(QWidget *parent = 0);
    ~FileWizardPage();

    QString fileName() const;
    QString path() const;

    bool isComplete() const;

    void setFileNameLabel(const QString &label);
    void setPathLabel(const QString &label);

    bool forceFirstCapitalLetterForFileName() const;
    void setForceFirstCapitalLetterForFileName(bool b);

    // Validate a base name entry field (potentially containing extension)
    static bool validateBaseName(const QString &name, QString *errorMessage = 0);

signals:
    void activated();
    void pathChanged();

public slots:
    void setPath(const QString &path);
    void setFileName(const QString &name);

private:
    void slotValidChanged();
    void slotActivated();

    FileWizardPagePrivate *d;
};

} // namespace Utils
