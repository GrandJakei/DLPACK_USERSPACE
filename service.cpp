#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cassert>
#include<queue>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<semaphore.h>
#include <sys/fcntl.h>
#include<fcntl.h>
using namespace std;

//用以标识是否已经改变的 信号量
sem_t first;
sem_t second;
//条件变量需要配套的互斥锁
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int lastPos = 0;
vector<string> strEvent = {"$T ATOMIC_0", "$T ATOMIC_1" , "$T ATOMIC_2", "$T ATOMIC_3", "$T ATOMIC_4"};
string evevt_path = "event.txt";
string data_path = "data.txt";
string path = "/tmp/dlpack_pid";
int sig1 = 0;
int sig2 = 1;
int* buf = NULL;
int shmid;
queue<string> qEvent;

//监控数据结构体和重载的操作符
class SceneData {
	public:
		double speed;
		int driverInCar;
		int wifi;
		int otaUpdate; 
		SceneData():speed(0), driverInCar(0), wifi(0), otaUpdate(0) { }
		bool operator == (const SceneData &s_data)
		{
			if (speed == s_data.speed && driverInCar == s_data.driverInCar 
			&& wifi == s_data.wifi && otaUpdate == s_data.otaUpdate) {
				return true;
			}
			return false;
		}
		void operator = (const SceneData &s_data) {
			speed = s_data.speed;
			driverInCar = s_data.driverInCar; 
			wifi = s_data.wifi; 
			otaUpdate = s_data.otaUpdate;
		}
};

//初始化服务
int initService() {

	// 创建pid文件
	fstream pid_write;
	pid_write.open(path, ios::in | ios::out |ios::trunc);
	pid_write << getpid();
	cout << "Service initialization completed!" << endl;
	return 1;

	//创建共享变量的操作
}

static void handleSignal(int signo)
{
	if(signo == SIGUSR1) {
		cout << "策略加载...\n";
		sig1 = 1;
		sig2 = 0;
		//
	}
	if (signo == SIGUSR2) {
		cout << "策略加载完成\n";
		sig1 = 0;
		sig2 = 1;
		ofstream write;
		write.open(evevt_path, ios::app);
		//文件没打开
		if (!write) {
			cout << " handleSignal error!" << endl;
			return;
		}
		string event = "";
		
		while (!qEvent.empty()) {
			event = qEvent.front();
			write << event;
			qEvent.pop();
		}
	}		
}


//事件处理函数, 根据场景信息向内核文件传入事件信号
string getEvent(struct SceneData* last, struct SceneData* cur) {
    string event = "";
	//行驶中
	if (last->speed > 0 && cur->speed == 0) {
		event = event + strEvent[0]  + "\n";
	}
	if (last->speed == 0 && cur->speed > 0) {
		event = event + strEvent[0]  + "\n";
	}

	//高速
	if (last->speed < 150 && cur->speed >= 150) {
		event = event + strEvent[1]  + "\n";
	}
	//正常速度
	if (last->speed >= 150 && cur->speed < 150) {
		event = event + strEvent[1] + "\n";
	}
	
	//驾驶员是否车内
	if (last->driverInCar != cur->driverInCar) {
		if (cur->driverInCar) {
			event = event + strEvent[2] + "\n";
		}
		else {
			event = event + strEvent[2] + "\n";
		}
	}
	
	//wifi是否连接
	if (last->wifi != cur->wifi) {
		if (cur->wifi) {
			event = event + strEvent[3] + "\n";
		}
		else {
			event = event + strEvent[3] + "\n";
		}
	}
	
	//OTA是否更新
	if (last->otaUpdate != cur->otaUpdate) {
		if (cur->otaUpdate) {
			event = event + strEvent[4] + "\n";
        }
		else {
			event = event + strEvent[4] + "\n";
		}
	}
	
    return event;
}

//获取实时场景信息线程
int getScenceData(struct SceneData* curData, int lastPos) {
	struct flock lock;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_type = F_RDLCK;
	lock.l_pid = getpid();
	int fd;
	fd = open(data_path.c_str(), O_RDWR | O_CREAT, 0666);
	//文件没打开
	if (fd < 0) {
		perror("open failed");
		return -1;
	}
	fcntl(fd, F_SETLKW, &lock);
	ifstream read;
	read.open(data_path, ios::app);
    read.seekg(lastPos, ios::beg);

	read >> curData->speed;
	read >> curData->driverInCar;
	read >> curData->wifi;
	read >> curData->otaUpdate;

	// cout << curData->speed << " " << curData->driverInCar<< " " << curData->wifi<< " " << curData->otaUpdate << endl;

    lastPos = read.tellg();
	read.close();

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &lock);

    return lastPos;
}

//触发事件处理函数线程, 当场景信息改变, 即触发事件处理函数
void* sendEvent(void *arg) {    
    SceneData* curData = (SceneData *)arg;
    SceneData* lastData = new SceneData();
    while (1) {
		sem_wait(&second);
		
		pthread_mutex_lock(&lock);
		sendEvent(lastData, curData);
		string event = "";
		event = getEvent(last, cur);
		if(sig1) {
			qEvent.push(event);
		}
		if (sig2) {
			ofstream write;
			write.open(evevt_path, ios::app);
			//文件没打开
			if (!write) {
				cout << " Send event error!" << endl;
				return;
			}

			write << event;
			write.close();
		}

		*lastData = *curData;
		pthread_mutex_unlock(&lock);

		sem_post(&first);
	}
	return NULL;
}

int main() {
	//初始化 
	struct SceneData s_data;
	pthread_t sendEventid;
	pthread_mutex_init(&lock,NULL);
	sem_init(&first,0,1);
	sem_init(&second,0,0);

    initService();
	
	//signal
	if(signal(SIGUSR1, handleSignal) == SIG_ERR) {
		printf("can not catch SIGUSR1\n");
	}
	if(signal(SIGUSR2, handleSignal) == SIG_ERR) {
		printf("can not catch SIGUSR1\n");
	}
        
	//thread
	pthread_create(&sendEventid, NULL, sendEvent, (void *)&s_data);
	srand(time(NULL));

	//双线程
    while (1) {
		sem_wait(&first);
		sleep(5);
		pthread_mutex_lock(&lock);
        getScenceData(&s_data, lastPos); 
		lastPos = 0;
		pthread_mutex_unlock(&lock);

		sem_post(&second);
    }
    cout << "hang up" <<endl;
	
    //回收子线程
	pthread_join(scanthreadid, NULL);

    //清除资源 
    pthread_mutex_destroy(&lock);
    return 0;
}
