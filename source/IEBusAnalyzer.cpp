#include "IEBusAnalyzer.h"
#include "IEBusAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

#define CONTROL 100
#define LENGTH  101
#define DATA    102
#define HEADER  103

__declspec(dllexport) IEBusAnalyzer::IEBusAnalyzer()
:	Analyzer(),  
	mSettings( new IEBusAnalyzerSettings() ),	
	mSimulationInitilized( false )
{
	mResults = new IEBusAnalyzerResults(this, mSettings);
	mSimulationDataGenerator = new IEBusSimulationDataGenerator();
	SetAnalyzerSettings( mSettings );
	tolerance_start = mSettings->mStartBitWidth / 10;
	tolerance_bit = mSettings->mBitWidth / 10;
	zero_bit_len = static_cast<U32>((static_cast<double>(mSettings->mBitWidth) * 0.875));
	data = 0;
	flags = 0;
	//mEndOfStopBitOffset = 0;
	//mSampleRateHz = 0;
	//mSerial = 0;
}

__declspec(dllexport) IEBusAnalyzer::~IEBusAnalyzer()
{
	//KillThread();
}

void IEBusAnalyzer::update(S64 starting_sample, U64 &data1, U8 type, U8 flags)
{
	Frame f;
	f.mData1 = U32(data1);
	f.mData2 = type;
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

U8 IEBusAnalyzer::parity(U16 data)
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

bool IEBusAnalyzer::getAddress(bool master)
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
		measure_width = (start_sample_number_finish - start_bit_number_start);
		if( (measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit )) && (measure_width < ( (mSettings->mBitWidth / 2) + tolerance_bit ) ))
		{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
			data |= mask;
		}
		else if ((measure_width > (zero_bit_len - tolerance_bit)) && (measure_width < (zero_bit_len + tolerance_bit)))
		{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
		}
		else
		{
			return false;
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
			measure_width = (start_sample_number_finish - start_bit_number_start);
			if (measure_width > (mSettings->mBitWidth / 2 - tolerance_bit) && measure_width < (mSettings->mBitWidth / 2 + tolerance_bit)) {
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel);
				flags = 1;
			}
			else if ((measure_width > (zero_bit_len - tolerance_bit)) && (measure_width < (zero_bit_len + tolerance_bit))) {
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel);
				flags = 0;
			}
			else
				return false;
			flags = flags & parity(static_cast<U16>( data ));
		}
	}
	// if slave address we should expect a slave ack
	if(!master){
		mSerial->AdvanceToNextEdge();
		start_bit_number_start = mSerial->GetSampleNumber();
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
		mSerial->AdvanceToNextEdge();
		start_sample_number_finish = mSerial->GetSampleNumber();
		measure_width = (start_sample_number_finish - start_bit_number_start);
		if (measure_width > (mSettings->mBitWidth / 2 - tolerance_bit) && measure_width < (mSettings->mBitWidth / 2 + tolerance_bit)) {
			mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel);
			flags = NAK;
		}
		else if ((measure_width > (zero_bit_len - tolerance_bit)) && (measure_width < (zero_bit_len + tolerance_bit))) {
			mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel);
		}
		else
			return false;
	}
	update(start_sample_number_start, data, master, 0/*flags*/); //return flags for NACK
	return (flags != NAK);
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
		measure_width = (start_sample_number_finish - start_bit_number_start);
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
			measure_width = (start_sample_number_finish - start_bit_number_start);
			if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
				//flags = 1;
			}
			else{
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
				//flags = 0;
			}
			//flags = flags & parity(data);
		}
	}
	// we should always expect a ACK
	mSerial->AdvanceToNextEdge();
	start_bit_number_start = mSerial->GetSampleNumber();
	mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
	mSerial->AdvanceToNextEdge();
	start_sample_number_finish = mSerial->GetSampleNumber();
	measure_width = (start_sample_number_finish - start_bit_number_start);
	if( measure_width > ( mSettings->mBitWidth / 2 - tolerance_bit ) && measure_width < ( mSettings->mBitWidth / 2 + tolerance_bit ) ){
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
		flags = flags | 2;
	}
	else{
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
	}
	// data gets reset in update, so save a copy for returning
	U8 tempData = static_cast<U8>( data );
	update(start_sample_number_start, data, dataType, flags);
	return tempData;
}

void IEBusAnalyzer::WorkerThread()
{
	mResults = new IEBusAnalyzerResults( this, mSettings );
	SetAnalyzerResults( mResults );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );

	mSampleRateHz = GetSampleRate();

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );
	
	U8 data_mask = 1 << 7;

	for( ; ; )
	{
		if( mSerial->GetBitState() == BIT_LOW )
			mSerial->AdvanceToNextEdge();
		data = 0;
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
			measure_width = (start_sample_number_finish - start_sample_number_start);

			// check to see if we have a starting bit
			// if so let us continue to collect data
			// if not we will reset the starting position
			if( (measure_width > (mSettings->mStartBitWidth - tolerance_start ))
				&&  (measure_width < (mSettings->mStartBitWidth + tolerance_start )) )
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
		update(start_sample_number_start, data, 0, 0);

		// get the header
		start_sample_number_start = mSerial->GetSampleNumber();
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
		mSerial->AdvanceToNextEdge();
		start_sample_number_finish = mSerial->GetSampleNumber();
		measure_width = (start_sample_number_finish - start_sample_number_start);
		if( (measure_width > ( (mSettings->mBitWidth / 2) - tolerance_bit )) && (measure_width < ( (mSettings->mBitWidth / 2) + tolerance_bit ) )){
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
			data = 1;
		}
		else{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel );
			data = 0;
		}
		update(start_sample_number_start, data, HEADER, 0);

		// get the master address
		if (getAddress(true) == false) continue;
		// get the slave address
		if (getAddress(false) == false) continue;
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
		mSimulationDataGenerator->Initialize( GetSimulationSampleRate(), mSettings );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator->GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
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
