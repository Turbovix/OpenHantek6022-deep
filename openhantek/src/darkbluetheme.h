// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QColor>
#include <QString>
#include <vector>

class QApplication;

/// \brief Dark blue theme: flat dark UI with rounded controls and a blue accent.
/// Colors follow the dark palette of GitHub's Primer design system (MIT license),
/// the default font is Microsoft's Selawik (SIL Open Font License, see res/fonts).
namespace DarkBlueTheme {

/// Applies palette and style sheet to the application.
void apply( QApplication &app );

/// Loads the bundled fonts, returns the font family to use (empty on error).
QString loadFont();

/// Scope screen colors matching the theme: black screen, dim grid, no bright border
const QColor screenBackground( 0x00, 0x00, 0x00 );
const QColor screenGrid( 0x2d, 0x33, 0x3b );
const QColor screenAxes( 0x6e, 0x76, 0x81 );
const QColor screenBorder( 0x30, 0x36, 0x3d );
const QColor screenMarkers( 0x8b, 0x94, 0x9e );
const QColor screenText( 0xc9, 0xd1, 0xd9 );
/// Channel colors CH1, CH2, MATH (yellow, cyan, magenta); the spectrum uses darker variants
const std::vector< QColor > screenVoltage = { QColor( 0xf2, 0xcc, 0x60 ), QColor( 0x1f, 0xd6, 0xd6 ), QColor( 0xd6, 0x3f, 0xd6 ) };

} // namespace DarkBlueTheme
