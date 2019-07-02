#include "stdafx.h"
using namespace std;

//Global Variables
static CRITICAL_SECTION waveCriticalSection;

void WavFile::InitWaveOut()
{
	InitializeCriticalSection(&waveCriticalSection);
	wfx.nSamplesPerSec = samplesPerSec;
	wfx.wBitsPerSample = bitsPerSample;
	wfx.nChannels = numbChannels;
	wfx.cbSize = 0; //size of _extra_ info
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION | WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE) != MMSYSERR_NOERROR) {
		cout << stderr, "unable to open WAVE_MAPPER device\n";
		ExitProcess(1);
	}
}

//callback function for when a buffer finishes playing
void CALLBACK WavFile::WaveOutProc(HWAVEOUT hWaveOut2, UINT uMsg, DWORD_PTR wavFileInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	WAVEHDR* bufferBlocks = ((WavFile*)wavFileInstance)->blocks;
	if (uMsg != WOM_DONE)return;
	EnterCriticalSection(&waveCriticalSection);
	try
	{
		if (waveOutUnprepareHeader(hWaveOut2, &(bufferBlocks[0]), sizeof(WAVEHDR)) != WAVERR_STILLPLAYING) {
			((WavFile*)wavFileInstance)->FillBuffer(&(bufferBlocks[0]));
			((WavFile*)wavFileInstance)->WriteAudioBlock(&bufferBlocks[0]);
		}
		else if (waveOutUnprepareHeader(hWaveOut2, &bufferBlocks[1], sizeof(WAVEHDR)) != WAVERR_STILLPLAYING) {
			((WavFile*)wavFileInstance)->FillBuffer(&bufferBlocks[1]);
			((WavFile*)wavFileInstance)->WriteAudioBlock(&bufferBlocks[1]);
		}
	}
	catch (int){}
	LeaveCriticalSection(&waveCriticalSection);
}

//This method prepares the wave header passed in and writes it to the audio device.
void WavFile::WriteAudioBlock(WAVEHDR* block)
{
	if (waveOutPrepareHeader(hWaveOut, block, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		cout << "waveOutPrepareHeader failed" << '\n';
	}
	//write the block to the device. waveOutWrite returns immediately
	if (waveOutWrite(hWaveOut, block, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		cout << "waveOutWrite failed" << '\n';
	}
}

void WavFile::PlayAudio()
{
	InitWaveOut();
	FillBuffer(&blocks[0]);
	WriteAudioBlock(&blocks[0]);
	if (!wavFile.eof()) {
		EnterCriticalSection(&waveCriticalSection);
		FillBuffer(&blocks[1]);
		WriteAudioBlock(&blocks[1]);
		LeaveCriticalSection(&waveCriticalSection);
	}
}