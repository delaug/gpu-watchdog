#pragma once
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <nvml.h>
#include <fstream>
#include <string>
#include <Windows.h>
#include <WinInet.h>
#include <ctime>

#define MAX_MINER_COUNT 16 // Максимальное кол-во запускаемых приложений

using namespace std;

// Структура конфигурации
struct cConfig {
	string miner[MAX_MINER_COUNT];
	unsigned int minTemp;
	unsigned int maxTemp;
	unsigned int interval;
	unsigned int statisticInterval;
	unsigned int minUtilizationRate;
	unsigned int startPause;
	unsigned int minerCount;
};

// Структура статистики
struct cStatistics {	
	unsigned int restarts;
	unsigned int nvmlInit;
	unsigned int nvmlDeviceGetHandleByIndex;
	unsigned int nvmlDeviceGetUtilizationRates;
};

class cWatchdog
{
public:
	// Работа с цветами вывода в консоль
	enum ConsoleColor
	{
		Black = 0,
		Blue = 1,
		Green = 2,
		Cyan = 3,
		Red = 4,
		Magenta = 5,
		Brown = 6,
		LightGray = 7,
		DarkGray = 8,
		LightBlue = 9,
		LightGreen = 10,
		LightCyan = 11,
		LightRed = 12,
		LightMagenta = 13,
		Yellow = 14,
		White = 15
	};

	cWatchdog();
	~cWatchdog();

	void SetColor(ConsoleColor text, ConsoleColor background);
	void ReadConfig();

	void GetTime();
	void WriteLog(string text);
	void WriteStatistics();
	void CloseMiner();

	int Run();
	int RestartMiner();
	void Shutdown();
private:
	cConfig config;
	cStatistics statistics;
	nvmlReturn_t result;
	ofstream logFile;
	ofstream statisticsFile;
	string datetime;
	time_t date;

	unsigned int gpuAttempts;
	unsigned int connectionAttempts;
};

