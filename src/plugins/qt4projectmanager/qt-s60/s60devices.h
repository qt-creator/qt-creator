/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef S60DEVICES_H
#define S60DEVICES_H

#include <projectexplorer/toolchain.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class S60Devices : public QObject
{
    Q_OBJECT
public:
    struct Device {
        Device();
        QString toHtml() const;

        QString id;
        QString name;
        bool isDefault;
        QString epocRoot;
        QString toolsRoot;
        QString qt;
    };

    S60Devices(QObject *parent = 0);
    bool read();
    QString errorString() const;
    QList<Device> devices() const;
    bool detectQtForDevices();
    Device deviceForId(const QString &id) const;
    Device deviceForEpocRoot(const QString &root) const;
    Device defaultDevice() const;

    static QString cleanedRootPath(const QString &deviceRoot);
signals:
    void qtVersionsChanged();

private:
    bool readLinux();
    bool readWin();
    QString m_errorString;
    QList<Device> m_devices;
};

/* Mixin for the toolchains with convenience functions for EPOC
 * (Windows) and GnuPoc (Linux). */

class S60ToolChainMixin {
    Q_DISABLE_COPY(S60ToolChainMixin)
public:
    explicit S60ToolChainMixin(const S60Devices::Device &device);

    const S60Devices::Device &device() const;

    // Epoc
    QList<ProjectExplorer::HeaderPath> epocHeaderPaths() const;
    void addEpocToEnvironment(ProjectExplorer::Environment *env) const;

    // GnuPoc
    QList<ProjectExplorer::HeaderPath> gnuPocHeaderPaths() const;
    QList<ProjectExplorer::HeaderPath> gnuPocRvctHeaderPaths(int major, int minor) const;
    QStringList gnuPocRvctLibPaths(int armver, bool debug) const;
    void addGnuPocToEnvironment(ProjectExplorer::Environment *env) const;

    bool equals(const S60ToolChainMixin &rhs) const;

private:
    const S60Devices::Device m_device;
};

inline bool operator==(const S60ToolChainMixin &s1, const S60ToolChainMixin &s2)
{ return s1.equals(s2); }
inline bool operator!=(const S60ToolChainMixin &s1, const S60ToolChainMixin &s2)
{ return !s1.equals(s2); }

QDebug operator<<(QDebug dbg, const S60Devices::Device &d);
QDebug operator<<(QDebug dbg, const S60Devices &d);

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICES_H
