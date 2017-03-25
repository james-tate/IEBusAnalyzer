#include "IEBusAnalyzer.h"
#include "IEBusAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

#define CONTROL 100
#define LENGTH  101
#define DATA    102

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
#define WRITEANDLOCKCOMMAND 	0xE
#define WRITEDATA 				0xF

IEBusAnalyzer::IEBusAnalyzer()
:	Analyzer(),  
	mSettings( new IEBusAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
	tolerance_start = mSettings->mStartBitWidth * .05;
	tolerance_bit = mSettings->mBitWidth * .05;
}

IEBusAnalyzer::~IEBusAnalyzer()
{
	KillThread();
}

void IEBusAnalyzer::update(S64 starting_sample, U64 &data1, U8 flags)
{
	Frame f;
	f.mData1 = U32(data1);
	f.mFlags = flags;
	f.mStartingSampleInclusive = starting_sample; 
	f.mEndingSampleInclusive = mSerial->GetSampleNumber();

	mResults->AddFrame( f );
	mResults->CommitResults();

	ReportProgress( f.mEndingSampleInclusive );

	// move to next rising edge
	mSerial->AdvanceToNextEdge();
	data1 = 0;
}

bool IEBusAnalyzer::parity(U16 data)
{
    int parity = 0;
    for(int x = 0; x < 8; x++){
    	if(data & 1 << x)
    		parity++;
    }
    if(parity % 2)
    	return 1;
    else
    	return 0;
}

void IEBusAnalyzer::getAddress(bool master)
{
	U16 mask = 1 << 11;
	data = 0;
	flags = 0;
	start_sample_number_start = mSerial->GetSampleNumber();
	start_bit_number_start = start_sample_number_start;

	int i = 0;
	while(i < 12){
		mResults->AddMarker( start_bit_number_start, AnalyzerResults::Dot, mSettings->mInputChannel );
		mSerial->AdvanceToNextEdge();
		start_sample_number_finish = mSerial->GetSampleNumber();
		measure_width = (start_sample_number_finish - start_bit_number_start) * 2 * .001;
		if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
			data |= mask;
		}
		else{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
		}
		i++;
		if(i < 12){
			mSerial->AdvanceToNextEdge();
			start_bit_number_start = mSerial->GetSampleNumber();
			mask = mask >> 1;
		}
		// get the parity
		else{
			mSerial->AdvanceToNextEdge();
			start_bit_number_start = mSerial->GetSampleNumber();
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
			mSerial->AdvanceToNextEdge();
			start_sample_number_finish = mSerial->GetSampleNumber();
			measure_width = (start_sample_number_finish - start_bit_number_start) * 2 * .001;
			if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
				flags = 1;
			}
			else{
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
				flags = 0;
			}
			flags = flags & parity(data);
		}
	}
	// if slave address we should expect a slave ack
	if(!master){
		mSerial->AdvanceToNextEdge();
		start_bit_number_start = mSerial->GetSampleNumber();
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
		mSerial->AdvanceToNextEdge();
		start_sample_number_finish = mSerial->GetSampleNumber();
		measure_width = (start_sample_number_finish - start_bit_number_start) * 2 * .001;
		if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
			flags = flags | 2;
		}
		else{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
		}
	}
	update(start_sample_number_start, data , flags);
}

int IEBusAnalyzer::getData(U8 dataType)
{
	U8 numBits = 8;
	U16 mask = 1 << 7;
	if(dataType == CONTROL){
		numBits = 4;
		mask = 1 << 3;
	}
	data = 0;
	flags = 0;
	start_sample_number_start = mSerial->GetSampleNumber();
	start_bit_number_start = start_sample_number_start;

	int i = 0;
	while(i < numBits){
		mResults->AddMarker( start_bit_number_start, AnalyzerResults::Dot, mSettings->mInputChannel );
		mSerial->AdvanceToNextEdge();
		start_sample_number_finish = mSerial->GetSampleNumber();
		measure_width = (start_sample_number_finish - start_bit_number_start) * 2 * .001;
		if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
			data |= mask;
		}
		else{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
		}
		i++;
		if(i < numBits){
			mSerial->AdvanceToNextEdge();
			start_bit_number_start = mSerial->GetSampleNumber();
			mask = mask >> 1;
		}
		// get the parity
		else{
			mSerial->AdvanceToNextEdge();
			start_bit_number_start = mSerial->GetSampleNumber();
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
			mSerial->AdvanceToNextEdge();
			start_sample_number_finish = mSerial->GetSampleNumber();
			measure_width = (start_sample_number_finish - start_bit_number_start) * 2 * .001;
			if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
				flags = 1;
			}
			else{
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
				flags = 0;
			}
			flags = flags & parity(data);
		}
	}
	// we should always expect a ACK
	mSerial->AdvanceToNextEdge();
	start_bit_number_start = mSerial->GetSampleNumber();
	mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
	mSerial->AdvanceToNextEdge();
	start_sample_number_finish = mSerial->GetSampleNumber();
	measure_width = (start_sample_number_finish - start_bit_number_start) * 2 * .001;
	if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
		flags = flags | 2;
	}
	else{
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
	}
	// data gets reset in update, so save a copy for returning
	U8 tempData = data;
	update(start_sample_number_start, data, flags);
	return tempData;
}

void IEBusAnalyzer::WorkerThread()
{
	mResults.reset( new IEBusAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );

	mSampleRateHz = GetSampleRate();

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );
	
	U8 data_mask = 1 << 7;

	for( ; ; )
	{
		if( mSerial->GetBitState() == BIT_LOW )
			mSerial->AdvanceToNextEdge();
		data = 0;
		flags = 0;
		int length = 0 ;

		// flag for starting bit
		bool found_start = false;
		bool finished_field = false;
		int bit_counter = 0;
		start_sample_number_start = mSerial->GetSampleNumber();

		// search for the starting bit
		while(!found_start){
			// go to edge of what might be the starting bit and get the sample number
			mSerial->AdvanceToNextEdge();
			start_sample_number_finish = mSerial->GetSampleNumber();

			// measure the sample and normilize the units
			measure_width = (start_sample_number_finish - start_sample_number_start) * 2;
			if (measure_width > 100000)
				measure_width *= .01;
			else
				measure_width *= .001;

			// check to see if we have a starting bit
			// if so let us continue to collect data
			// if not we will reset the starting position
			if( measure_width > ( mSettings->mStartBitWidth - tolerance_start )
				&&  measure_width < ( mSettings->mStartBitWidth + tolerance_start ) ) 
			{
				data = 0xFFFF;
				found_start = true;
				mResults->AddMarker( start_sample_number_start, AnalyzerResults::UpArrow, mSettings->mInputChannel );
				mResults->AddMarker( start_sample_number_finish, AnalyzerResults::Start, mSettings->mInputChannel );
			}
			else
			{
				mSerial->AdvanceToNextEdge();
				start_sample_number_start = mSerial->GetSampleNumber();
			}
		}
		update(start_sample_number_start, data, 0);

		// get the header
		start_sample_number_start = mSerial->GetSampleNumber();
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
		mSerial->AdvanceToNextEdge();
		start_sample_number_finish = mSerial->GetSampleNumber();
		measure_width = (start_sample_number_finish - start_sample_number_start) * 2 * .001;
		if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
			data = 1;
		}
		else{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
			data = 0;
		}
		update(start_sample_number_start, data, 0);

		// get the master address
		getAddress(true);
		// get the slave address
		getAddress(false);
		// get control
		getData(CONTROL);
		// get length
		length = getData(LENGTH);
		while(length > 0){
			getData(DATA);
			length--;
		}
	}
}

bool IEBusAnalyzer::NeedsRerun()
{
	return false;
}

U32 IEBusAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 IEBusAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mStartBitWidth * 4;
}

const char* IEBusAnalyzer::GetAnalyzerName() const
{
	return "IEbus by github.com/james-tate";
}

const char* GetAnalyzerName()
{
	return "IEbus by github.com/james-tate";
}

Analyzer* CreateAnalyzer()
{
	return new IEBusAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
