// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>
#include <QThread>

#include <algorithm>
#include <cmath>

#include "HorizontalDock.h"
#include "dockwindows.h"
#include "hantekdso/dsosamples.h"

#include "scopesettings.h"
#include "sispinbox.h"
#include "utils/printutils.h"
#include "viewconstants.h"

static int row = 0;

template < typename... Args > struct SELECT {
    template < typename C, typename R > static constexpr auto OVERLOAD_OF( R ( C::*pmf )( Args... ) ) -> decltype( pmf ) {
        return pmf;
    }
};

HorizontalDock::HorizontalDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent )
    : QDockWidget( tr( "Horizontal" ), parent ), scope( scope ) {

    if ( scope->verboseLevel > 1 )
        qDebug() << " HorizontalDock::HorizontalDock()";

    // Initialize elements
    samplerateLabel = new QLabel( tr( "Samplerate" ) );
    samplerateSiSpinBox = new SiSpinBox( UNIT_SAMPLES );
    if ( scope->toolTipVisible )
        samplerateSiSpinBox->setToolTip( tr( "Effective samplerate, automatically selected from 'Timebase' setting" ) );
    samplerateSiSpinBox->setMinimum( 1 );
    samplerateSiSpinBox->setMaximum( 1e8 );
    samplerateSiSpinBox->setUnitPostfix( tr( "/s" ) );

    timebaseSteps << 1.0 << 2.0 << 5.0 << 10.0;

    timebaseLabel = new QLabel( tr( "Timebase" ) );
    timebaseSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    if ( scope->toolTipVisible )
        timebaseSiSpinBox->setToolTip( tr( "Time per horizontal screen division" ) );
    timebaseSiSpinBox->setSteps( timebaseSteps );
    timebaseSiSpinBox->setMinimum( 1e-9 );
    timebaseSiSpinBox->setMaximum( 1e3 );

    formatLabel = new QLabel( tr( "Format" ) );
    formatComboBox = new QComboBox();
    if ( scope->toolTipVisible )
        formatComboBox->setToolTip( tr( "Select signal over time or XY display" ) );
    for ( Dso::GraphFormat format : Dso::GraphFormatEnum )
        formatComboBox->addItem( Dso::graphFormatString( format ) );

    calfreqLabel = new QLabel( tr( "Calibration out" ) );
    calfreqSteps = spec->calfreqSteps;
    std::reverse( calfreqSteps.begin(), calfreqSteps.end() ); // put highest value on top of the list
    calfreqComboBox = new QComboBox();
    if ( scope->toolTipVisible )
        calfreqComboBox->setToolTip( tr( "Select the frequency of the calibration output, scroll for fast change" ) );
    for ( double calfreqStep : std::as_const( calfreqSteps ) )
        calfreqComboBox->addItem( valueToString( calfreqStep, UNIT_HERTZ, calfreqStep < 10e3 ? 2 : 0 ) );

    memoryDepthLabel = new QLabel( tr( "Memory depth" ) );
    memoryDepthComboBox = new QComboBox();
    if ( scope->toolTipVisible )
        memoryDepthComboBox->setToolTip(
            tr( "Max. samples per channel and acquisition, higher values allow higher samplerates for slow timebases" ) );
    for ( unsigned depth : MEMORY_DEPTHS )
        memoryDepthComboBox->addItem( valueToString( depth, UNIT_SAMPLES, 0 ) );

    oversamplingLabel = new QLabel( tr( "Oversampling" ) );
    oversamplingComboBox = new QComboBox();
    if ( scope->toolTipVisible )
        oversamplingComboBox->setToolTip( tr( "Reduced: less averaging of raw samples at slow samplerates, "
                                              "allows much higher samplerates with deep memory but shows more noise" ) );
    oversamplingComboBox->addItem( tr( "Normal" ) );
    oversamplingComboBox->addItem( tr( "Reduced" ) );
    hasReducedOversampling = !spec->fixedSampleRatesReduced.empty();

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth( 0, 64 );
    dockLayout->setColumnStretch( 1, 1 );
    dockLayout->setSpacing( DOCK_LAYOUT_SPACING );

    row = 0; // allows flexible shift up/down
    dockLayout->addWidget( timebaseLabel, row, 0 );
    dockLayout->addWidget( timebaseSiSpinBox, row++, 1 );
    dockLayout->addWidget( samplerateLabel, row, 0 );
    dockLayout->addWidget( samplerateSiSpinBox, row++, 1 );
    dockLayout->addWidget( memoryDepthLabel, row, 0 );
    dockLayout->addWidget( memoryDepthComboBox, row++, 1 );
    if ( hasReducedOversampling ) { // available for this model?
        dockLayout->addWidget( oversamplingLabel, row, 0 );
        dockLayout->addWidget( oversamplingComboBox, row++, 1 );
    } else {
        oversamplingLabel->hide();
        oversamplingComboBox->hide();
    }
    dockLayout->addWidget( formatLabel, row, 0 );
    dockLayout->addWidget( formatComboBox, row++, 1 );
    dockLayout->addWidget( calfreqLabel, row, 0 );
    dockLayout->addWidget( calfreqComboBox, row++, 1 );

    dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );

    // Load settings into GUI
    loadSettings( scope );

    // Connect signals and slots
    connect( samplerateSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             [ this ]( double samplerate ) { this->samplerateSelected( samplerate ); } );
    connect( timebaseSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             [ this ]( double timebase ) { this->timebaseSelected( timebase ); } );
    connect( formatComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
             [ this ]( int index ) { this->formatSelected( index ); } );
    connect( calfreqComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
             [ this ]( int index ) { this->calfreqIndexSelected( index ); } );
    connect( memoryDepthComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
             [ this ]( int index ) { this->memoryDepthIndexSelected( index ); } );
    connect( oversamplingComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
             [ this ]( int index ) { this->oversamplingIndexSelected( index ); } );
}


void HorizontalDock::loadSettings( DsoSettingsScope *scope ) {
    // Set values
    setMemoryDepth( scope->horizontal.memoryDepth );
    setReducedOversampling( scope->horizontal.reducedOversampling );
    setSamplerate( scope->horizontal.samplerate );
    setTimebase( scope->horizontal.timebase );
    setFormat( scope->horizontal.format );
    setCalfreq( scope->horizontal.calfreq );
}


void HorizontalDock::triggerModeChanged( Dso::TriggerMode mode ) {
    rollMode = mode == Dso::TriggerMode::ROLL;
    if ( rollMode )
        timebaseSiSpinBox->setMinimum( 0.2 );
    else
        timebaseSiSpinBox->setMinimum( 1e-9 );
    // roll mode uses a fixed buffer size, memory depth and oversampling setting do not apply
    memoryDepthComboBox->setEnabled( !rollMode );
    oversamplingComboBox->setEnabled( !rollMode && scope->horizontal.memoryDepth > unsigned( SAMPLESIZE ) );
    calculateSamplerateSteps( scope->horizontal.timebase );
}


/// \brief Don't close the dock, just hide it.
/// \param event The close event that should be handled.
void HorizontalDock::closeEvent( QCloseEvent *event ) {
    hide();
    event->accept();
}


double HorizontalDock::setSamplerate( double samplerate ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setSamplerate()" << samplerate;
    if ( scope->verboseLevel > 3 )
        qDebug() << "   ThreadID:" << QThread::currentThreadId();
    samplerateRequest = samplerate;
    QSignalBlocker blocker( timebaseSiSpinBox );
    timebaseSiSpinBox->setMaximum( scope->horizontal.maxTimebase );
    blocker = QSignalBlocker( samplerateSiSpinBox );
    samplerateSiSpinBox->setValue( samplerate );
    return samplerateSiSpinBox->value();
}


double HorizontalDock::setTimebase( double timebase ) {
    QSignalBlocker blocker( timebaseSiSpinBox );
    // timebaseSteps are repeated in each decade
    double decade = pow( 10, floor( log10( timebase ) ) );
    double vNorm = timebase / decade;
    for ( int i = 0; i < timebaseSteps.size() - 1; ++i ) {
        if ( timebaseSteps.at( i ) <= vNorm && vNorm < timebaseSteps.at( i + 1 ) ) {
            timebaseSiSpinBox->setValue( decade * timebaseSteps.at( i ) );
            break;
        }
    }
    calculateSamplerateSteps( timebase );
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setTimebase()" << timebase << "return" << timebaseSiSpinBox->value();
    return timebaseSiSpinBox->value();
}


int HorizontalDock::setFormat( Dso::GraphFormat format ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setFormat()" << format;
    QSignalBlocker blocker( formatComboBox );
    if ( format >= Dso::GraphFormat::TY && format <= Dso::GraphFormat::XY ) {
        formatComboBox->setCurrentIndex( format );
        return format;
    }
    return -1;
}


double HorizontalDock::setCalfreq( double calfreq ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setCalfreq()" << calfreq;
    auto indexIt = std::find( calfreqSteps.begin(), calfreqSteps.end(), calfreq );
    if ( indexIt == calfreqSteps.end() )
        return -1;
    int index = int( std::distance( calfreqSteps.begin(), indexIt ) );
    QSignalBlocker blocker( calfreqComboBox );
    calfreqComboBox->setCurrentIndex( index );
    return calfreq;
}


void HorizontalDock::setSamplerateLimits( double minimum, double maximum ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setSamplerateLimits()" << minimum << maximum;
    QSignalBlocker blocker( samplerateSiSpinBox );
    if ( bool( minimum ) )
        samplerateSiSpinBox->setMinimum( minimum );
    if ( bool( maximum ) )
        samplerateSiSpinBox->setMaximum( maximum );
}


void HorizontalDock::setSamplerateSteps( int mode, const QList< double > steps ) {
    if ( samplerateSteps.size() == steps.size() ) // no action needed
        return;
    if ( scope->verboseLevel > 3 )
        qDebug() << "   HDock::setSamplerateSteps()" << steps;
    else if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::setSamplerateSteps()" << steps.first() << "..." << steps.last();
    samplerateSteps = steps;
    // Assume that method is invoked for fixed samplerate devices only
    QSignalBlocker samplerateBlocker( samplerateSiSpinBox );
    samplerateSiSpinBox->setMode( mode );
    samplerateSiSpinBox->setSteps( steps );
    samplerateSiSpinBox->setMinimum( steps.first() );
    samplerateSiSpinBox->setMaximum( steps.last() );
    // Make reasonable adjustments to the timebase spinbox
    QSignalBlocker timebaseBlocker( timebaseSiSpinBox );
    timebaseSiSpinBox->setMinimum( pow( 10, floor( log10( 1.0 / steps.last() ) ) ) );
    calculateSamplerateSteps( timebaseSiSpinBox->value() );
}


/// \brief Called when the samplerate spinbox changes its value.
/// \param samplerate The samplerate in samples/second.
void HorizontalDock::samplerateSelected( double samplerate ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::samplerateSelected()" << samplerate;
    scope->horizontal.samplerate = samplerate;
    emit samplerateChanged( samplerate );
}


/// \brief Called when the timebase spinbox changes its value.
/// \param timebase The timebase in seconds.
void HorizontalDock::timebaseSelected( double timebase ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::timebaseSelected()" << timebase;
    if ( scope->verboseLevel > 3 )
        qDebug() << "   ThreadID:" << QThread::currentThreadId();
    scope->horizontal.timebase = timebase;
    calculateSamplerateSteps( timebase );
    emit timebaseChanged( timebase );
}


void HorizontalDock::calculateSamplerateSteps( double timebase ) {
    int size = samplerateSteps.size();
    if ( size ) {
        // search appropriate min & max sample rate
        double min = samplerateSteps[ 0 ];
        double max = samplerateSteps[ 0 ];
        for ( int id = 0; id < size; ++id ) {
            double sRate = samplerateSteps[ id ];
            if ( scope->verboseLevel > 3 )
                qDebug() << "   sRate, sRate*timebase" << sRate << sRate * timebase;
            // min must be < maxRate
            // find minimal samplerate to get at least this number of samples per div
            if ( id < size - 1 && sRate * timebase <= 10 ) { // 10 samples/div
                min = sRate;
            }
            // max must be > minRate
            // find max samplesrate to get not more then this number of samples per div
            // use max. half of the memory depth to get enough samples for two full screens (to ensure triggering)
            unsigned depth = rollMode ? unsigned( SAMPLESIZE ) : scope->horizontal.memoryDepth;
            if ( id && sRate * timebase <= depth / 2 / DIVS_TIME ) { // 1000 samples/div @ 20 kS
                max = sRate;
            }
        }
        min = qMax( min, qMin( 10e3, max ) ); // not less than 10kS unless max is smaller
        if ( scope->verboseLevel > 2 )
            qDebug() << "  HDock::calculateSamplerateSteps()" << timebase << min << max;
        setSamplerateLimits( min, max );
        // update samplerate if the requested value was limited
        if ( samplerateRequest > samplerateSiSpinBox->value() )
            setSamplerate( samplerateRequest );
    }
}


/// \brief Called when the format combo box changes its value.
/// \param index The index of the combo box item.
void HorizontalDock::formatSelected( int index ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::formatSelected()" << index;
    scope->horizontal.format = Dso::GraphFormat( index );
    emit formatChanged( scope->horizontal.format );
}


/// \brief Called when the calfreq combobox changes its value.
/// \param index The item index.
void HorizontalDock::calfreqIndexSelected( int index ) {
    double calfreq = calfreqSteps[ index ];
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::calfreqIndex Selected()" << index << calfreq;
    scope->horizontal.calfreq = calfreq;
    emit calfreqChanged( calfreq );
}


void HorizontalDock::setMemoryDepth( unsigned depth ) {
    auto it = std::find( MEMORY_DEPTHS.begin(), MEMORY_DEPTHS.end(), depth );
    if ( it == MEMORY_DEPTHS.end() )
        return;
    QSignalBlocker blocker( memoryDepthComboBox );
    memoryDepthComboBox->setCurrentIndex( int( it - MEMORY_DEPTHS.begin() ) );
    oversamplingComboBox->setEnabled( !rollMode && depth > unsigned( SAMPLESIZE ) ); // only for deep memory
}


void HorizontalDock::setReducedOversampling( bool reduced ) {
    QSignalBlocker blocker( oversamplingComboBox );
    oversamplingComboBox->setCurrentIndex( reduced ? 1 : 0 );
}


/// \brief Called when the oversampling combobox changes its value.
/// \param index 0: normal, 1: reduced.
void HorizontalDock::oversamplingIndexSelected( int index ) {
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::oversamplingIndexSelected()" << index;
    scope->horizontal.reducedOversampling = index == 1;
    emit reducedOversamplingChanged( scope->horizontal.reducedOversampling );
}


/// \brief Called when the memory depth combobox changes its value.
/// \param index The item index.
void HorizontalDock::memoryDepthIndexSelected( int index ) {
    unsigned depth = MEMORY_DEPTHS[ size_t( index ) ];
    if ( scope->verboseLevel > 2 )
        qDebug() << "  HDock::memoryDepthIndexSelected()" << index << depth;
    scope->horizontal.memoryDepth = depth;
    oversamplingComboBox->setEnabled( !rollMode && depth > unsigned( SAMPLESIZE ) ); // only for deep memory
    calculateSamplerateSteps( scope->horizontal.timebase );                          // the samplerate limit depends on the depth
    emit memoryDepthChanged( depth );
}
