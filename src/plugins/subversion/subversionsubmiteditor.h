// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QPair>

namespace Subversion {
namespace Internal {

class SubversionSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT
public:
    SubversionSubmitEditor();

    static QString fileFromStatusLine(const QString &statusLine);

    // A list of ( 'A','C','D','M') status indicators and file names.
    using StatusFilePair = QPair<QString, QString>;

    void setStatusList(const QList<StatusFilePair> &statusOutput);

    QByteArray fileContents() const override;
    bool setFileContents(const QByteArray &contents) override;
};

} // namespace Internal
} // namespace Subversion
