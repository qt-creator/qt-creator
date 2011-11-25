/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMOTOOLCHAIN_H
#define MAEMOTOOLCHAIN_H

#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>

namespace Madde {
namespace Internal {

// --------------------------------------------------------------------------
// MaemoToolChain
// --------------------------------------------------------------------------

class MaemoToolChain : public ProjectExplorer::GccToolChain
{
public:
    ~MaemoToolChain();

    QString typeName() const;
    ProjectExplorer::Abi targetAbi() const;
    Utils::FileName mkspec() const;

    bool isValid() const;
    bool canClone() const;

    void addToEnvironment(Utils::Environment &env) const;

    bool operator ==(const ProjectExplorer::ToolChain &) const;

    ProjectExplorer::ToolChainConfigWidget *configurationWidget();

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    void setQtVersionId(int);
    int qtVersionId() const;

private:
    void updateId();

    explicit MaemoToolChain(bool);
    MaemoToolChain(const MaemoToolChain &);

    int m_qtVersionId;
    mutable QString m_sysroot;
    ProjectExplorer::Abi m_targetAbi;

    friend class MaemoToolChainFactory;
};

// --------------------------------------------------------------------------
// MaemoToolChainConfigWidget
// --------------------------------------------------------------------------

class MaemoToolChainConfigWidget : public ProjectExplorer::ToolChainConfigWidget
{
    Q_OBJECT

public:
    MaemoToolChainConfigWidget(MaemoToolChain *);

    void apply();
    void discard();
    bool isDirty() const;
};

// --------------------------------------------------------------------------
// MaemoToolChainFactory
// --------------------------------------------------------------------------

class MaemoToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    MaemoToolChainFactory();

    QString displayName() const;
    QString id() const;

    QList<ProjectExplorer::ToolChain *> autoDetect();

private slots:
    void handleQtVersionChanges(const QList<int> &);
    QList<ProjectExplorer::ToolChain *> createToolChainList(const QList<int> &);
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOTOOLCHAIN_H
