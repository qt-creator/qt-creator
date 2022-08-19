// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <QString>
#include <QObject>

namespace CppEditor {
class CppModelManager;

class CPPEDITOR_EXPORT AbstractEditorSupport : public QObject
{
    Q_OBJECT
public:
    explicit AbstractEditorSupport(CppModelManager *modelmanager, QObject *parent = nullptr);
    ~AbstractEditorSupport() override;

    /// \returns the contents, encoded as UTF-8
    virtual QByteArray contents() const = 0;
    virtual QString fileName() const = 0;
    virtual QString sourceFileName() const = 0;

    void updateDocument();
    void notifyAboutUpdatedContents() const;
    unsigned revision() const { return m_revision; }

    static QString licenseTemplate(const QString &file = QString(), const QString &className = QString());
    static bool usePragmaOnce();

private:
    CppModelManager *m_modelmanager;
    unsigned m_revision;
};

} // namespace CppEditor
