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


#ifndef UICODECOMPLETIONSUPPORT_H
#define UICODECOMPLETIONSUPPORT_H

#include "cpptools_global.h"
#include "abstracteditorsupport.h"
#include "cppmodelmanagerinterface.h"

#include <QDateTime>
#include <QProcess>

namespace CppTools {

class CPPTOOLS_EXPORT UiCodeModelSupport : public AbstractEditorSupport
{
    Q_OBJECT
public:
    UiCodeModelSupport(CppTools::CppModelManagerInterface *modelmanager,
                       const QString &sourceFile,
                       const QString &uiHeaderFile);
    ~UiCodeModelSupport();
    void setFileName(const QString &name);
    void setSourceName(const QString &name);
    virtual QByteArray contents() const;
    virtual QString fileName() const;
    void updateFromEditor(const QString &formEditorContents);
    void updateFromBuild();
protected:
    virtual QString uicCommand() const = 0;
    virtual QStringList environment() const = 0;
private:
    enum State { BARE, RUNNING, FINISHED };

    void init() const;
    bool runUic(const QString &ui) const;
    Q_SLOT bool finishProcess() const;
    mutable QProcess m_process;
    QString m_sourceName;
    QString m_fileName;
    mutable State m_state;
    mutable QByteArray m_contents;
    mutable QDateTime m_cacheTime;
    static QList<UiCodeModelSupport *> m_waitingForStart;
};

} // CppTools

#endif // UICODECOMPLETIONSUPPORT_H
