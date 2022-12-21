// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    explicit FileNameValidatingLineEdit(QWidget *parent = nullptr);

    static bool validateFileName(const QString &name,
                                 bool allowDirectories = false,
                                 QString *errorMessage = nullptr);

    static bool validateFileNameExtension(const QString &name,
                                          const QStringList &requiredExtensions = QStringList(),
                                          QString *errorMessage = nullptr);

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
    QString fixInputString(const QString &string) override;

private:
    bool m_allowDirectories;
    QStringList m_requiredExtensionList;
    bool m_forceFirstCapitalLetter;
};

} // namespace Utils
