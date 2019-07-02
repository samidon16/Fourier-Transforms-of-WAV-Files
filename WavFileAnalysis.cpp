/*
Up coming changes:
-Add a GUI.
-Expand WavFile so it is compatible with WAV files other than pulse code modulated.
-Make sure there are no memory leaks
-Expand error handleing
-Output Function (and GUI) for WavFourierTransform

*/

// WavFileAnalysis.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#pragma comment(lib, "winmm.lib")

int main()
{
	string fileName = "sax.wav";
	HWAVEOUT hWaveOut;
	WAVEFORMATEX wfx;
	WavFile* wavFile = new WavFile(fileName, hWaveOut, wfx);
	wavFile->PlayAudio();
	Sleep(4000);
	wavFile->FourierTransform();
	wavFile->PrintFrequencies();
}