/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/


#ifndef UICODECOMPLETIONSUPPORT_H
#define UICODECOMPLETIONSUPPORT_H

#include "cpptools_global.h"
#include "abstracteditorsupport.h"

#include <cplusplus/ModelManagerInterface.h>

#include <QtCore/QDateTime>

namespace CppTools {

class CPPTOOLS_EXPORT UiCodeModelSupport : public AbstractEditorSupport
{
public:
    UiCodeModelSupport(CPlusPlus::CppModelManagerInterface *modelmanager,
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
    void init() const;
    bool runUic(const QString &ui) const;
    QString m_sourceName;
    QString m_fileName;
    mutable bool m_updateIncludingFiles;
    mutable bool m_initialized;
    mutable QByteArray m_contents;
    mutable QDateTime m_cacheTime;
};

} // CppTools

#endif // UICODECOMPLETIONSUPPORT_H
