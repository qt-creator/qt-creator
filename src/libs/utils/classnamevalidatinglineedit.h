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

#ifndef CLASSNAMEVALIDATINGLINEEDIT_H
#define CLASSNAMEVALIDATINGLINEEDIT_H

#include "utils_global.h"
#include "basevalidatinglineedit.h"

namespace Utils {

struct ClassNameValidatingLineEditPrivate;

/* A Line edit that validates a C++ class name and emits a signal
 * to derive suggested file names from it. */

class QTCREATOR_UTILS_EXPORT ClassNameValidatingLineEdit
  : public Utils::BaseValidatingLineEdit
{
    Q_DISABLE_COPY(ClassNameValidatingLineEdit)
    Q_PROPERTY(bool namespacesEnabled READ namespacesEnabled WRITE setNamespacesEnabled DESIGNABLE true)
    Q_PROPERTY(bool lowerCaseFileName READ lowerCaseFileName WRITE setLowerCaseFileName)
    Q_OBJECT

public:
    explicit ClassNameValidatingLineEdit(QWidget *parent = 0);
    virtual ~ClassNameValidatingLineEdit();

    bool namespacesEnabled() const;
    void setNamespacesEnabled(bool b);

    bool lowerCaseFileName() const;
    void setLowerCaseFileName(bool v);

    // Clean an input string to get a valid class name.
    static QString createClassName(const QString &name);

signals:
    // Will be emitted with a suggestion for a base name of the
    // source/header file of the class.
    void updateFileName(const QString &t);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;
    virtual void slotChanged(const QString &t);

private:
    ClassNameValidatingLineEditPrivate *m_d;
};

} // namespace Utils

#endif // CLASSNAMEVALIDATINGLINEEDIT_H
