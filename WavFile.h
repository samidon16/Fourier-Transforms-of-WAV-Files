//Class specific include files
#include "stdafx.h"
#include <vector>
using namespace std;

#pragma comment(lib, "winmm.lib")
#pragma once

class WavFile
{
private:
	//WavFileGeneral
	ifstream wavFile;
	WAVEHDR* blocks;
	unsigned long lengthSamples;
	double lengthTime;
	void AllocateBlocks();
	void LoadWavFile();
	void FillBuffer(WAVEHDR* block);

	//WavWaveOut
	HWAVEOUT hWaveOut;
	WAVEFORMATEX wfx;
	void InitWaveOut();
	static void CALLBACK WaveOutProc(HWAVEOUT hWaveOut2, UINT uMsg, DWORD_PTR blocks, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	void WriteAudioBlock(WAVEHDR* block);

	//WavFourierTransform
	vector<double> frequencies;
public:
	//WavFileGeneral
	string fileName;
	DWORD samplesPerSec;
	WORD bitsPerSample;
	WORD numbChannels;
	WavFile(string fileNameInit, HWAVEOUT& hWaveOutInit, WAVEFORMATEX& wfxInit);
	~WavFile();

	//WavWaveOut
	void PlayAudio();

	//WavFourierTransform
	void FourierTransform();
	void PrintFrequencies();
};