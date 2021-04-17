#include "IEBusAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


IEBusAnalyzerSettings::IEBusAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mStartBitWidth( 170 ),
	mBitWidth( 40 )
{
	mInputChannelInterface = new std::unique_ptr< AnalyzerSettingInterfaceChannel >();
	mStartBitWidthInterface = new std::unique_ptr< AnalyzerSettingInterfaceInteger >();
	mBitWidthInterface = new std::unique_ptr< AnalyzerSettingInterfaceInteger >();
	mInputChannelInterface->reset( new AnalyzerSettingInterfaceChannel() );
	(*mInputChannelInterface)->SetTitleAndTooltip( "Receive Channel", "Slave Receive Channel" );
	(*mInputChannelInterface)->SetChannel( mInputChannel );

	mStartBitWidthInterface->reset( new AnalyzerSettingInterfaceInteger() );
	(*mStartBitWidthInterface)->SetTitleAndTooltip( "Start Bit Width (uS)",  "Specify the start bit width in uS" );
	(*mStartBitWidthInterface)->SetMax( 6000000 );
	(*mStartBitWidthInterface)->SetMin( 1 );
	(*mStartBitWidthInterface)->SetInteger( mStartBitWidth );

	mBitWidthInterface->reset( new AnalyzerSettingInterfaceInteger() );
	(*mBitWidthInterface)->SetTitleAndTooltip( "Bit Width (uS)",  "Specify the bit width in uS" );
	(*mBitWidthInterface)->SetMax( 6000000 );
	(*mBitWidthInterface)->SetMin( 1 );
	(*mBitWidthInterface)->SetInteger( mBitWidth );

	AddInterface( mInputChannelInterface->get() );
	AddInterface( mStartBitWidthInterface->get() );
	AddInterface( mBitWidthInterface->get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "IEbus", false );
}

IEBusAnalyzerSettings::~IEBusAnalyzerSettings()
{
}

bool IEBusAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = (*mInputChannelInterface)->GetChannel();
	mStartBitWidth = (*mStartBitWidthInterface)->GetInteger();
	mBitWidth = (*mBitWidthInterface)->GetInteger();

	ClearChannels();
	AddChannel( mInputChannel, "IEbus", true );

	return true;
}

void IEBusAnalyzerSettings::UpdateInterfacesFromSettings()
{
	(*mInputChannelInterface)->SetChannel( mInputChannel );
	(*mStartBitWidthInterface)->SetInteger( mStartBitWidth );
	mBitWidth = (*mBitWidthInterface)->GetInteger();
}

void IEBusAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mStartBitWidth;
	text_archive >> mBitWidth;

	ClearChannels();
	AddChannel( mInputChannel, "IEbus", true );

	UpdateInterfacesFromSettings();
}

const char* IEBusAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mStartBitWidth;
	text_archive >> mBitWidth;

	return SetReturnString( text_archive.GetString() );
}