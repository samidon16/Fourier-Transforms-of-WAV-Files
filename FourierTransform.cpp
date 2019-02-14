/*
Methods and data structure in this file use the Windows WaveOut API.

Next update will include:
-Ability to play frequency superposition for any arbitrary time period, not just 5 seconds.
-Functions for finding the fourier decomposition of arbitrary PCM audio data
-Functions for changing the amount of an arbitrary frequency in a WAV file using the fourier decomposition
-Functions for generating audio data for a superposition of WAV audio data
*/


#include <iostream>
#include <Windows.h>
#include <Audioclient.h>
#include <mmsystem.h>
#include <windows.h>
#include <cmath>
#include <fstream>

using namespace std;
#pragma comment(lib, "winmm.lib")

//Macros
#define NUMB_BLOCKS 2
#define SAMPLES_PER_SECOND 44100.0
#define SECONDS_PER_SAMPLE 1 / SAMPLES_PER_SECOND
#define BITS_PER_SAMPLE 16
#define PI 3.14159265359
#define BIT_DEPTH pow(2, 16)
#define BUFFER_LENGTH 10000

// function prototypes
struct instanceData;
static void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
static WAVEHDR* allocateBlocks(int);
static void freeBlocks(WAVEHDR*);
static void writeAudioBlock(HWAVEOUT, WAVEHDR*);
static void fillBuffer(WAVEHDR*, ifstream&);
static void writeFourierSuper(WAVEHDR*, unsigned int, double&, long*, unsigned int);
static void initWaveOut(HWAVEOUT&, WAVEFORMATEX&, DWORD, WORD, WORD, instanceData*);
static void initWaveOut(HWAVEOUT&, WAVEFORMATEX&);
static void playFourierSuper(HWAVEOUT, long*, unsigned int);
static void loadWavFile(ifstream&, HWAVEOUT&, WAVEFORMATEX&, instanceData*, unsigned int&);
static void fillBuffer(WAVEHDR*, ifstream&);
static void playWavFile(string, HWAVEOUT&, WAVEFORMATEX&);

//Global Variables
static CRITICAL_SECTION waveCriticalSection;

//This packages the instance data that is passed to the waveOutProc callback function by waveOut
struct instanceData{
	WAVEHDR* blocks;
	ifstream* wavFile;
	instanceData(WAVEHDR* blocksInit, ifstream* wavFileInit) {
		blocks = blocksInit;
		wavFile = wavFileInit;
	}
};
//callback function for when a buffer finishes playing
static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR instance, DWORD_PTR dwParam1, DWORD_PTR dwParam2){
	instanceData* data = (instanceData*)instance;
	if (uMsg != WOM_DONE)return;
	EnterCriticalSection(&waveCriticalSection);
	if (waveOutUnprepareHeader(hWaveOut, (WAVEHDR*)&(data->blocks[0]), sizeof(WAVEHDR)) != WAVERR_STILLPLAYING) {
		fillBuffer(&(data->blocks[0]), *(data->wavFile));
		writeAudioBlock(hWaveOut, (WAVEHDR*)&(data->blocks[0]));
	}
	else if (waveOutUnprepareHeader(hWaveOut, (WAVEHDR*)&(data->blocks[1]), sizeof(WAVEHDR)) != WAVERR_STILLPLAYING) {
		fillBuffer(&(data->blocks[1]), *(data->wavFile));
		writeAudioBlock(hWaveOut, (WAVEHDR*)&(data->blocks[1]));
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
	//allocate memory for the blocks in one large block
	if ((buffer = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, totalBufferSize)) == NULL) {
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}
	//formats the large block of data in its sub-blocks
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
//This method prepares the wave header passed in and write it the audio device.
void writeAudioBlock(HWAVEOUT hWaveOut, WAVEHDR* block) {
	if (waveOutPrepareHeader(hWaveOut, block, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		cout << "waveOutPrepareHeader failed" << '\n';
	}
	//write the block to the device. waveOutWrite returns immediately
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
//This method initializes the data associated with WAVEFORATEX using values selected to be the default
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
//This method initializes the data associated with WAVEFORATEX by using passed in values from a WAV file
void initWaveOut(HWAVEOUT& hWaveOut, WAVEFORMATEX& wfx, DWORD samplesPerSec, WORD bitsPerSample, WORD numbChannels, instanceData* instance) {
	InitializeCriticalSection(&waveCriticalSection);
	wfx.nSamplesPerSec = samplesPerSec;
	wfx.wBitsPerSample = bitsPerSample;
	wfx.nChannels = numbChannels;
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)instance, CALLBACK_FUNCTION | WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE) != MMSYSERR_NOERROR) {
		cout << stderr, "unable to open WAVE_MAPPER device\n";
		ExitProcess(1);
	}
}
//This method plays 5 seconds of audio data generated from a superposition of frequencies
void playFourierSuper(HWAVEOUT hWaveOut, long* frequencies, unsigned int numbFreq) {
	WAVEHDR* blocks = allocateBlocks((int)SAMPLES_PER_SECOND*2);
	double timePos = 0;
	writeFourierSuper(&blocks[0], (unsigned int)SAMPLES_PER_SECOND, timePos, frequencies, numbFreq);
	writeFourierSuper(&blocks[1], (unsigned int)SAMPLES_PER_SECOND, timePos, frequencies, numbFreq);
	writeAudioBlock(hWaveOut, &blocks[0]);
	writeAudioBlock(hWaveOut, &blocks[1]);
	Sleep(5000);
}
//This method pulls format data from a passed in WAV file and uses it to call initWaveOut
void loadWavFile(ifstream& wavFile, HWAVEOUT& hWaveOut, WAVEFORMATEX& wfx, instanceData* instance, unsigned int& timeSleep) {
	//Pulls the information for initializing waveOut from the format chunk of wavFile
	DWORD samplesPerSec;
	WORD bitsPerSample;
	WORD numbChannels;
	wavFile.seekg(22);
	wavFile.read((char*)&numbChannels, 2);
	wavFile.read((char*)&samplesPerSec, 4);
	wavFile.seekg(34);
	wavFile.read((char*)&bitsPerSample, 2);
	//find the amount of time it will take to play the WAV file
	wavFile.seekg(4);
	wavFile.read((char*)&timeSleep, 4);
	timeSleep -= 44;
	timeSleep /= (bitsPerSample / 8);
	timeSleep /= (samplesPerSec / 1000);
	//checks to make sure file is pulse code modulated (PCM)
	wavFile.seekg(16);
	short pcm;
	wavFile.read((char*)&pcm, 2);
	if (pcm != 16) {
		cout << "File is not PCM";
		return;
	}
	initWaveOut(hWaveOut, wfx, samplesPerSec, bitsPerSample, numbChannels, instance);
	//Sets the stream pointer for wavFile to the beginging of the data chunk
	wavFile.seekg(44);
}
//This method fills a passed in WAVEHDR block with audio data from a passed in file
void fillBuffer(WAVEHDR* block, ifstream& wavFile) {
	if (wavFile.eof()) {
		return;
	}
	wavFile.read((char*)&(block->lpData[0]), BUFFER_LENGTH);
}
//This method plays a passed in WAV file
void playWavFile(string fileName, HWAVEOUT& hWaveOut, WAVEFORMATEX& wfx) {
	//Initializes the wav file and waveOut
	ifstream wavFile;
	wavFile.open(fileName, ifstream::binary);
	if (!wavFile)cout << "File failed to open.";
	WAVEHDR* blocks = allocateBlocks(BUFFER_LENGTH);
	instanceData* instance = new instanceData(blocks, &wavFile);
	unsigned int timeSleep;
	loadWavFile(wavFile, hWaveOut, wfx, instance, timeSleep);
	//Initializes and plays both buffer blocks
	fillBuffer(&blocks[0], wavFile);
	writeAudioBlock(hWaveOut, &blocks[0]);
	if (!wavFile.eof()) {
		EnterCriticalSection(&waveCriticalSection);
		fillBuffer(&blocks[1], wavFile);
		writeAudioBlock(hWaveOut, &blocks[1]);
		LeaveCriticalSection(&waveCriticalSection);
	}
	Sleep(timeSleep);
}
int main() {
	//declare and initialize waveOut variables
	HWAVEOUT hWaveOut;
	WAVEFORMATEX wfx;
	//Choice path for user
	char choice;
	cout << "Play WAV file or generate sound from frequency superposition?" << '\n' << "Enter w for WAV file or s for frequency superposition: ";
	cin >> choice;
	if ((choice == 'w') | (choice == 'W')) {
		int file;
		cout << "Choose WAV file to play:" << '\n' << "Enter: 1 for fanfar10.wav, 2 for sax.wav, 3 for saxaphone.wav" << '\n';
		cin >> file;
		switch (file) {
		case 1:
			playWavFile("fanfare10.wav", hWaveOut, wfx);
			break;
		case 2:
			playWavFile("sax.wav", hWaveOut, wfx);
			break;
		case 3:
			playWavFile("saxaphone.wav", hWaveOut, wfx);
			break;
		default:
			cout << "Your input did not match any of the options" << '\n';
		}
	}
	else if ((choice == 's') | (choice == 'S')) {
		initWaveOut(hWaveOut, wfx);
		long frequencies[2] = { 400, 0 };
		playFourierSuper(hWaveOut, &frequencies[0], 1);
	}
	else {
		cout << "Your input did not match either choice." << '\n';
	}
}
