// SPDX-License-Identifier: GPL-2.0-or-later

#include "darkbluetheme.h"

#include <QApplication>
#include <QColor>
#include <QFontDatabase>
#include <QPalette>

namespace DarkBlueTheme {

// Primer dark palette (https://primer.style, MIT license)
static const QColor canvas( 0x0d, 0x11, 0x17 );    // window background
static const QColor overlay( 0x16, 0x1b, 0x22 );   // panels, popups
static const QColor control( 0x21, 0x26, 0x2d );   // buttons, inputs
static const QColor border( 0x30, 0x36, 0x3d );    // borders, hover
static const QColor fgDefault( 0xe6, 0xed, 0xf3 ); // text
static const QColor fgMuted( 0x8b, 0x94, 0x9e );   // secondary text
static const QColor accent( 0x1f, 0x6f, 0xeb );    // selection, checked state
static const QColor link( 0x58, 0xa6, 0xff );

static const char *styleSheet = R"(
QMainWindow, QDialog { background: #0d1117; }
QMainWindow::separator { background: #0d1117; width: 6px; height: 6px; }
QToolTip { background: #161b22; color: #e6edf3; border: 1px solid #30363d; border-radius: 6px; padding: 4px; }

QMenuBar { background: #0d1117; padding: 2px; }
QMenuBar::item { padding: 4px 10px; border-radius: 6px; }
QMenuBar::item:selected { background: #21262d; }
QMenu { background: #161b22; border: 1px solid #30363d; border-radius: 6px; padding: 4px; }
QMenu::item { padding: 5px 24px 5px 10px; border-radius: 4px; }
QMenu::item:selected { background: #1f6feb; color: #ffffff; }
QMenu::item:disabled { color: #6e7681; }
QMenu::separator { height: 1px; background: #30363d; margin: 4px 6px; }

QToolBar { background: #0d1117; border: none; spacing: 4px; padding: 4px; }
QToolButton { background: #21262d; border: 1px solid #30363d; border-radius: 6px; padding: 4px; }
QToolButton:hover { background: #30363d; border-color: #8b949e; }
QToolButton:checked { background: #1f6feb; border-color: #388bfd; }

QDockWidget { titlebar-close-icon: none; color: #8b949e; }
QDockWidget::title { background: #0d1117; padding: 6px 8px; text-align: left; }
QDockWidget > QWidget { background: #161b22; border: 1px solid #30363d; border-radius: 6px; }

QPushButton { background: #21262d; border: 1px solid #30363d; border-radius: 6px; padding: 5px 14px; }
QPushButton:hover { background: #30363d; border-color: #8b949e; }
QPushButton:pressed { background: #161b22; }
QPushButton:checked, QPushButton:default { background: #1f6feb; border-color: #388bfd; color: #ffffff; }
QPushButton:disabled { color: #6e7681; }

QComboBox, QAbstractSpinBox, QLineEdit {
    background: #21262d; border: 1px solid #30363d; border-radius: 6px; padding: 3px 6px; min-height: 18px;
    selection-background-color: #1f6feb; }
QComboBox:hover, QAbstractSpinBox:hover, QLineEdit:hover { border-color: #8b949e; }
QComboBox:focus, QAbstractSpinBox:focus, QLineEdit:focus { border-color: #1f6feb; }
QComboBox:disabled, QAbstractSpinBox:disabled { color: #6e7681; background: #161b22; }
QAbstractSpinBox { padding-right: 18px; }
QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
    subcontrol-origin: border; width: 16px; border: none; background: transparent; }
QAbstractSpinBox::up-button { subcontrol-position: top right; }
QAbstractSpinBox::down-button { subcontrol-position: bottom right; }
QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background: #30363d; border-radius: 4px; }
QAbstractSpinBox::up-arrow { image: url(:/images/darkblue/up.svg); width: 10px; height: 10px; }
QAbstractSpinBox::down-arrow { image: url(:/images/darkblue/down.svg); width: 10px; height: 10px; }
QComboBox { padding-right: 20px; }
QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 18px; border: none; }
QComboBox::down-arrow { image: url(:/images/darkblue/down.svg); width: 10px; height: 10px; }
QComboBox QAbstractItemView {
    background: #161b22; border: 1px solid #30363d; selection-background-color: #1f6feb; outline: none; }

QCheckBox { spacing: 6px; }
QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #30363d; border-radius: 4px; background: #0d1117; }
QCheckBox::indicator:hover { border-color: #8b949e; }
QCheckBox::indicator:checked { background: #1f6feb; border-color: #1f6feb; image: url(:/images/darkblue/check.svg); }

QTabWidget::pane { border: 1px solid #30363d; border-radius: 6px; }
QTabBar::tab { background: #0d1117; color: #8b949e; padding: 6px 12px; border-bottom: 2px solid transparent; }
QTabBar::tab:selected { color: #e6edf3; border-bottom: 2px solid #f78166; }
QGroupBox { border: 1px solid #30363d; border-radius: 6px; margin-top: 12px; padding-top: 6px; }
QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: #8b949e; }

QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar:horizontal { background: transparent; height: 10px; }
QScrollBar::handle { background: #30363d; border-radius: 5px; min-height: 24px; min-width: 24px; }
QScrollBar::handle:hover { background: #484f58; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QStatusBar { background: #0d1117; color: #8b949e; }
)";


void apply( QApplication &app ) {
    QPalette palette;
    palette.setColor( QPalette::Window, canvas );
    palette.setColor( QPalette::WindowText, fgDefault );
    palette.setColor( QPalette::Base, control );
    palette.setColor( QPalette::AlternateBase, overlay );
    palette.setColor( QPalette::Text, fgDefault );
    palette.setColor( QPalette::PlaceholderText, fgMuted );
    palette.setColor( QPalette::Button, control );
    palette.setColor( QPalette::ButtonText, fgDefault );
    palette.setColor( QPalette::BrightText, Qt::white );
    palette.setColor( QPalette::Light, border );
    palette.setColor( QPalette::Midlight, border );
    palette.setColor( QPalette::Mid, control );
    palette.setColor( QPalette::Dark, canvas );
    palette.setColor( QPalette::Shadow, Qt::black );
    palette.setColor( QPalette::Highlight, accent );
    palette.setColor( QPalette::HighlightedText, Qt::white );
    palette.setColor( QPalette::Link, link );
    palette.setColor( QPalette::LinkVisited, link );
    palette.setColor( QPalette::ToolTipBase, overlay );
    palette.setColor( QPalette::ToolTipText, fgDefault );
    palette.setColor( QPalette::Disabled, QPalette::Text, fgMuted );
    palette.setColor( QPalette::Disabled, QPalette::WindowText, fgMuted );
    palette.setColor( QPalette::Disabled, QPalette::ButtonText, fgMuted );
    app.setPalette( palette );
    app.setStyleSheet( styleSheet );
}


QString loadFont() {
    QString family;
    for ( const char *file : { ":/fonts/selawk.ttf", ":/fonts/selawksb.ttf", ":/fonts/selawkb.ttf" } ) {
        int id = QFontDatabase::addApplicationFont( file );
        if ( id >= 0 && family.isEmpty() && !QFontDatabase::applicationFontFamilies( id ).isEmpty() )
            family = QFontDatabase::applicationFontFamilies( id ).first();
    }
    return family;
}

} // namespace DarkBlueTheme
