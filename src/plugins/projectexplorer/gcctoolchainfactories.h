/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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

namespace Utils { class PathChooser; }

namespace ProjectExplorer {
class GccToolChain;

namespace Internal {

class GccToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    GccToolChainFactory();

    QList<ToolChain *> autoDetect();

    bool canCreate();
    ToolChain *create();

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
    static QStringList splitString(const QString &s);
private slots:
    void handleCompilerCommandChange();
    void handlePlatformCodeGenFlagsChange();
    void handlePlatformLinkerFlagsChange();

private:
    void applyImpl();
    void discardImpl() { setFromToolchain(); }
    bool isDirtyImpl() const;
    void makeReadOnlyImpl();

    void setFromToolchain();

    Utils::PathChooser *m_compilerCommand;
    QLineEdit *m_platformCodeGenFlagsLineEdit;
    QLineEdit *m_platformLinkerFlagsLineEdit;
    AbiWidget *m_abiWidget;

    bool m_isReadOnly;
    QByteArray m_macros;
};

// --------------------------------------------------------------------------
// ClangToolChainFactory
// --------------------------------------------------------------------------

class ClangToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    ClangToolChainFactory();

    QList<ToolChain *> autoDetect();

    bool canRestore(const QVariantMap &data);

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
    MingwToolChainFactory();

    QList<ToolChain *> autoDetect();

    bool canRestore(const QVariantMap &data);

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
    LinuxIccToolChainFactory();

    QList<ToolChain *> autoDetect();

    bool canRestore(const QVariantMap &data);

protected:
    GccToolChain *createToolChain(bool autoDetect);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // GCCTOOLCHAINFACTORIES_H
