#include "watchdog.h"
#include <thread>

using namespace std;
cWatchdog watchdog;

// Функция потока
void threadFunction() {
	watchdog.WriteStatistics();
}

int main()
{
	// Создаём поток для статистики
	thread thr(threadFunction);
	thr.detach();	

	unsigned int result = 1;
	while (result != 0) {
		result = watchdog.Run();		
	}

	return 0;
}