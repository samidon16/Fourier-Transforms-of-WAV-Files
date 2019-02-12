#include <iostream>
#include <Windows.h>
#include <Audioclient.h>
#include <mmsystem.h>
#include <windows.h>
#include <cmath>
#include <fstream>

using namespace std;
#pragma comment(lib, "winmm.lib")
/*
* some good values for block size and count
*/
#define NUMB_BLOCKS 2
#define SAMPLES_PER_SECOND 44100.0
#define SECONDS_PER_SAMPLE 1 / SAMPLES_PER_SECOND
#define BITS_PER_SAMPLE 16
#define PI 3.14159265359
#define BIT_DEPTH pow(2, 16)
#define BUFFER_LENGTH 50000
/*
* function prototypes
*/
static void CALLBACK waveOutCallback(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
static WAVEHDR* allocateBlocks(int size);
static void freeBlocks(WAVEHDR* blockArray);
static void writeAudioBlock(HWAVEOUT hWaveOut, WAVEHDR* block);
static void fillBuffer(WAVEHDR* block, ifstream& wavFile);
/*
* module level variables
*/
static CRITICAL_SECTION waveCriticalSection;
static bool eofFlag = 0;

struct instanceData{
	WAVEHDR* blocks;
	ifstream* wavFile;
	instanceData(WAVEHDR* blocksInit, ifstream* wavFileInit) {
		blocks = blocksInit;
		wavFile = wavFileInit;
	}
};
//callback function for when a buffer finishes playing
static void CALLBACK waveOutCallback(HWAVEOUT hWaveOut, UINT uMsg, DWORD instance, DWORD dwParam1, DWORD dwParam2){
	/*
	* ignore calls that occur due to openining and closing the
	* device.
	*/
	instanceData* data = (instanceData*)instance;
	if (uMsg != WOM_DONE)return;
	EnterCriticalSection(&waveCriticalSection);
	if (waveOutUnprepareHeader(hWaveOut, &(data->blocks[0]), sizeof(WAVEHDR)) != WAVERR_STILLPLAYING) {
		fillBuffer(&(data->blocks[0]), *(data->wavFile));
		if (waveOutPrepareHeader(hWaveOut, &(data->blocks[0]), sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			cout << "waveOutPrepareHeader failed" << '\n';
		}
	}
	else if (waveOutUnprepareHeader(hWaveOut, &(data->blocks[1]), sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
		fillBuffer(&(data->blocks[1]), *(data->wavFile));
		if (waveOutPrepareHeader(hWaveOut, &(data->blocks[1]), sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			cout << "waveOutPrepareHeader failed" << '\n';
		}
	}
	LeaveCriticalSection(&waveCriticalSection);
}
//allocates and initializes heap memory for WAVEHDRs and their data
WAVEHDR* allocateBlocks(int size)
{
	unsigned char* buffer;
	int i;
	WAVEHDR* blocks;
	DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * NUMB_BLOCKS;
	/*
	* allocate memory for the entire set in one go
	*/
	if ((buffer = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, totalBufferSize)) == NULL) {
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}
	/*
	* and set up the pointers to each bit
	*/
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * NUMB_BLOCKS;
	for (i = 0; i < NUMB_BLOCKS; i++) {
		blocks[i].dwBufferLength = size;
		blocks[i].lpData = (LPSTR)buffer;
		buffer += size;
	}
	return blocks;
}
void freeBlocks(WAVEHDR* blockArray)
{
	/*
	* and this is why allocateBlocks works the way it does
	*/
	HeapFree(GetProcessHeap(), 0, blockArray);
}
void writeAudioBlock(HWAVEOUT hWaveOut, WAVEHDR* block) {
	if (waveOutPrepareHeader(hWaveOut, block, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		cout << "waveOutPrepareHeader failed" << '\n';
	}
	/*
	* write the block to the device. waveOutWrite returns immediately
	* unless a synchronous driver is used
	*/
	if (waveOutWrite(hWaveOut, block, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		cout << "waveOutWrite failed" << '\n';
	}
}
//Uses PCM to encode a list of sine functions starting at a specified time
//into a block of audio data and returns the end time. The size is measure in 16 bits.
void writeFourierSuper(WAVEHDR* block, unsigned int size, double& timePos, long* frequencies, unsigned int numbFreq) {
	short* samples = (short*)block->lpData;
	unsigned int memUsed = 0;
	int i = 0;
	while (memUsed < size) {
		samples[memUsed] = 0;
		while (frequencies[i] != 0) {
			samples[memUsed] += (short)((BIT_DEPTH / (numbFreq * 2)) * (sin((frequencies[i] * 2 * PI) * timePos)));
			i++;
		}
		i = 0;
		timePos += SECONDS_PER_SAMPLE;
		memUsed++;
	}
}
void initWaveOut(HWAVEOUT& hWaveOut, WAVEFORMATEX& wfx) {
	wfx.nSamplesPerSec = (DWORD)SAMPLES_PER_SECOND;
	wfx.wBitsPerSample = BITS_PER_SAMPLE;
	wfx.nChannels = 1;
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, NULL, 0, CALLBACK_NULL | WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE) != MMSYSERR_NOERROR) {
		cout << stderr, "unable to open WAVE_MAPPER device\n";
		ExitProcess(1);
	}
}
void initWaveOut(HWAVEOUT& hWaveOut, WAVEFORMATEX& wfx, DWORD samplesPerSec, WORD bitsPerSample, WORD numbChannels, instanceData* instance) {
	wfx.nSamplesPerSec = samplesPerSec;
	wfx.wBitsPerSample = bitsPerSample;
	wfx.nChannels = numbChannels;
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutCallback, (DWORD_PTR) instance, CALLBACK_NULL | WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE) != MMSYSERR_NOERROR) {
		cout << stderr, "unable to open WAVE_MAPPER device\n";
		ExitProcess(1);
	}
}
void playFourierSuper(HWAVEOUT hWaveOut, long* frequencies, unsigned int numbFreq) {
	WAVEHDR* blocks = allocateBlocks((int)SAMPLES_PER_SECOND*2);
	double timePos = 0;
	writeFourierSuper(&blocks[0], (unsigned int)SAMPLES_PER_SECOND, timePos, frequencies, numbFreq);
	writeFourierSuper(&blocks[1], (unsigned int)SAMPLES_PER_SECOND, timePos, frequencies, numbFreq);
	writeAudioBlock(hWaveOut, &blocks[0]);
	writeAudioBlock(hWaveOut, &blocks[1]);
	Sleep(2000);
}
void loadWavFile(ifstream& wavFile, HWAVEOUT& hWaveOut, WAVEFORMATEX& wfx, instanceData* instance) {
	if (!wavFile)cout << "File failed to open.";
	//Pulls the information for initializing waveOut from the format chunk of wavFile
	DWORD samplesPerSec;
	WORD bitsPerSample;
	WORD numbChannels;
	wavFile.seekg(22);
	wavFile >> numbChannels;
	wavFile >> samplesPerSec;
	wavFile.seekg(34);
	wavFile >> bitsPerSample;
	initWaveOut(hWaveOut, wfx, samplesPerSec, bitsPerSample, numbChannels, instance);
	//Sets the stream pointer for wavFile to the beginging of the data chunk
	wavFile.seekg(44);
}
void fillBuffer(WAVEHDR* block, ifstream& wavFile) {
	if (eofFlag) {
		return;
	}
	for (unsigned int i = 0; i < BUFFER_LENGTH; i++) {
		if (!wavFile.eof())wavFile >> block->lpData[i];
		else {
			eofFlag = true;
			break;
		}
	}
}
void playWavFile(string fileName, HWAVEOUT& hWaveOut, WAVEFORMATEX& wfx) {
	//Initializes the wav file and waveOut
	ifstream wavFile;
	wavFile.open(fileName);
	WAVEHDR* blocks = allocateBlocks(BUFFER_LENGTH);
	instanceData* instance = new instanceData(blocks, &wavFile);
	loadWavFile(wavFile, hWaveOut, wfx, instance);
	//Initializes and plays both buffer blocks
	fillBuffer(&blocks[0], wavFile);
	writeAudioBlock(hWaveOut, &blocks[0]);
	if (!eofFlag) {
		fillBuffer(&blocks[1], wavFile);
		writeAudioBlock(hWaveOut, &blocks[1]);
	}
}
int main() {
	//declare and initialize waveOut variables
	HWAVEOUT hWaveOut;
	WAVEFORMATEX wfx;
	initWaveOut(hWaveOut, wfx);
	//
	long frequencies[2] = { 400, 0 };
	playFourierSuper(hWaveOut, &frequencies[0], 1);
}