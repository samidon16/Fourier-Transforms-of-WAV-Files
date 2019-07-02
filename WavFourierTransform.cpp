#include "stdafx.h"
#include "math.h"
using namespace std;

void WavFile::FourierTransform() {
	unsigned long bufferPos;
	short fx;
	for (unsigned long freqNumb = 0; freqNumb < NUMB_FREQUENCIES; freqNumb++)
	{
		wavFile.seekg(44);
		FillBuffer(&blocks[0]);
		bufferPos = 0;
		frequencies.push_back(0);
		for (unsigned long sample = 0; sample < lengthSamples; sample++)
		{
			fx = (short)blocks[0].lpData[bufferPos * BYTES_PER_SAMPLE];
			frequencies[freqNumb] += fx * SECONDS_PER_SAMPLE * pow(E, (-2)*(PI)*(sample)*(SECONDS_PER_SAMPLE)/(freqNumb * V_RESOLUTION + V_RESOLUTION));
			if (bufferPos < BLOCK_LENGTH - 1)
			{
				bufferPos++;
			}
			else
			{
				FillBuffer(&blocks[0]);
				bufferPos = 0;
			}
		}
	}
}

void WavFile::PrintFrequencies() {
	for (unsigned long i = 0; i < frequencies.size(); i++)
	{
		cout << frequencies[i] << '\t';
	}
}