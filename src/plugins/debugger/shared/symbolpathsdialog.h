// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QString>

#include <utils/pathchooser.h>

namespace Debugger::Internal {

class SymbolPathsDialog : public QDialog
{
public:
    explicit SymbolPathsDialog(QWidget *parent = nullptr);
    ~SymbolPathsDialog() override;

    bool useSymbolCache() const;
    bool useSymbolServer() const;
    Utils::FilePath path() const;
    bool doNotAskAgain() const;

    void setUseSymbolCache(bool useSymbolCache);
    void setUseSymbolServer(bool useSymbolServer);
    void setPath(const Utils::FilePath &path);
    void setDoNotAskAgain(bool doNotAskAgain) const;

    static bool useCommonSymbolPaths(bool &useSymbolCache, bool &useSymbolServer, Utils::FilePath &path);

private:
    QLabel *m_pixmapLabel;
    QLabel *m_msgLabel;
    QCheckBox *m_useLocalSymbolCache;
    QCheckBox *m_useSymbolServer;
    Utils::PathChooser *m_pathChooser;
};

} // Debugger::Internal
