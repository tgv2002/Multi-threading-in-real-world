#define _POSIX_C_SOURCE 199309L
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

// colors for color-coding
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[92m"
#define COLOR_DEEP_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_BRIGHT_MAGENTA "\x1b[95m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

// semaphores to be used
sem_t acoustic_stage;
sem_t electric_stage;
sem_t coordinators;
sem_t musiciansPerformingWithSingers;

// locks for performers and stages
pthread_mutex_t* perf_lock;
pthread_mutex_t* stage_lock;
pthread_mutex_t lock_for_all;

int k, a, e, c, t1, t2, t;
char* inst_type;
char** name;
int* arrival;
int* isSinger;
int* singer_partner;
int* stage_type;
int* perf_status;
int* stage_num;
int* stage_is_free;
int* stage_performer;

struct arg{
	int id;
};

struct arg2{
	int id;
	int stage_mode;
};

struct arg* perfs;
struct arg* collector;
struct arg2* searchA;
struct arg2* searchE;

/* print functions for formatting and printing */

void printMusicianArrives(char* mus_name, char instrument)
{
	printf(COLOR_GREEN "%s %c arrived" COLOR_RESET "\n", mus_name, instrument); // musician arrived
}

void printPerformanceStarts(char* mus_name, int stagey, int instrument, int duration)
{
	// performance begins, color coded according to stage type

	if(stagey >= 1 && stagey <= a)
	{
		printf(COLOR_CYAN "%s performing %c at acoustic stage %d for %d sec" COLOR_RESET "\n",
				mus_name, instrument, stagey, duration);
	}

	else
	{
		printf(COLOR_YELLOW "%s performing %c at electric stage %d for %d sec" COLOR_RESET "\n",
				mus_name, instrument, stagey, duration);    
	}
}

void printPerformanceEnds(char* mus_name, int stagey)
{
	// performance ends, color coded according to stage type

	if(stagey >= 1 && stagey <= a)
	{
		printf(COLOR_CYAN "%s performance at acoustic stage %d finished" COLOR_RESET "\n",
				mus_name, stagey);
	}

	else
	{
		printf(COLOR_YELLOW "%s performance at electric stage %d finished" COLOR_RESET "\n",
				mus_name, stagey);
	}
}

void printCollectTShirt(char* mus_name)
{
	// t-shirt collected after performance
	printf(COLOR_MAGENTA "%s collecting t-shirt" COLOR_RESET "\n", mus_name);
}

void printLeftImpatiently(char* mus_name, char instrument)
{
	// impatient musician left after waiting for t secs
	printf(COLOR_BRIGHT_MAGENTA "%s %c left because of impatience" COLOR_RESET "\n", mus_name, instrument);
}

void printSingerJoined(char* sing_name, char* mus_name)
{
	// singer joined performance
	printf(COLOR_DEEP_GREEN "%s joined %s's performance, performance extended by 2 secs" COLOR_RESET "\n", sing_name, mus_name);
}

void printStartOrEnd(int isStart)
{   
	// printed at beginning
	if(isStart == 1)
	{
		printf(COLOR_RED "Event has started. Enjoy the performances :)" COLOR_RESET "\n");
	}

	// printed at end
	else
	{
		printf(COLOR_RED "Finished" COLOR_RESET "\n");
	}
}

int returnRandom(int lower, int upper) 
{ 
	// randomize in a given range of integers

	int num = (rand() % (upper - lower + 1)) + lower; 
	return num;  
}

int eligibleStages(char c)
{
	if(c == 's')
		return 3;       // special case
	if(c == 'p' || c == 'g')
		return 2;       // both electric and acoustic stages are eligible
	if(c == 'v')
		return 0;       // only acoustic stage is eligible
	return 1;           // only electric stage is eligible
}

void freeStage(int stageNum)
{
	// reset the stage

	stage_is_free[stageNum] = 1;
	stage_performer[stageNum] = -1;    
}

void* getTshirtAlready(void* collectors)
{
	struct arg* curr_perf = (struct arg*) collectors;
	int curr_id = curr_perf->id;

	sem_wait(&coordinators);		// current participant arrives and waits for a coordinator to give him a t-shirt
	printCollectTShirt(name[curr_id]);	// t-shirt collected by perfomer
	sleep(2);		// coordinator giving performer appropriate sized t-shirt
	sem_post(&coordinators);		// signalling that a coordinator is free now
	return NULL;
}

void* performOnSpecificStage(void* performers)
{
	struct arg2* curr_perf = (struct arg2*) performers;
	int perf_id = curr_perf->id;       // store id of performer
	int mode = curr_perf->stage_mode;       // store current (specific) stage type - acoustic or electric
	int performanceDuration;        // denotes the duration of current performance about to start
	int stageChosen;

	struct timespec seeTime;
	clock_gettime(CLOCK_REALTIME, &seeTime);		// checking if the patience of performer has exhausted (t secs are passed or not)
	seeTime.tv_sec = seeTime.tv_sec + t;
	int timeout;
	if(mode == 0)
		timeout = sem_timedwait(&acoustic_stage, &seeTime);
	else
	{
		timeout = sem_timedwait(&electric_stage, &seeTime);
	}

	pthread_mutex_lock(&perf_lock[perf_id]);

	if(timeout == -1)
	{
		// performer not allotted a stage yet, performer leaves

		if(perf_status[perf_id] == 0)       // if a stage has not been allotted to performer yet, it means he leaves impatiently
		{
			perf_status[perf_id] = -1;      // indicates that performer left
			printLeftImpatiently(name[perf_id], inst_type[perf_id]);
		}

		pthread_mutex_unlock(&perf_lock[perf_id]);
		return NULL;
	}

	if(perf_status[perf_id] != 0)
	{
		// if the performer has already left in some version / got assigned a stage, this version should be discarded

		if(mode == 0)
			sem_post(&acoustic_stage);
		else                                // undo the previous timed wait before leaving by signalling
		{
			sem_post(&electric_stage);
		}

		pthread_mutex_unlock(&perf_lock[perf_id]);
		return NULL;
	}

	// if this point is reached, the stage has not been assigned yet and the performer didn't leave impatiently yet

	for(int i=1;i<=a+e;i++)
	{
		pthread_mutex_lock(&stage_lock[i]);     // locking stage, as its shared variables are accessed

		if(stage_type[i] == mode && stage_is_free[i])       // performer can choose this stage
		{
			perf_status[perf_id] = 1;           // stage assigned to performer
			stage_num[perf_id] = i;
			stage_performer[i] = perf_id;       // assigning performer to stage and vice versa

			if(!isSinger[perf_id])
			{
				stage_is_free[i] = 2;       // 2 indicates that a singer can join this performance
				sem_post(&musiciansPerformingWithSingers);  // signalling that singer can perform with musicians on this stage
			}

			else
			{
				stage_is_free[i] = 0;       // 0 indicates that a singer cannot join this musician's performance
			}

			pthread_mutex_unlock(&stage_lock[i]);
			stageChosen = i;
			break;
		}

		pthread_mutex_unlock(&stage_lock[i]);
	}

	pthread_mutex_unlock(&perf_lock[perf_id]);      // unlocking performer, as critical section ends here

	performanceDuration = returnRandom(t1, t2);
	printPerformanceStarts(name[perf_id], stageChosen, inst_type[perf_id], performanceDuration);
	sleep(performanceDuration);     // performance is going on for 'performanceDuration' seconds

	// check if a singer has joined this musician

	pthread_mutex_lock(&perf_lock[perf_id]);
	pthread_mutex_lock(&stage_lock[stageChosen]);       // to secure global arrays access

	int twoPerformers = 0;

	if(singer_partner[perf_id] != -1)       // if a partner (singer) performed with this musician
	{
		twoPerformers = 1;      // there are two performers on chosen stage now
		pthread_mutex_unlock(&stage_lock[stageChosen]);
		pthread_mutex_unlock(&perf_lock[perf_id]);      // performance not done yet, hence sleep shouldn't be encapsulated in lock
		sleep(2);       // extended performance due to singer
		pthread_mutex_lock(&perf_lock[perf_id]);
		pthread_mutex_lock(&stage_lock[stageChosen]);       // lock it back to re-update global variables after performance
	}

	int retVal;

	if(!isSinger[perf_id] && singer_partner[perf_id] == -1)
	{
		// non-singer musician finished his solo performance, so the number of solo performances a singer can join reduces by 1
		retVal = sem_trywait(&musiciansPerformingWithSingers);

		if(retVal < 0)		// this is the only musician left and a singer wants to join this stage itself
		{
			twoPerformers = 1;      // there are two performers on chosen stage now
			pthread_mutex_unlock(&stage_lock[stageChosen]);
			pthread_mutex_unlock(&perf_lock[perf_id]);      // performance not done yet, hence sleep shouldn't be encapsulated in lock
			sleep(2);       // extended performance due to singer
			pthread_mutex_lock(&perf_lock[perf_id]);
			pthread_mutex_lock(&stage_lock[stageChosen]);       // lock it back to re-update global variables after performance	
		}
	}

	freeStage(stageChosen);     // leave the stage after performing

	if(mode == 0)
		sem_post(&acoustic_stage);      
	else                    // signalling that this stage of given mode is now open to be utilized
	{
		sem_post(&electric_stage);    
	}

	if(singer_partner[perf_id] != -1)			// if a singer joined this performer, his performance is also finished
	{
		pthread_mutex_lock(&perf_lock[singer_partner[perf_id]]);
		printPerformanceEnds(name[singer_partner[perf_id]], stageChosen);
		perf_status[singer_partner[perf_id]] = -1;		// indicates that the singer who partnered with musician has left the stage
		pthread_mutex_unlock(&perf_lock[singer_partner[perf_id]]);
	}

	printPerformanceEnds(name[perf_id], stageChosen);
	perf_status[perf_id] = -1;			// indicates that this performer has left the stage
	pthread_mutex_unlock(&stage_lock[stageChosen]);		// unlock stage and performer, as critical section ends here
	pthread_mutex_unlock(&perf_lock[perf_id]);

	/* t-shirt collection procedure begins here */

	pthread_t perff[2];

	if(c > 0)
	{
		for(int j=0;j<=twoPerformers;j++)
		{
			int curr_id = (j == 0) ? (perf_id) : (singer_partner[perf_id]);
			collector[curr_id].id = curr_id;
			pthread_create(&perff[j], NULL, getTshirtAlready, &collector[curr_id]);		// trying to collect t-shirt from coordinator
		}

		for(int j=0;j<=twoPerformers;j++)
		{
			int curr_id = (j == 0) ? (perf_id) : (singer_partner[perf_id]);
			pthread_join(perff[j], NULL);			// waiting for t-shirt collection process to end
		}
	}

	// performances finished, t-shirts collected, performer(s) leave the event
	return NULL;
}

void* performWithMusician(void* performers)
{
	/* depicts a singer trying to join a musician's solo performance */

	struct arg* curr_perf = (struct arg*) performers;
	int perf_id = curr_perf->id;       // store id of singer

	struct timespec seeTime;
	clock_gettime(CLOCK_REALTIME, &seeTime);	// checking if the patience of singer has exhausted (t secs are passed or not)
	seeTime.tv_sec = seeTime.tv_sec + t;
	int timeout = sem_timedwait(&musiciansPerformingWithSingers, &seeTime);

	pthread_mutex_lock(&perf_lock[perf_id]);

	if(timeout == -1)
	{
		if(perf_status[perf_id] == 0)		// if stage not assigned to singer yet, and his patience has exhausted, he leaves
		{
			printLeftImpatiently(name[perf_id], inst_type[perf_id]);
			perf_status[perf_id] = -1;	// indicates singer left
		}

		pthread_mutex_unlock(&perf_lock[perf_id]);
		return NULL;
	}

	if(perf_status[perf_id] != 0)
	{
		// if a stage has been assigned to singer or he left in some other version, this version should be discarded
		sem_post(&musiciansPerformingWithSingers); 		// undo the previous wait for a musician who the singer can join
		pthread_mutex_unlock(&perf_lock[perf_id]);
		return NULL;
	}

	// this version is the version finally considered for the singer

	for(int i=1;i<=a+e;i++)
	{
		pthread_mutex_lock(&stage_lock[i]);

		if(stage_is_free[i] == 2)		// solo performance of musician going on, singer can join this stage
		{
			printSingerJoined(name[perf_id], name[stage_performer[i]]);

			singer_partner[stage_performer[i]] = perf_id;	// this singer is the partner of musician performing on this stage now
			perf_status[perf_id] = 1;		// a stage is assigned to this singer
			stage_num[perf_id] = i;		// stage number assigned to this singer
			stage_is_free[i] = 0;		// this stage's performance cannot be joined by a singer now
			pthread_mutex_unlock(&stage_lock[i]);
			break;
		}

		pthread_mutex_unlock(&stage_lock[i]);
	}

	pthread_mutex_unlock(&perf_lock[perf_id]);		// critical section ends here
	return NULL;
}

void* srujana(void* performers)
{
	struct arg* curr_perf = (struct arg*) performers;
	int perf_id = curr_perf->id;       // store id of performer
	sleep(arrival[perf_id]);        // performer arrives when time = arrival secs
	printMusicianArrives(name[perf_id], inst_type[perf_id]);

	pthread_mutex_lock(&perf_lock[perf_id]);
	int eligibilities = eligibleStages(inst_type[perf_id]);     // get stages on which he can perform
	searchE[perf_id].id = perf_id;
	searchE[perf_id].stage_mode = 1;
	searchA[perf_id].id = perf_id;
	searchA[perf_id].stage_mode = 0;
	pthread_mutex_unlock(&perf_lock[perf_id]);

	pthread_t searchForAcoustic;
	pthread_t searchForElectric;
	pthread_t joinSomePerformer;    // creating atmost three versions of a performer, who search for an acoustic/electric stage or can join a performer

	if(eligibilities != 0)
		pthread_create(&searchForElectric, NULL, performOnSpecificStage, &searchE[perf_id]);
	if(eligibilities == 3)
		pthread_create(&joinSomePerformer, NULL, performWithMusician, &perfs[perf_id]);
	if(eligibilities != 1)
		pthread_create(&searchForAcoustic, NULL, performOnSpecificStage, &searchA[perf_id]);      // dispatching possible versions
	if(eligibilities != 0)
		pthread_join(searchForElectric, NULL);
	if(eligibilities == 3)
		pthread_join(joinSomePerformer, NULL);
	if(eligibilities != 1)
		pthread_join(searchForAcoustic, NULL);      // waiting for the threads to terminate

	return NULL;
}

int main()
{
	srand(time(0));		// for randomization purposes

	// take inputs
	int inp = scanf("%d %d %d %d %d %d %d", &k, &a, &e, &c, &t1, &t2, &t);

	if(inp != 7)
	{
		printf(COLOR_RED "INVALID ARGUMENTS !!!!!!!!!" COLOR_RESET "\n");
		return 0;
	}

	arrival = (int *)malloc((k+1)*sizeof(int));
	perf_status = (int *)malloc((k+1)*sizeof(int));
	stage_num = (int *)malloc((k+1)*sizeof(int));
	isSinger = (int *)malloc((k+1)*sizeof(int));
	singer_partner = (int *)malloc((k+1)*sizeof(int));
	stage_type = (int *)malloc((a+e+1)*sizeof(int));
	stage_is_free = (int *)malloc((a+e+1)*sizeof(int));
	stage_performer = (int *)malloc((a+e+1)*sizeof(int));
	inst_type = (char *)malloc((k+1)*sizeof(char));
	name = (char **)malloc((k+1)*sizeof(char *));

	perfs = malloc((k+1)*sizeof(struct arg));
	collector = malloc((k+1)*sizeof(struct arg));
	searchA = malloc((k+1)*sizeof(struct arg2));
	searchE = malloc((k+1)*sizeof(struct arg2));


	pthread_t performer[k+1];
	perf_lock = (pthread_mutex_t *)malloc((k+1)*sizeof(pthread_mutex_t));
	stage_lock = (pthread_mutex_t *)malloc((a+e+1)*sizeof(pthread_mutex_t));

	for(int i=0;i<=k;i++)
		name[i] = (char *)malloc((102)*sizeof(char));

	for(int i=1;i<=k;i++)
	{
		scanf("%s %c %d", name[i], &inst_type[i], &arrival[i]);
		isSinger[i] = (inst_type[i] == 's') ? (1) : (0);    // if a performer is a singer
		singer_partner[i] = -1;      // no musician has a singing partner initially
		perf_status[i] = 0;      // describes if stage is assigned to him / not assigned / he left
		stage_num[i] = -1;      // describes stage number on which musician is performing
		pthread_mutex_init(&perf_lock[i], NULL);
	}

	pthread_mutex_init(&lock_for_all, NULL);    // to lock global variables

	for(int i=1;i<=a+e;i++)
	{
		stage_is_free[i] = 1;       // describes if stage is free
		stage_performer[i] = -1;        // describes id of performer performing on this stage
		pthread_mutex_init(&stage_lock[i], NULL);
	}

	for(int i=1;i<=a;i++)
		stage_type[i] = 0;      // 0 represents acoustic stage
	for(int i=a+1;i<=a+e;i++)
		stage_type[i] = 1;      // 1 represents electric stage

	sem_init(&musiciansPerformingWithSingers, 0, 0);    // represents number of performances singer can join
	if(c > 0)
		sem_init(&coordinators, 0, c);              // represents c cooridnators
	sem_init(&acoustic_stage, 0, a);            // represents acoustic stages available
	sem_init(&electric_stage, 0, e);            // represents electric stages available

	printStartOrEnd(1);

	if(c == 0)
		printf(COLOR_RED "Only passionate performers are allowed. No t-shirts would be distributed" COLOR_RESET "\n");

	for(int i=1;i<=k;i++)
	{
		perfs[i].id = i;
		pthread_create(&performer[i], NULL, srujana, &perfs[i]);
	}

	for(int i=1;i<=k;i++)
	{
		pthread_join(performer[i], NULL);
	}

	pthread_mutex_destroy(&lock_for_all);
	for(int i=1;i<=a+e;i++)
		pthread_mutex_destroy(&stage_lock[i]);		// destroy all locks
	for(int i=1;i<=k;i++)
		pthread_mutex_destroy(&perf_lock[i]);

	sem_destroy(&musiciansPerformingWithSingers);
	if(c > 0)
		sem_destroy(&coordinators);
	sem_destroy(&acoustic_stage);
	sem_destroy(&electric_stage);		// destroy all semaphores
	printStartOrEnd(0);

	return 0;
}
