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

#ifndef BUILTINEDITORDOCUMENTPARSER_H
#define BUILTINEDITORDOCUMENTPARSER_H

#include "baseeditordocumentparser.h"
#include "cpptools_global.h"
#include "cppmodelmanager.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/DependencyTable.h>
#include <utils/qtcoverride.h>

#include <QMutex>
#include <QString>

namespace CppTools {

class CPPTOOLS_EXPORT BuiltinEditorDocumentParser : public BaseEditorDocumentParser
{
    Q_OBJECT

public:
    BuiltinEditorDocumentParser(const QString &filePath);

    void update(WorkingCopy workingCopy) QTC_OVERRIDE;
    void releaseResources();

    CPlusPlus::Document::Ptr document() const;
    CPlusPlus::Snapshot snapshot() const;
    ProjectPart::HeaderPaths headerPaths() const;

    void setReleaseSourceAndAST(bool onoff);

public:
    static BuiltinEditorDocumentParser *get(const QString &filePath);

private:
    void addFileAndDependencies(QSet<QString> *toRemove, const QString &fileName) const;

private:
    QByteArray m_configFile;

    ProjectPart::HeaderPaths m_headerPaths;
    QString m_projectConfigFile;
    QStringList m_precompiledHeaders;

    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::DependencyTable m_deps;
    bool m_forceSnapshotInvalidation;
    bool m_releaseSourceAndAST;
};

} // namespace CppTools

#endif // BUILTINEDITORDOCUMENTPARSER_H
