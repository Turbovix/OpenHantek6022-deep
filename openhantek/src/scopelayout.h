// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFrame>
#include <QToolBar>
#include <vector>

#include "hantekprotocol/types.h"

class QAction;
class QButtonGroup;
class QComboBox;
class QLabel;
class QPushButton;
class QToolButton;
class HorizontalDock;
class TriggerDock;
class VoltageDock;
struct DsoSettingsScope;
struct DsoSettingsView;
namespace Dso {
struct ControlSpecification;
}

/// \brief Top bar of the alternative "dark blue" layout: big run/stop button, timebase,
/// trigger summary and trigger mode, quick actions. It operates the existing docks and actions,
/// so all settings take the usual way.
class ScopeTopBar : public QToolBar {
    Q_OBJECT

  public:
    ScopeTopBar( DsoSettingsScope *scope, HorizontalDock *horizontalDock, TriggerDock *triggerDock, QAction *samplingAction,
                 const QList< QAction * > &toolActions, const QList< QAction * > &fileActions, QWidget *parent );

  private:
    void update();
    QFrame *group( const QString &title, QWidget *content );

    DsoSettingsScope *scope;
    TriggerDock *triggerDock;
    QAction *samplingAction;
    QPushButton *runButton;
    QToolButton *timebaseButton;
    QToolButton *triggerButton;
    QButtonGroup *modeGroup;
};


/// \brief Left sidebar of the "dark blue" layout: one colored card per channel with
/// gain -/+ buttons and an expandable settings panel, followed by a menu of actions.
class ChannelSidebar : public QFrame {
    Q_OBJECT

  public:
    ChannelSidebar( DsoSettingsScope *scope, const DsoSettingsView *view, const Dso::ControlSpecification *spec,
                    VoltageDock *voltageDock, const QList< QAction * > &menuActions, QWidget *parent );

  private:
    struct Card {
        ChannelID channel;
        QPushButton *body;      ///< name, coupling and gain, click to show the details
        QFrame *details;        ///< channel settings
        QButtonGroup *onOff;    ///< 0: on, 1: off
        QButtonGroup *coupling; ///< real channels: DC / AC
        QComboBox *mathMode;    ///< math channel: operation
        QButtonGroup *invert;   ///< 0: off, 1: on
        QButtonGroup *probe;    ///< 0: x1, 1: x10
        QString style;          ///< last applied style sheet of the body
    };
    void update();
    QButtonGroup *segments( QFrame *parent, const QString &title, const QStringList &texts, int row );

    DsoSettingsScope *scope;
    const DsoSettingsView *view;
    const Dso::ControlSpecification *spec;
    VoltageDock *voltageDock;
    std::vector< Card > cards;
};
