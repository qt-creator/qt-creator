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

#include "fancylineedit.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileNameValidatingLineEdit : public FancyLineEdit
{
    Q_OBJECT
    Q_PROPERTY(bool allowDirectories READ allowDirectories WRITE setAllowDirectories)
    Q_PROPERTY(QStringList requiredExtensions READ requiredExtensions WRITE setRequiredExtensions)
    Q_PROPERTY(bool forceFirstCapitalLetter READ forceFirstCapitalLetter WRITE setForceFirstCapitalLetter)

public:
    explicit FileNameValidatingLineEdit(QWidget *parent = 0);

    static bool validateFileName(const QString &name,
                                 bool allowDirectories = false,
                                 QString *errorMessage = 0);

    static bool validateFileNameExtension(const QString &name,
                                          const QStringList &requiredExtensions = QStringList(),
                                          QString *errorMessage = 0);

    /**
     * Sets whether entering directories is allowed. This will enable the user
     * to enter slashes in the filename. Default is off.
     */
    bool allowDirectories() const;
    void setAllowDirectories(bool v);

    /**
     * Sets whether the first letter is forced to be a capital letter
     * Default is off.
     */
    bool forceFirstCapitalLetter() const;
    void setForceFirstCapitalLetter(bool b);

    /**
     * Sets a requred extension. If the extension is empty no extension is required.
     * Default is empty.
     */
    QStringList requiredExtensions() const;
    void setRequiredExtensions(const QStringList &extensionList);

protected:
    virtual QString fixInputString(const QString &string);

private:
    bool m_allowDirectories;
    QStringList m_requiredExtensionList;
    bool m_forceFirstCapitalLetter;
};

} // namespace Utils
