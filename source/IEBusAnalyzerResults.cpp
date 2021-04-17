#include "IEBusAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "IEBusAnalyzer.h"
#include "IEBusAnalyzerSettings.h"
#include <iostream>
#include <fstream>



#define CONTROL 100
#define LENGTH  101
#define DATA    102
#define HEADER  103

// this is the control bit functions
// this likely needs to move to results
#define READSLAVESTATUS 		0x0
#define READANDLOCK 			0x3
#define READLOCKLOWER    		0x4
#define READLOCKUPPER 			0x5
#define READANDUNLOCK 			0x6
#define READDATA 				0x7
#define WRITEANDLOCKCOMMAND 	0xA
#define WRITEANDLOCKDATA 		0xB
//#define WRITEANDLOCKCOMMAND 	0xE
#define WRITEDATA 				0xF

IEBusAnalyzerResults::IEBusAnalyzerResults( IEBusAnalyzer* analyzer, IEBusAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

IEBusAnalyzerResults::~IEBusAnalyzerResults()
{
}

void IEBusAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	if( frame.mFlags ){
		if( frame.mFlags & NAK )
			AddResultString( "NAK" );
	}
	else
		AddResultString( number_str );
}

void IEBusAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		if( frame.mFlags ){
			if( frame.mFlags & NAK )
				file_stream << time_str << "," << "NAK" << std::endl;
		}
		else if( frame.mData1 == 0xFFFF ){
			file_stream << std::endl << "=======================================" << std::endl;
			file_stream << time_str << "," << " START" << std::endl;
		}
		else{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
			if( frame.mData2 == CONTROL )
				file_stream << time_str << ", Control: " << number_str << std::endl;
			else if( frame.mData2 == LENGTH ){
				file_stream << time_str << ", Frame Length: " << number_str << std::endl;
				file_stream << time_str << ", DATA: ";
			}
			else if( frame.mData2 == DATA )
				file_stream << "," << number_str;
			else if( frame.mData2 == HEADER )
				file_stream << time_str << ", HEADER: " << number_str << std::endl;
			else if( frame.mData2 == 0 )
				file_stream << time_str << ", SLAVE ADDRESS: " << number_str << std::endl;
			else
				file_stream << time_str << ", MASTER ADDRESS" << number_str << std::endl;
		}

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}
	
	file_stream << std::endl << std::endl << "=======================================" << std::endl;
	file_stream << " RAW DATA" << std::endl;

	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, Hexadecimal, 8, number_str, 128 );
		file_stream << number_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void IEBusAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddResultString( number_str );
}

void IEBusAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void IEBusAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}