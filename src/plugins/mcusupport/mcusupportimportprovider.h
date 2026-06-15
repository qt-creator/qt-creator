// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcusupport_global.h"
#include "mcusupportplugin.h"

#include <utils/filepath.h>
#include <utils/synchronizedvalue.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>

#include <QHash>

#include <optional>

namespace McuSupport::Internal {
using namespace QmlJS;
using namespace Utils;
class McuSupportImportProvider : public CustomImportsProvider
{
public:
    McuSupportImportProvider() {}
    ~McuSupportImportProvider() {}

    // Overridden functions
    QList<Import> imports(ValueOwner *valueOwner,
                          const Document *context,
                          Snapshot *snapshot) const override;
    void loadBuiltins(ImportsPerDocument *importsPerDocument,
                      Imports *imports,
                      const Document *context,
                      ValueOwner *valueOwner,
                      Snapshot *snapshot) override;

    virtual Utils::FilePaths prioritizeImportPaths(const Document *context,
                                                   const Utils::FilePaths &importPaths) override;

    // Precompute the target build folder on the GUI thread (ProjectManager / ProjectNode
    // are not thread-safe). imports()/etc. read the cached value on the link worker thread.
    void prepare(const Document *context) override;

    // Add to the interfaces needed for a document
    // path: opened qml document
    // importsPerDocument: imports available in the document (considered imported)
    // import: qul import containing builtin types (interfaces)
    // valueOwner: imports members owner
    // snapshot: where qul documenents live
    void getInterfacesImport(const FilePath &path,
                             ImportsPerDocument *importsPerDocument,
                             Import &import,
                             ValueOwner *valueOwner,
                             Snapshot *snapshot) const;

    // Get the qmlproject module which a qmlfile is part of
    // nullopt means the file is part of the main project
    std::optional<FilePath> getFileModule(const FilePath &file, const FilePath &inputFile) const;

private:
    std::optional<FilePath> cachedTargetBuildFolder(const FilePath &path) const;

    Utils::SynchronizedValue<QHash<FilePath, FilePath>> m_targetBuildFolders;
};
}; // namespace McuSupport::Internal
