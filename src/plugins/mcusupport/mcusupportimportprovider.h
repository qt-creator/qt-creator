// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcusupport_global.h"
#include "mcusupportplugin.h"

#include <utils/filepath.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>

namespace McuSupport::Internal {
using namespace QmlJS;
using namespace Utils;
class McuSupportImportProvider : public CustomImportsProvider
{
public:
    McuSupportImportProvider() {}
    ~McuSupportImportProvider() {}

    // Overridden functions
    virtual QList<Import> imports(ValueOwner *valueOwner,
                                  const Document *context,
                                  Snapshot *snapshot) const override;
    virtual void loadBuiltins(ImportsPerDocument *importsPerDocument,
                              Imports *imports,
                              const Document *context,
                              ValueOwner *valueOwner,
                              Snapshot *snapshot) override;

    virtual Utils::FilePaths prioritizeImportPaths(const Document *context,
                                                   const Utils::FilePaths &importPaths) override;

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
};
}; // namespace McuSupport::Internal
