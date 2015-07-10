/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BUILTINEDITORDOCUMENTPARSER_H
#define BUILTINEDITORDOCUMENTPARSER_H

#include "baseeditordocumentparser.h"
#include "cpptools_global.h"
#include "cppmodelmanager.h"

#include <cplusplus/CppDocument.h>

#include <QString>

namespace CppTools {

class CPPTOOLS_EXPORT BuiltinEditorDocumentParser : public BaseEditorDocumentParser
{
    Q_OBJECT

public:
    BuiltinEditorDocumentParser(const QString &filePath);

    bool releaseSourceAndAST() const;
    void setReleaseSourceAndAST(bool release);

    CPlusPlus::Document::Ptr document() const;
    CPlusPlus::Snapshot snapshot() const;
    ProjectPart::HeaderPaths headerPaths() const;

    void releaseResources();

signals:
    void finished(CPlusPlus::Document::Ptr document, CPlusPlus::Snapshot snapshot);

public:
    static BuiltinEditorDocumentParser *get(const QString &filePath);

private:
    void updateHelper(const InMemoryInfo &info) override;
    void addFileAndDependencies(CPlusPlus::Snapshot *snapshot,
                                QSet<Utils::FileName> *toRemove,
                                const Utils::FileName &fileName) const;

    struct ExtraState {
        QByteArray configFile;

        ProjectPart::HeaderPaths headerPaths;
        QString projectConfigFile;
        QStringList precompiledHeaders;

        CPlusPlus::Snapshot snapshot;
        bool forceSnapshotInvalidation = false;
    };
    ExtraState extraState() const;
    void setExtraState(const ExtraState &extraState);

    bool m_releaseSourceAndAST = true;
    ExtraState m_extraState;
};

} // namespace CppTools

#endif // BUILTINEDITORDOCUMENTPARSER_H
