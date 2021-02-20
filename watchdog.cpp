#include "watchdog.h"

cWatchdog::cWatchdog(){	
	SetColor(Yellow, Black);
	connectionAttempts = 0;
	gpuAttempts = 0;

	// Инициализация статистики
	statistics.restarts = 0;
	statistics.nvmlInit = 0;
	statistics.nvmlDeviceGetHandleByIndex = 0;
	statistics.nvmlDeviceGetUtilizationRates = 0;

	// Дефолтные настройки
	ZeroMemory(config.miner, sizeof(string)*MAX_MINER_COUNT);	 
	config.minTemp = 30;
	config.maxTemp = 80;
	config.interval = 1000;
	config.statisticInterval = 3600;
	config.minUtilizationRate = 90;	
	config.startPause = 30000;
	config.minerCount = 0;
	GetTime();
	WriteLog("Start GPU watchdog...\n");	
}

cWatchdog::~cWatchdog(){
	WriteLog("Stop GPU watchdog.\n");
}

void cWatchdog::SetColor(ConsoleColor text, ConsoleColor background){
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdOut, (WORD)((background << 4) | text));
}

void cWatchdog::ReadConfig() {
	ifstream configFile;
	string output, valName;
	// Очищаем список майнеров и счётчик
	ZeroMemory(config.miner, sizeof(string)*MAX_MINER_COUNT);
	config.minerCount = 0;

	configFile.open("config.cfg");
	if (configFile.is_open()) {
		while (!configFile.eof()) {
			configFile >> output;			
			if (valName == "miner") {
				if (config.miner[config.minerCount] == "" && config.minerCount < MAX_MINER_COUNT) {
					config.miner[config.minerCount] = output;
					config.minerCount++;
				}
			}
			if (valName == "minTemp") {
				config.minTemp = atoi(output.c_str());
			}
			if (valName == "maxTemp") {
				config.maxTemp = atoi(output.c_str());
			}
			if (valName == "interval") {
				config.interval = atoi(output.c_str());
			}
			if (valName == "minUtilizationRate") {
				config.minUtilizationRate = atoi(output.c_str());
			}
			if (valName == "startPause") {
				config.startPause = atoi(output.c_str());
			}
			if (valName == "statisticInterval") {
				config.statisticInterval = atoi(output.c_str());
			}
			valName = output;
		}
		configFile.close();
	}
	else {
		WriteLog("Config file not found and will be creted!\n");
		ofstream newConfigFile;
		newConfigFile.open("config.cfg");
		newConfigFile << "miner \"C:\\Users\\sergey\\Desktop\\Miner\\BTG.bat\"\n\
minTemp 40\n\
maxTemp 90\n\
interval 10000\n\
statisticInterval 3600\n\
minUtilizationRate 10\n\
startPause 30000\n";
			
		newConfigFile.close();
	}
	
	for (int i = 0; i < config.minerCount; i++) {
		cout << config.miner[i] << endl;
	}
}

void cWatchdog::GetTime(){
	SYSTEMTIME st;
	datetime = "";
	date = 0;
	char d[11];
	char t[10];	
	GetLocalTime(&st);

	GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "dd-MM-yyyy", d, 255);
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, " HH:mm:ss", t, 255);
	datetime.append(d);
	date = time(NULL);
	datetime.append(t);
}

void cWatchdog::WriteLog(string text){	
	printf(text.c_str());
	logFile.open("log.log", ios::out | ios::app);
	string buf = datetime+" ";
	logFile << buf.append(text);
	logFile.close();	
}

void cWatchdog::WriteStatistics(){	
	while (true) {
		Sleep(1000);
		GetTime();
		if (date % config.statisticInterval == 1) {	// Каждые 3 часа
			char bufStr[255];
			statisticsFile.open("statistics.log", ios::out | ios::app);
			string buf = datetime + "\n";
			snprintf(bufStr, sizeof(bufStr), "Restarts: %d\nnvmlInit: %d\nnvmlDeviceGetHandleByIndex: %d\nnvmlDeviceGetUtilizationRates: %d\n\n", statistics.restarts, statistics.nvmlInit, statistics.nvmlDeviceGetHandleByIndex, statistics.nvmlDeviceGetUtilizationRates);
			statisticsFile << buf.append(bufStr);
			statisticsFile.close();

			// Сброс статистики
			statistics.restarts = 0;
			statistics.nvmlInit = 0;
			statistics.nvmlDeviceGetHandleByIndex = 0;
			statistics.nvmlDeviceGetUtilizationRates = 0;
		}
	}
}

void cWatchdog::CloseMiner(){
	system("taskkill /f /im miner.exe >nul");
	WriteLog("Close Miner.\n");
}

int cWatchdog::Run(){	
	ReadConfig();
	Sleep(config.startPause);
	unsigned int device_count, i, GPU_Rate;	
	char bufStr[255];

	// First initialize NVML library
	result = nvmlInit();
	if (NVML_SUCCESS != result){		
		snprintf(bufStr, sizeof(bufStr), "Failed to initialize NVML: %s\n", nvmlErrorString(result));		
		WriteLog(bufStr);		
		statistics.nvmlInit++;
		return 1;
	}

	result = nvmlDeviceGetCount(&device_count);
	if (NVML_SUCCESS != result)
	{
		snprintf(bufStr, sizeof(bufStr), "Failed to query device count: %s\n", nvmlErrorString(result));
		WriteLog(bufStr);
		Shutdown();
	}
	SetColor(Yellow, Black);	

	snprintf(bufStr, sizeof(bufStr), "Found %d device%s\n", device_count, device_count != 1 ? "s" : "");	
	WriteLog(bufStr);	
	WriteLog("Listing devices:\n");

	while (true)
	{			
		GetTime();
		GPU_Rate = 0;
		connectionAttempts = 0;
		for (i = 0; i < device_count; i++)
		{
			nvmlDevice_t device;
			unsigned int temp;			

			result = nvmlDeviceGetHandleByIndex(i, &device);
			if (NVML_SUCCESS != result)
			{
				snprintf(bufStr, sizeof(bufStr), "Failed to get handle for device %i: %s\n", i, nvmlErrorString(result));
				WriteLog(bufStr);
				Shutdown();				
				statistics.nvmlDeviceGetHandleByIndex++;
				return 1;
			}

			result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
			if (NVML_SUCCESS != result) {				
				snprintf(bufStr, sizeof(bufStr), "Failed to get temperature of device %i: %s\n", i, nvmlErrorString(result));
				WriteLog(bufStr);
			}

			// Загруженность процессора и памяти

			nvmlUtilization_t nvmlUtilization = { 0 };
			result = nvmlDeviceGetUtilizationRates(device, &nvmlUtilization);
			if (result != NVML_SUCCESS) {				
				snprintf(bufStr, sizeof(bufStr), "nvmlDeviceGetUtilizationRates: %s\n", nvmlErrorString(result));
				WriteLog(bufStr);
				Sleep(60000);				
				Shutdown();				
				statistics.nvmlDeviceGetUtilizationRates++;
				return 1;
			}
			GPU_Rate += nvmlUtilization.gpu;
			
			if (nvmlUtilization.gpu >= config.minUtilizationRate)
				SetColor(Green, Black);
			else 
				SetColor(Red, Black);
			// Вывод статистики			
			snprintf(bufStr, sizeof(bufStr),"Devices %d temp: %d GPU: %u gmem: %u\n", i, temp, nvmlUtilization.gpu, nvmlUtilization.memory);
			WriteLog(bufStr);
		}

		snprintf(bufStr, sizeof(bufStr), "Total rate GPU: %d\n", (GPU_Rate / device_count));
		WriteLog(bufStr);

		if ((GPU_Rate / device_count) >= config.minUtilizationRate) {
			SetColor(Green, Black);
			gpuAttempts = 0;
		}
		else {
			SetColor(Red, Black);
			while (true)
			{
				// Если загруженность упала, проверяем интернет соединение
				if (!InternetCheckConnection("http://google.com", FLAG_ICC_FORCE_CONNECTION, 0)) {
					CloseMiner();
					if (connectionAttempts == 0) {
						WriteLog("Internet: Lost connection...\n");
					}
					else {
						snprintf(bufStr, sizeof(bufStr), "Attempt: %d\n", connectionAttempts);
						WriteLog(bufStr);
					}
				}
				else {					
					break;
				}
				connectionAttempts++;
				Sleep(config.interval);
			}
			gpuAttempts++;
			// Если попытки соединения не удалить ребутим Майнер и продолжаем
			if (connectionAttempts > 0 || gpuAttempts >= 3) {
				if (RestartMiner()) {
					WriteLog("Restart Miner\n");
					connectionAttempts = 0;
					WriteLog("Waiting 60 sec...\n");
					gpuAttempts = 0;
					Sleep(60000);
				}
			}
		}
		Sleep(config.interval);
	}

	Shutdown();
	return 0;
}

int cWatchdog::RestartMiner(){
	CloseMiner();
	Sleep(5000);
	if (config.minerCount > 0){
		for (int i = 0; i < config.minerCount; i++){
			WriteLog("Start: " + config.miner[i] + "\n");
			ShellExecute(NULL, "open",
				config.miner[i].c_str(),
				0,
				0,
				SW_SHOWNORMAL
			);
		}
	}
	else {
		WriteLog("Error! Write miner path(s)");
	}

	statistics.restarts++;
	return 1;
}

void cWatchdog::Shutdown(){
	char bufStr[255];

	result = nvmlShutdown();
	if (NVML_SUCCESS != result) {
		snprintf(bufStr, sizeof(bufStr), "Failed to shutdown NVML: %s\n", nvmlErrorString(result));
		WriteLog(bufStr);
	}	
}