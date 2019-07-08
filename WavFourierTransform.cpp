#include "stdafx.h"
#include "math.h"
using namespace std;

void WavFile::FourierTransform() {
	unsigned long bufferPos;
	short fx;
	double t;
	double w;
	for (unsigned long freqNumb = 0; freqNumb < NUMB_FREQUENCIES; freqNumb++)
	{
		wavFile.seekg(44);
		FillBuffer(&blocks[0]);
		bufferPos = 0;
		frequencies.push_back(0);
		for (unsigned long sample = 0; sample < lengthSamples; sample++)
		{
			fx = (short)blocks[0].lpData[bufferPos * BYTES_PER_SAMPLE];
			t = sample * SECONDS_PER_SAMPLE;
			w = 2 * PI * freqNumb * (V_RESOLUTION + 1);
			frequencies[freqNumb] += fx * SECONDS_PER_SAMPLE * cos((-1) * w * t);
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