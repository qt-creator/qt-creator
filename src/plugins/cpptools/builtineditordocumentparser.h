/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "baseeditordocumentparser.h"
#include "cpptools_global.h"

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
    ProjectPartHeaderPaths headerPaths() const;

    void releaseResources();

signals:
    void finished(CPlusPlus::Document::Ptr document, CPlusPlus::Snapshot snapshot);

public:
    using Ptr = QSharedPointer<BuiltinEditorDocumentParser>;
    static Ptr get(const QString &filePath);

private:
    void updateImpl(const QFutureInterface<void> &future,
                    const UpdateParams &updateParams) override;
    void addFileAndDependencies(CPlusPlus::Snapshot *snapshot,
                                QSet<Utils::FileName> *toRemove,
                                const Utils::FileName &fileName) const;

    struct ExtraState {
        QByteArray configFile;

        ProjectPartHeaderPaths headerPaths;
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
