/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#ifndef CLASSNAMEVALIDATINGLINEEDIT_H
#define CLASSNAMEVALIDATINGLINEEDIT_H

#include "utils_global.h"
#include "basevalidatinglineedit.h"

namespace Core {
namespace Utils {

struct ClassNameValidatingLineEditPrivate;

/* A Line edit that validates a C++ class name and emits a signal
 * to derive suggested file names from it. */

class QWORKBENCH_UTILS_EXPORT ClassNameValidatingLineEdit
  : public Core::Utils::BaseValidatingLineEdit
{
    Q_DISABLE_COPY(ClassNameValidatingLineEdit)
    Q_PROPERTY(bool namespacesEnabled READ namespacesEnabled WRITE setNamespacesEnabled DESIGNABLE true)
    Q_OBJECT

public:
    explicit ClassNameValidatingLineEdit(QWidget *parent = 0);
    virtual ~ClassNameValidatingLineEdit();

    bool namespacesEnabled() const;
    void setNamespacesEnabled(bool b);

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
} // namespace Core

#endif // CLASSNAMEVALIDATINGLINEEDIT_H
