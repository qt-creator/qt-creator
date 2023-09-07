// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ACTIONEDITORDIALOG_H
#define ACTIONEDITORDIALOG_H

#include <bindingeditor/abstracteditordialog.h>
#include <qmljs/parser/qmljsast_p.h>

#include <QStackedLayout>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class ActionEditorDialog : public AbstractEditorDialog
{
    Q_OBJECT

public:
    enum ConnectionType { Action, Assignment };
    Q_ENUM(ConnectionType)

    enum ComboBox { Type, TargetItem, TargetProperty, SourceItem, SourceProperty };
    Q_ENUM(ComboBox)

    class PropertyOption
    {
    public:
        PropertyOption(const QString &n, const TypeName &t, bool writeable = true)
            : name(n)
            , type(t)
            , isWriteable(writeable)
        {}

        bool operator==(const QString &value) const { return value == name; }
        bool operator==(const PropertyOption &value) const { return value.name == name; }

        QString name;
        TypeName type;
        bool isWriteable;
    };

    class SingletonOption
    {
    public:
        SingletonOption() {}
        SingletonOption(const QString &value) { item = value; }

        bool containsType(const TypeName &t) const
        {
            for (const auto &p : properties) {
                if (t == p.type || (isNumeric(t) && isNumeric(p.type)))
                    return true;
            }

            return false;
        }

        bool hasWriteableProperties() const
        {
            for (const auto &p : properties) {
                if (p.isWriteable)
                    return true;
            }

            return false;
        }

        bool operator==(const QString &value) const { return value == item; }
        bool operator==(const SingletonOption &value) const { return value.item == item; }

        QString item;
        QList<PropertyOption> properties;
    };

    class ConnectionOption : public SingletonOption
    {
    public:
        ConnectionOption(const QString &value) : SingletonOption(value) {}

        QStringList methods;
    };


    ActionEditorDialog(QWidget *parent = nullptr);
    ~ActionEditorDialog() override;

    void adjustProperties() override;

    void setAllConnections(const QList<ConnectionOption> &connections,
                           const QList<SingletonOption> &singeltons,
                           const QStringList &states);

    void updateComboBoxes(int idx, ComboBox type);

    void showControls(bool show);
    void setMultiline(bool multiline);

private:
    void setupUIComponents();

    void setType(ConnectionType type);

    void fillAndSetTargetItem(const QString &value, bool useDefault = false);
    void fillAndSetTargetProperty(const QString &value, bool useDefault = false);

    void fillAndSetSourceItem(const QString &value, bool useDefault = false);
    void fillAndSetSourceProperty(const QString &value,
                                  QmlJS::AST::Node::Kind kind = QmlJS::AST::Node::Kind::Kind_Undefined,
                                  bool useDefault = false);

    void insertAndSetUndefined(QComboBox *comboBox);

private:
    QComboBox *m_comboBoxType = nullptr;

    QStackedLayout *m_stackedLayout = nullptr;

    QWidget *m_actionPlaceholder = nullptr;
    QWidget *m_assignmentPlaceholder = nullptr;

    QHBoxLayout *m_actionLayout = nullptr;
    QHBoxLayout *m_assignmentLayout = nullptr;

    QComboBox *m_actionTargetItem = nullptr;
    QComboBox *m_actionMethod = nullptr;

    QComboBox *m_assignmentTargetItem = nullptr;
    QComboBox *m_assignmentTargetProperty = nullptr;
    QComboBox *m_assignmentSourceItem = nullptr;
    QComboBox *m_assignmentSourceProperty = nullptr; // Value

    QList<ConnectionOption> m_connections;
    QList<SingletonOption> m_singletons;
    QStringList m_states;

    const TypeName specificItem = {"specific"};
    const TypeName singletonItem = {"singleton"};
};

}

#endif //ACTIONEDITORDIALOG_H
