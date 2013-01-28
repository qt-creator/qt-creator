/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILENAMEVALIDATINGLINEEDIT_H
#define FILENAMEVALIDATINGLINEEDIT_H

#include "basevalidatinglineedit.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileNameValidatingLineEdit : public BaseValidatingLineEdit
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
    virtual bool validate(const QString &value, QString *errorMessage) const;
    virtual QString fixInputString(const QString &string);

private:
    bool m_allowDirectories;
    QStringList m_requiredExtensionList;
    bool m_forceFirstCapitalLetter;
};

} // namespace Utils

#endif // FILENAMEVALIDATINGLINEEDIT_H
