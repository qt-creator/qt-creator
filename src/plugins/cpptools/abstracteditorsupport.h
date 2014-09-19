/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ABSTRACTEDITORSUPPORT_H
#define ABSTRACTEDITORSUPPORT_H

#include "cpptools_global.h"

#include <QString>
#include <QObject>

namespace CppTools {
class CppModelManager;

class CPPTOOLS_EXPORT AbstractEditorSupport : public QObject
{
    Q_OBJECT
public:
    explicit AbstractEditorSupport(CppModelManager *modelmanager);
    virtual ~AbstractEditorSupport();

    /// \returns the contents, encoded as UTF-8
    virtual QByteArray contents() const = 0;
    virtual QString fileName() const = 0;

    void updateDocument();
    unsigned revision() const { return m_revision; }

    static QString licenseTemplate(const QString &file = QString(), const QString &className = QString());

private:
    CppModelManager *m_modelmanager;
    unsigned m_revision;
};

} // namespace CppTools

#endif // ABSTRACTEDITORSUPPORT_H
