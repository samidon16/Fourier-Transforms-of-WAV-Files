#pragma once

//Standard included files
#include <iostream>
#include <Windows.h>
#include <Audioclient.h>
#include <mmsystem.h>
#include <windows.h>
#include <cmath>
#include <fstream>
#include "WavFile.h"

//Macros
#define NUMB_BLOCKS 2
#define SAMPLES_PER_SECOND 44100.0
#define SECONDS_PER_SAMPLE 1 / SAMPLES_PER_SECOND
#define BYTES_PER_SAMPLE 2
#define PI 3.14159265359
#define E 2.718281828459
#define BIT_DEPTH pow(2, 16)
#define BLOCK_LENGTH 10000
//V_RESOLUTION is the Hz increment between successive frequency calculations.
#define V_RESOLUTION 10
#define UPPER_FREQUENCY 30000
#define NUMB_FREQUENCIES (UPPER_FREQUENCY / V_RESOLUTION)