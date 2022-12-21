// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace Utils {
class PathChooser;
class FilePath;
} // namespace Utils

namespace ProjectExplorer {
namespace Internal {

class ImportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImportWidget(QWidget *parent = nullptr);

    void setCurrentDirectory(const Utils::FilePath &dir);

    bool ownsReturnKey() const;

signals:
    void importFrom(const Utils::FilePath &dir);

private:
    void handleImportRequest();

    Utils::PathChooser *m_pathChooser;
    bool m_ownsReturnKey = false;
};

} // namespace Internal
} // namespace ProjectExplorer
