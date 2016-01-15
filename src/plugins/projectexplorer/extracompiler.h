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

#include "projectnodes.h"
#include "project.h"

#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>
#include <utils/environment.h>

namespace ProjectExplorer {

class ExtraCompilerPrivate;
class PROJECTEXPLORER_EXPORT ExtraCompiler : public QObject
{
    Q_OBJECT
public:

    ExtraCompiler(const Project *project, const Utils::FileName &source,
                  const Utils::FileNameList &targets, QObject *parent = 0);
    virtual ~ExtraCompiler() override;

    const Project *project() const;
    Utils::FileName source() const;

    // You can set the contents from the outside. This is done if the file has been (re)created by
    // the regular build process.
    void setContent(const Utils::FileName &file, const QString &content);
    QString content(const Utils::FileName &file) const;

    Utils::FileNameList targets() const;

    void setCompileTime(const QDateTime &time);
    QDateTime compileTime() const;

signals:
    void contentsChanged(const Utils::FileName &file);

protected:
    Utils::Environment buildEnvironment() const;

private:
    void onTargetsBuilt(Project *project);
    void onEditorChanged(Core::IEditor *editor);
    void onEditorAboutToClose(Core::IEditor *editor);
    void onActiveTargetChanged();
    void onActiveBuildConfigurationChanged();
    void setDirty();
    virtual void run(const QByteArray &sourceContent) = 0;

    ExtraCompilerPrivate *const d;
};

class PROJECTEXPLORER_EXPORT ExtraCompilerFactory : public QObject
{
    Q_OBJECT
public:
    explicit ExtraCompilerFactory(QObject *parent = 0);

    virtual FileType sourceType() const = 0;
    virtual QString sourceTag() const = 0;

    virtual ExtraCompiler *create(const Project *project, const Utils::FileName &source,
                                  const Utils::FileNameList &targets) = 0;

    static void registerExtraCompilerFactory(ExtraCompilerFactory *factory);
    static QList<ExtraCompilerFactory *> extraCompilerFactories();
};

} // namespace ProjectExplorer
