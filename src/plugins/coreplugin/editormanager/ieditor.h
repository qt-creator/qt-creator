// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"
#include "../icontext.h"

#include <QMetaType>

namespace Core {

class IDocument;

class CORE_EXPORT IEditor : public IContext
{
    Q_OBJECT

public:
    IEditor();

    bool duplicateSupported() const;
    void setDuplicateSupported(bool duplicateSupported);

    virtual IDocument *document() const = 0;

    virtual IEditor *duplicate() { return nullptr; }

    virtual QByteArray saveState() const { return QByteArray(); }
    virtual void restoreState(const QByteArray & /*state*/) {}

    virtual int currentLine() const { return 0; }
    virtual int currentColumn() const { return 0; }
    virtual void gotoLine(int line, int column = 0, bool centerLine = true) { Q_UNUSED(line) Q_UNUSED(column) Q_UNUSED(centerLine) }

    virtual QWidget *toolBar() = 0;

    virtual bool isDesignModePreferred() const { return false; }

signals:
    void editorDuplicated(IEditor *duplicate);

private:
    bool m_duplicateSupported;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::IEditor*)
