// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <utils/aspects.h>

namespace LanguageClient {

namespace Internal { class MimeTypesAspectPrivate; }

class LANGUAGECLIENT_EXPORT MimeTypesAspect : public Utils::TypedAspect<QStringList>
{
    Q_OBJECT

public:
    explicit MimeTypesAspect(Utils::AspectContainer *container = nullptr);
    ~MimeTypesAspect() override;

    bool guiToVolatileValue() override;
    void volatileValueToGui() override;
    void addToLayoutImpl(Layouting::Layout &parent) override;

private:
    void showMimeTypeDialog();

    std::unique_ptr<Internal::MimeTypesAspectPrivate> d;
};

} // namespace LanguageClient
