// SPDX-License-Identifier: GPL-2.0-or-later

#include "scopelayout.h"

#include <QAbstractItemView>
#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "docks/HorizontalDock.h"
#include "docks/TriggerDock.h"
#include "docks/VoltageDock.h"
#include "hantekdso/controlspecification.h"
#include "hantekdso/enums.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewsettings.h"

static const int UPDATE_INTERVAL = 200; // ms, the bars show the current settings

// re-evaluate style sheet rules that depend on dynamic properties
static void repolish( QWidget *widget ) {
    widget->style()->unpolish( widget );
    widget->style()->polish( widget );
}


////////////////////////////////////////////////////////////////////////////////
// ScopeTopBar

ScopeTopBar::ScopeTopBar( DsoSettingsScope *scope, HorizontalDock *horizontalDock, TriggerDock *triggerDock,
                          QAction *samplingAction, const QList< QAction * > &toolActions, const QList< QAction * > &fileActions,
                          QWidget *parent )
    : QToolBar( tr( "Scope bar" ), parent ), scope( scope ), triggerDock( triggerDock ), samplingAction( samplingAction ) {
    setObjectName( "scopeTopBar" ); // needed by QMainWindow::saveState()
    setMovable( false );
    setFloatable( false );
    setToolButtonStyle( Qt::ToolButtonTextUnderIcon );

    // start / stop
    runButton = new QPushButton();
    runButton->setObjectName( "runButton" );
    runButton->setToolTip( samplingAction->toolTip() );
    runButton->setFocusPolicy( Qt::NoFocus ); // keep <space> as start/stop shortcut
    connect( runButton, &QPushButton::clicked, samplingAction, &QAction::trigger );
    addWidget( runButton );

    for ( QAction *action : toolActions )
        addAction( action );

    // timebase: - value +
    QWidget *timebase = new QWidget();
    QHBoxLayout *timebaseLayout = new QHBoxLayout( timebase );
    timebaseLayout->setContentsMargins( 0, 0, 0, 0 );
    timebaseLayout->setSpacing( 4 );
    QPushButton *slower = new QPushButton( "−" );
    QPushButton *faster = new QPushButton( "+" );
    for ( QPushButton *button : { slower, faster } ) {
        button->setObjectName( "stepButton" );
        button->setFocusPolicy( Qt::NoFocus );
    }
    slower->setToolTip( tr( "Faster timebase (less time per division)" ) );
    faster->setToolTip( tr( "Slower timebase (more time per division)" ) );
    timebaseButton = new QToolButton();
    timebaseButton->setObjectName( "valueButton" );
    timebaseButton->setToolTip( tr( "Show the horizontal settings (memory depth, oversampling, ...)" ) );
    connect( slower, &QPushButton::clicked, horizontalDock, [ horizontalDock ]() { horizontalDock->requestTimebaseStep( -1 ); } );
    connect( faster, &QPushButton::clicked, horizontalDock, [ horizontalDock ]() { horizontalDock->requestTimebaseStep( 1 ); } );
    connect( timebaseButton, &QToolButton::clicked, horizontalDock,
             [ horizontalDock ]() { horizontalDock->setVisible( !horizontalDock->isVisible() ); } );
    timebaseLayout->addWidget( slower );
    timebaseLayout->addWidget( timebaseButton );
    timebaseLayout->addWidget( faster );
    addWidget( group( tr( "Timebase" ), timebase ) );

    // trigger summary, click shows the trigger dock
    triggerButton = new QToolButton();
    triggerButton->setObjectName( "valueButton" );
    triggerButton->setToolTip( tr( "Show the trigger settings" ) );
    connect( triggerButton, &QToolButton::clicked, triggerDock,
             [ triggerDock ]() { triggerDock->setVisible( !triggerDock->isVisible() ); } );
    addWidget( group( tr( "Trigger" ), triggerButton ) );

    // trigger mode as segmented buttons
    QWidget *modes = new QWidget();
    QHBoxLayout *modesLayout = new QHBoxLayout( modes );
    modesLayout->setContentsMargins( 0, 0, 0, 0 );
    modesLayout->setSpacing( 2 );
    modeGroup = new QButtonGroup( this );
    int id = 0;
    for ( Dso::TriggerMode mode : triggerDock->triggerModes() ) {
        QPushButton *button = new QPushButton( Dso::triggerModeString( mode ) );
        button->setObjectName( "segment" );
        button->setCheckable( true );
        button->setFocusPolicy( Qt::NoFocus );
        modeGroup->addButton( button, id++ );
        modesLayout->addWidget( button );
    }
    connect( modeGroup, &QButtonGroup::idClicked, triggerDock,
             [ triggerDock ]( int id ) { triggerDock->requestMode( triggerDock->triggerModes()[ size_t( id ) ] ); } );
    addWidget( group( tr( "Mode" ), modes ) );

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    addWidget( spacer );
    for ( QAction *action : fileActions )
        addAction( action );

    QTimer *timer = new QTimer( this );
    connect( timer, &QTimer::timeout, this, &ScopeTopBar::update );
    timer->start( UPDATE_INTERVAL );
    update();
}


QFrame *ScopeTopBar::group( const QString &title, QWidget *content ) {
    QFrame *frame = new QFrame();
    frame->setObjectName( "barGroup" );
    QVBoxLayout *layout = new QVBoxLayout( frame );
    layout->setContentsMargins( 8, 2, 8, 4 );
    layout->setSpacing( 0 );
    QLabel *label = new QLabel( title );
    label->setObjectName( "barGroupTitle" );
    layout->addWidget( label );
    layout->addWidget( content );
    return frame;
}


void ScopeTopBar::update() {
    const bool running = samplingAction->isChecked();
    if ( runButton->property( "running" ).toBool() != running || runButton->text().isEmpty() ) {
        runButton->setText( running ? tr( "Running" ) : tr( "Stopped" ) );
        runButton->setProperty( "running", running );
        repolish( runButton );
    }
    runButton->setEnabled( samplingAction->isEnabled() );

    timebaseButton->setText( valueToString( scope->horizontal.timebase, UNIT_SECONDS, 0 ) + tr( "/div" ) );

    const int source = scope->trigger.source;
    if ( source >= 0 && size_t( source ) < scope->voltage.size() )
        triggerButton->setText( QString( "%1  %2  %3" )
                                    .arg( scope->voltage[ size_t( source ) ].name, Dso::slopeString( scope->trigger.slope ),
                                          valueToString( scope->voltage[ size_t( source ) ].trigger, UNIT_VOLTS, 3 ) ) );

    const auto &modes = triggerDock->triggerModes();
    for ( size_t index = 0; index < modes.size(); ++index )
        if ( modes[ index ] == scope->trigger.mode && modeGroup->checkedId() != int( index ) )
            modeGroup->button( int( index ) )->setChecked( true );
}


////////////////////////////////////////////////////////////////////////////////
// ChannelSidebar

ChannelSidebar::ChannelSidebar( DsoSettingsScope *scope, const DsoSettingsView *view, const Dso::ControlSpecification *spec,
                                VoltageDock *voltageDock, const QList< QAction * > &menuActions, QWidget *parent )
    : QFrame( parent ), scope( scope ), view( view ), spec( spec ), voltageDock( voltageDock ) {
    setObjectName( "channelSidebar" );
    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setContentsMargins( 8, 8, 8, 8 );
    layout->setSpacing( 6 );

    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        Card card;
        card.channel = channel;

        // [-] [ name coupling / gain ] [+]
        QWidget *row = new QWidget();
        QHBoxLayout *rowLayout = new QHBoxLayout( row );
        rowLayout->setContentsMargins( 0, 0, 0, 0 );
        rowLayout->setSpacing( 4 );
        QPushButton *less = new QPushButton( "−" );
        QPushButton *more = new QPushButton( "+" );
        for ( QPushButton *button : { less, more } ) {
            button->setObjectName( "stepButton" );
            button->setFocusPolicy( Qt::NoFocus );
            button->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
        }
        less->setToolTip( tr( "Smaller voltage range per division" ) );
        more->setToolTip( tr( "Larger voltage range per division" ) );
        card.body = new QPushButton();
        card.body->setObjectName( "channelCard" );
        card.body->setFocusPolicy( Qt::NoFocus );
        card.body->setToolTip( tr( "Click to show the channel settings" ) );
        rowLayout->addWidget( less );
        rowLayout->addWidget( card.body, 1 );
        rowLayout->addWidget( more );
        layout->addWidget( row );
        connect( less, &QPushButton::clicked, voltageDock,
                 [ voltageDock, channel ]() { voltageDock->requestGainStep( channel, -1 ); } );
        connect( more, &QPushButton::clicked, voltageDock,
                 [ voltageDock, channel ]() { voltageDock->requestGainStep( channel, 1 ); } );

        // details, hidden until the card is clicked
        card.details = new QFrame();
        card.details->setObjectName( "channelDetails" );
        QGridLayout *grid = new QGridLayout( card.details );
        grid->setContentsMargins( 8, 6, 8, 6 );
        grid->setHorizontalSpacing( 6 );
        grid->setVerticalSpacing( 4 );
        card.details->hide();
        layout->addWidget( card.details );
        connect( card.body, &QPushButton::clicked, card.details,
                 [ details = card.details ]() { details->setVisible( !details->isVisible() ); } );

        int gridRow = 0;
        card.onOff = segments( card.details, tr( "Channel" ), { tr( "ON" ), tr( "OFF" ) }, gridRow++ );
        connect( card.onOff, &QButtonGroup::idClicked, voltageDock,
                 [ voltageDock, channel ]( int id ) { voltageDock->requestUsed( channel, id == 0 ); } );
        card.coupling = nullptr;
        card.mathMode = nullptr;
        card.invert = nullptr;
        card.probe = nullptr;
        if ( channel < spec->channels ) { // real channel
            card.coupling = segments( card.details, tr( "Coupling" ), voltageDock->miscTexts( channel ), gridRow++ );
            connect( card.coupling, &QButtonGroup::idClicked, voltageDock,
                     [ voltageDock, channel ]( int id ) { voltageDock->requestMisc( channel, id ); } );
            card.invert = segments( card.details, tr( "Invert" ), { tr( "OFF" ), tr( "ON" ) }, gridRow++ );
            connect( card.invert, &QButtonGroup::idClicked, voltageDock,
                     [ voltageDock, channel ]( int id ) { voltageDock->requestInverted( channel, id == 1 ); } );
            card.probe = segments( card.details, tr( "Probe" ), { "×1", "×10" }, gridRow++ );
            connect( card.probe, &QButtonGroup::idClicked, voltageDock,
                     [ voltageDock, channel ]( int id ) { voltageDock->requestAttn( channel, id == 1 ? 10 : 1 ); } );
        } else { // math channel: many operations -> combobox
            QLabel *label = new QLabel( tr( "Function" ) );
            label->setObjectName( "detailsTitle" );
            card.mathMode = new QComboBox();
            card.mathMode->addItems( voltageDock->miscTexts( channel ) );
            grid->addWidget( label, gridRow, 0 );
            grid->addWidget( card.mathMode, gridRow++, 1 );
            connect( card.mathMode, QOverload< int >::of( &QComboBox::activated ), voltageDock,
                     [ voltageDock, channel ]( int index ) { voltageDock->requestMisc( channel, index ); } );
        }
        cards.push_back( card );
    }

    // menu
    layout->addSpacing( 8 );
    for ( QAction *action : menuActions ) { // push buttons allow left aligned text
        QPushButton *button = new QPushButton( action->icon(), action->text().remove( '&' ) );
        button->setObjectName( "menuButton" );
        button->setToolTip( action->toolTip() );
        button->setCheckable( action->isCheckable() );
        button->setChecked( action->isChecked() );
        button->setFocusPolicy( Qt::NoFocus );
        connect( button, &QPushButton::clicked, action, &QAction::trigger );
        connect( action, &QAction::toggled, button, &QPushButton::setChecked ); // also changed by menu or shortcut
        layout->addWidget( button );
    }
    layout->addStretch( 1 );
    setFixedWidth( 210 );

    QTimer *timer = new QTimer( this );
    connect( timer, &QTimer::timeout, this, &ChannelSidebar::update );
    timer->start( UPDATE_INTERVAL );
    update();
}


QButtonGroup *ChannelSidebar::segments( QFrame *parent, const QString &title, const QStringList &texts, int row ) {
    QGridLayout *grid = static_cast< QGridLayout * >( parent->layout() );
    QLabel *label = new QLabel( title );
    label->setObjectName( "detailsTitle" );
    QWidget *buttons = new QWidget();
    QHBoxLayout *buttonsLayout = new QHBoxLayout( buttons );
    buttonsLayout->setContentsMargins( 0, 0, 0, 0 );
    buttonsLayout->setSpacing( 2 );
    QButtonGroup *group = new QButtonGroup( parent );
    int id = 0;
    for ( const QString &text : texts ) {
        QPushButton *button = new QPushButton( text );
        button->setObjectName( "segment" );
        button->setCheckable( true );
        button->setFocusPolicy( Qt::NoFocus );
        group->addButton( button, id++ );
        buttonsLayout->addWidget( button );
    }
    grid->addWidget( label, row, 0 );
    grid->addWidget( buttons, row, 1 );
    return group;
}


static void check( QButtonGroup *group, int id ) {
    if ( group && group->button( id ) && group->checkedId() != id )
        group->button( id )->setChecked( true );
}


void ChannelSidebar::update() {
    for ( Card &card : cards ) {
        const ChannelID channel = card.channel;
        const DsoSettingsScopeVoltage &voltage = scope->voltage[ channel ];
        const bool on = voltage.visible;
        QString info;
        if ( channel < spec->channels )
            info = QString( "%1  ×%2" )
                       .arg( Dso::couplingString( Dso::Coupling( voltage.couplingOrMathIndex ) ) )
                       .arg( voltage.probeAttn );
        else if ( card.mathMode && int( voltage.couplingOrMathIndex ) < card.mathMode->count() )
            info = card.mathMode->itemText( int( voltage.couplingOrMathIndex ) );
        card.body->setText( QString( "%1   %2\n%3" )
                                .arg( voltage.name, info, on ? voltageDock->gainText( channel ) + tr( "/div" ) : tr( "Off" ) ) );

        // card colored with the channel color when on, dimmed when off
        QColor color = channel < view->screen.voltage.size() ? view->screen.voltage[ channel ] : QColor( Qt::gray );
        QString style = on ? QString( "QPushButton#channelCard { background: %1; color: %2; border-color: %1; }" )
                                 .arg( color.name(), color.lightness() > 140 ? "#0d1117" : "#ffffff" )
                           : QString( "QPushButton#channelCard { color: %1; }" ).arg( color.darker( 150 ).name() );
        if ( style != card.style ) {
            card.body->setStyleSheet( style );
            card.style = style;
        }
        check( card.onOff, on ? 0 : 1 );
        check( card.coupling, int( voltage.couplingOrMathIndex ) );
        check( card.invert, voltage.inverted ? 1 : 0 );
        if ( card.probe ) { // x1 / x10, other values (typed in the voltage dock) check no button
            int id = voltage.probeAttn == 1.0 ? 0 : voltage.probeAttn == 10.0 ? 1 : -1;
            if ( id >= 0 )
                check( card.probe, id );
            else if ( card.probe->checkedButton() ) {
                card.probe->setExclusive( false );
                card.probe->checkedButton()->setChecked( false );
                card.probe->setExclusive( true );
            }
        }
        if ( card.mathMode && card.mathMode->currentIndex() != int( voltage.couplingOrMathIndex ) &&
             !card.mathMode->view()->isVisible() )
            card.mathMode->setCurrentIndex( int( voltage.couplingOrMathIndex ) );
    }
}
