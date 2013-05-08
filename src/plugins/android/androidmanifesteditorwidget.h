/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDMANIFESTEDITORWIDGET_H
#define ANDROIDMANIFESTEDITORWIDGET_H

#include <texteditor/basetexteditor.h>
#include <texteditor/plaintexteditor.h>

#include <QAbstractListModel>
#include <QStackedWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QDomDocument;
class QDomElement;
class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QListView;
class QSpinBox;
class QToolButton;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace TextEditor { class TextEditorActionHandler; }

namespace Android {
namespace Internal {
class AndroidManifestEditor;


class PermissionsModel: public QAbstractListModel
{
    Q_OBJECT
public:
    PermissionsModel(QObject *parent = 0 );
    void setPermissions(const QStringList &permissions);
    const QStringList &permissions();
    QModelIndex addPermission(const QString &permission);
    bool updatePermission(QModelIndex index, const QString &permission);
    void removePermission(int index);
    QVariant data(const QModelIndex &index, int role) const;

protected:
    int rowCount(const QModelIndex &parent) const;

private:
    QStringList m_permissions;
};

class AndroidManifestEditorWidget : public TextEditor::PlainTextEditorWidget
{
    Q_OBJECT
public:
    enum EditorPage {
        General,
        Source
    };

    explicit AndroidManifestEditorWidget(QWidget *parent, TextEditor::TextEditorActionHandler *ah);

    bool open(QString *errorString, const QString &fileName, const QString &realFileName);

    bool isModified() const;

    EditorPage activePage() const;
    bool setActivePage(EditorPage page);

    void preSave();

public slots:
    void setDirty(bool dirty = true);

protected:
    TextEditor::BaseTextEditor *createEditor();
    void resizeEvent(QResizeEvent *event);

private slots:
    void setLDPIIcon();
    void setMDPIIcon();
    void setHDPIIcon();
    void addPermission();
    void removePermission();
    void updateAddRemovePermissionButtons();
    void setAppName();
    void setPackageName();
    void gotoError();
    void updateInfoBar();
    void startParseCheck();
    void delayedParseCheck();
private:
    void initializePage();
    bool syncToWidgets();
    void syncToWidgets(const QDomDocument &doc);
    void syncToEditor();

    bool checkDocument(QDomDocument doc, QString *errorMessage, int *errorLine, int *errorColumn);
    bool setAndroidAppLibName(QDomDocument document, QDomElement activity, const QString &name);
    enum IconDPI { LowDPI, MediumDPI, HighDPI };
    QIcon icon(const QString &baseDir, IconDPI dpi);
    QString iconPath(const QString &baseDir, IconDPI dpi);
    void copyIcon(IconDPI dpi, const QString &baseDir, const QString &filePath);

    void updateInfoBar(const QString &errorMessage, int line, int column);
    void hideInfoBar();

    bool m_dirty; // indicates that we need to call syncToEditor()
    bool m_stayClean;
    bool m_setAppName;
    int m_errorLine;
    int m_errorColumn;

    QLineEdit *m_packageNameLineEdit;
    QLabel *m_packageNameWarningIcon;
    QLabel *m_packageNameWarning;
    QSpinBox *m_versionCode;
    QLineEdit *m_versionNameLinedit;

    // Application
    QLineEdit *m_appNameLineEdit;
    QLineEdit *m_targetLineEdit;
    QToolButton *m_lIconButton;
    QToolButton *m_mIconButton;
    QToolButton *m_hIconButton;
    QString m_lIconPath; // only set if the user changed the icon
    QString m_mIconPath;
    QString m_hIconPath;

    // Permissions
    PermissionsModel *m_permissionsModel;
    QListView *m_permissionsListView;
    QPushButton *m_addPermissionButton;
    QPushButton *m_removePermissionButton;
    QComboBox *m_permissionsComboBox;

    TextEditor::TextEditorActionHandler *m_ah;
    QWidget *m_overlayWidget;
    QTimer m_timerParseCheck;
};
} // namespace Internal
} // namespace Android


#endif // ANDROIDMANIFESTEDITORWIDGET_H
