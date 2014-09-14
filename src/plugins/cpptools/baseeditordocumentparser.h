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

#ifndef BASEEDITORDOCUMENTPARSER_H
#define BASEEDITORDOCUMENTPARSER_H

#include "cppmodelmanager.h"
#include "cpptools_global.h"

#include <QObject>

namespace CppTools {

class CPPTOOLS_EXPORT BaseEditorDocumentParser : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseEditorDocumentParser)
    BaseEditorDocumentParser();

public:
    BaseEditorDocumentParser(const QString &filePath);
    virtual ~BaseEditorDocumentParser();

    QString filePath() const;

    virtual void update(WorkingCopy workingCopy) = 0;

    ProjectPart::Ptr projectPart() const;
    void setProjectPart(ProjectPart::Ptr projectPart);

    bool usePrecompiledHeaders() const;
    void setUsePrecompiledHeaders(bool usePrecompiledHeaders);

    QByteArray editorDefines() const;
    void setEditorDefines(const QByteArray &editorDefines);

public:
    static BaseEditorDocumentParser *get(const QString &filePath);

protected:
    void updateProjectPart();

    bool editorDefinesChanged() const;
    void resetEditorDefinesChanged();

protected:
    mutable QMutex m_mutex;

private:
    const QString m_filePath;

    ProjectPart::Ptr m_projectPart;
    ProjectPart::Ptr m_manuallySetProjectPart;

    bool m_usePrecompiledHeaders;

    QByteArray m_editorDefines;
    bool m_editorDefinesChangedSinceLastUpdate;
};

} // namespace CppTools

#endif // BASEEDITORDOCUMENTPARSER_H
