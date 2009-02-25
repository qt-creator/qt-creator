/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef NEWCLASSWIDGET_H
#define NEWCLASSWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QStringList;
QT_END_NAMESPACE

namespace Core {
namespace Utils {

struct NewClassWidgetPrivate;

/* NewClassWidget: Utility widget for 'New Class' wizards. Prompts the user
 * to enter a class name (optionally derived from some base class) and file
 * names for header, source and form files. Has some smart logic to derive
 * the file names from the class name. */

class QWORKBENCH_UTILS_EXPORT NewClassWidget : public QWidget
{
    Q_DISABLE_COPY(NewClassWidget)
    Q_OBJECT
    Q_PROPERTY(bool namespacesEnabled READ namespacesEnabled WRITE setNamespacesEnabled DESIGNABLE true)
    Q_PROPERTY(bool baseClassInputVisible READ isBaseClassInputVisible WRITE setBaseClassInputVisible DESIGNABLE true)
    Q_PROPERTY(bool baseClassEditable READ isBaseClassEditable WRITE setBaseClassEditable DESIGNABLE false)
    Q_PROPERTY(bool formInputVisible READ isFormInputVisible WRITE setFormInputVisible DESIGNABLE true)
    Q_PROPERTY(bool pathInputVisible READ isPathInputVisible WRITE setPathInputVisible DESIGNABLE true)
    Q_PROPERTY(QString className READ className WRITE setClassName DESIGNABLE true)
    Q_PROPERTY(QString baseClassName READ baseClassName WRITE setBaseClassName DESIGNABLE true)
    Q_PROPERTY(QString sourceFileName READ sourceFileName DESIGNABLE false)
    Q_PROPERTY(QString headerFileName READ headerFileName DESIGNABLE false)
    Q_PROPERTY(QString formFileName READ formFileName DESIGNABLE false)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QStringList baseClassChoices READ baseClassChoices WRITE setBaseClassChoices DESIGNABLE true)
    Q_PROPERTY(QString sourceExtension READ sourceExtension WRITE setSourceExtension DESIGNABLE true)
    Q_PROPERTY(QString headerExtension READ headerExtension WRITE setHeaderExtension DESIGNABLE true)
    Q_PROPERTY(QString formExtension READ formExtension WRITE setFormExtension DESIGNABLE true)
    Q_PROPERTY(bool formInputCheckable READ formInputCheckable WRITE setFormInputCheckable DESIGNABLE true)
    Q_PROPERTY(bool formInputChecked READ formInputChecked WRITE setFormInputChecked DESIGNABLE true)
    Q_PROPERTY(bool allowDirectories READ allowDirectories WRITE setAllowDirectories)
    // Utility "USER" property for wizards containing file names.
    Q_PROPERTY(QStringList files READ files DESIGNABLE false USER true)
public:
    explicit NewClassWidget(QWidget *parent = 0);
    ~NewClassWidget();

    bool namespacesEnabled() const;
    bool isBaseClassInputVisible() const;
    bool isBaseClassEditable() const;
    bool isFormInputVisible() const;
    bool isPathInputVisible() const;
    bool formInputCheckable() const;
    bool formInputChecked() const;

    QString className() const;
    QString baseClassName() const;
    QString sourceFileName() const;
    QString headerFileName() const;
    QString formFileName() const;
    QString path() const;
    QStringList baseClassChoices() const;
    QString sourceExtension() const;
    QString headerExtension() const;
    QString formExtension() const;
    bool allowDirectories() const;

    bool isValid(QString *error = 0) const;

    QStringList files() const;

signals:
    void validChanged();
    void activated();

public slots:
    void setNamespacesEnabled(bool b);
    void setBaseClassInputVisible(bool visible);
    void setBaseClassEditable(bool editable);
    void setFormInputVisible(bool visible);
    void setPathInputVisible(bool visible);
    void setFormInputCheckable(bool v);
    void setFormInputChecked(bool v);

    /* The name passed into the new class widget will be reformatted to be a
     * valid class name. */
    void setClassName(const QString &suggestedName);
    void setBaseClassName(const QString &);
    void setPath(const QString &path);
    void setBaseClassChoices(const QStringList &choices);
    void setSourceExtension(const QString &e);
    void setHeaderExtension(const QString &e);
    void setFormExtension(const QString &e);
    void setAllowDirectories(bool v);

    /* Suggest a class name from the base class by stripping the leading 'Q'
     * character. This will happen automagically if the base class combo
     * changes until the class line edited is manually edited. */
    void suggestClassNameFromBase();

private slots:
    void updateFileNames(const QString &t);
    void slotValidChanged();
    void slotActivated();
    void classNameEdited();
    void slotFormInputChecked();

private:
    void setFormInputCheckable(bool checkable, bool force);

    QString fixSuffix(const QString &suffix);
    NewClassWidgetPrivate *m_d;
};

} // namespace Utils
} // namespace Core

#endif // NEWCLASSWIDGET_H
