#ifndef IEBUS_ANALYZER_H
#define IEBUS_ANALYZER_H

#include <Analyzer.h>
#include "IEBusAnalyzerResults.h"
#include "IEBusSimulationDataGenerator.h"

#define PARITY_ERROR 	1 << 0
#define NAK				1 << 1

class ANALYZER_EXPORT IEBusAnalyzer : public Analyzer
{
public:
	IEBusAnalyzer();
	virtual ~IEBusAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();
protected: //vars
	IEBusAnalyzerSettings * mSettings;
	IEBusAnalyzerResults * mResults;
	AnalyzerChannelData* mSerial;

	IEBusSimulationDataGenerator * mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
	// tolerance for instability of readings
	U32 tolerance_start;
	U32 tolerance_bit;
	U32 zero_bit_len;
	// measure width for each bit.
	U64 measure_width;
	// to hold the start of the start bit
	U64 start_sample_number_start;
	// to hold the finish of the start bit
	U64 start_sample_number_finish;
	// to hold the start of the data bit
	U64 start_bit_number_start;
	// data output
	U64 data;
	// flags
	// bit 0 = parity error
	// bit 1 = NAK
	U8 flags;

	// let's make this so we don't have DRY
	void update(S64 starting_sample, U64 &data1, U8 type, U8 flags);
	// simple parity check :: returns 1 if even number of bits, returns 0 if odd number of bits
	U8 parity(U16 data);
	bool getAddress(bool master);
	int getData(U8 dataType);
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //IEBUS_ANALYZER_H
