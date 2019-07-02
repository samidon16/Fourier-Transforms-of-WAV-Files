#include "stdafx.h"
using namespace std;

WavFile::WavFile(string fileNameInit, HWAVEOUT& hWaveOutInit, WAVEFORMATEX& wfxInit)
{
	hWaveOut = hWaveOutInit;
	wfx = wfxInit;
	fileName = fileNameInit;
	try
	{
		wavFile.open(fileName, ios::binary);
	}
	catch (ios_base::failure f)
	{
		cout << f.what();
	}
	AllocateBlocks();
	LoadWavFile();
}

WavFile::~WavFile()
{
	delete[NUMB_BLOCKS] blocks;
	wavFile.close();
}

//allocates and initializes heap memory for WAVEHDRs and their data
void WavFile::AllocateBlocks()
{
	unsigned char* buffer;
	int i;
	blocks;
	DWORD totalBufferSize = (BLOCK_LENGTH + sizeof(WAVEHDR)) * NUMB_BLOCKS;
	//allocate memory for the blocks in one large block
	if ((buffer = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, totalBufferSize)) == NULL)
	{
		fprintf(stderr, "Memory allocation error\n");
		ExitProcess(1);
	}
	//formats the large block of data in its sub-blocks
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * NUMB_BLOCKS;
	for (i = 0; i < NUMB_BLOCKS; i++)
	{
		blocks[i].dwBufferLength = BLOCK_LENGTH;
		blocks[i].lpData = (LPSTR)buffer;
		buffer += BLOCK_LENGTH;
	}
}

void WavFile::LoadWavFile()
{
	//The next few lines are for calculating the lengthSamples and lengthTime;
	unsigned long length;
	wavFile.seekg(4);
	wavFile.read((char*)&length, 4);
	length -= 32;
	lengthSamples = length / BYTES_PER_SAMPLE;
	lengthTime = lengthSamples / SAMPLES_PER_SECOND;
	//Pulls the information for initializing waveOut from the format chunk of wavFile
	wavFile.seekg(22);
	wavFile.read((char*)&numbChannels, 2);
	wavFile.read((char*)&samplesPerSec, 4);
	wavFile.seekg(34);
	wavFile.read((char*)&bitsPerSample, 2);
	//checks to make sure file is pulse code modulated (PCM)
	wavFile.seekg(16);
	short pcm;
	wavFile.read((char*)&pcm, 2);
	if (pcm != 16)
	{
		cout << "File is not PCM";
		exit(1);
	}
	//Sets the stream pointer for wavFile to the beginging of the data chunk
	wavFile.seekg(44);
}

//This method fills a passed in WAVEHDR block with audio data from a passed in file
void WavFile::FillBuffer(WAVEHDR* bufferBlock)
{
	if (wavFile.eof())
	{
		throw 10;
	}
	long startPos = (long)wavFile.tellg();
	wavFile.read((char*)&(bufferBlock->lpData[0]), BLOCK_LENGTH);
	if (wavFile.eof())
	{
		long numbBytesRead = (lengthSamples * BYTES_PER_SAMPLE) - startPos;
		for (unsigned long i = numbBytesRead; i < BLOCK_LENGTH; i++)
		{
			bufferBlock->lpData[i] = 0;
		}
	}
}