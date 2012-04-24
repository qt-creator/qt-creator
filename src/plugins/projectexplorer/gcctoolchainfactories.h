/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef GCCTOOLCHAINFACTORIES_H
#define GCCTOOLCHAINFACTORIES_H

#include "toolchain.h"
#include "toolchainconfigwidget.h"
#include "abi.h"
#include "abiwidget.h"

#include <QList>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace ProjectExplorer {
class GccToolChain;

namespace Internal {

class GccToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the tool chain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    bool canCreate();
    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated tool chains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    virtual GccToolChain *createToolChain(bool autoDetect);
    QList<ToolChain *> autoDetectToolchains(const QString &compiler,
                                            const Abi &);
};

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

class GccToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    GccToolChainConfigWidget(GccToolChain *);
    void apply();
    void discard() { setFromToolchain(); }
    bool isDirty() const;
    void makeReadOnly();

private slots:
    void handleCompilerCommandChange();

private:
    void setFromToolchain();

    Utils::PathChooser *m_compilerCommand;
    AbiWidget *m_abiWidget;
    Utils::FileName m_autoDebuggerCommand;

    QList<Abi> m_abiList;
    bool m_isReadOnly;
};

// --------------------------------------------------------------------------
// ClangToolChainFactory
// --------------------------------------------------------------------------

class ClangToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the tool chain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    bool canCreate();
    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated tool chains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    GccToolChain *createToolChain(bool autoDetect);
};

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

class MingwToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the tool chain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    bool canCreate();
    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated tool chains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    GccToolChain *createToolChain(bool autoDetect);
};

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

class LinuxIccToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    // Name used to display the name of the tool chain that will be created.
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    ToolChain *create();

    // Used by the ToolChainManager to restore user-generated tool chains
    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

protected:
    GccToolChain *createToolChain(bool autoDetect);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // GCCTOOLCHAINFACTORIES_H
