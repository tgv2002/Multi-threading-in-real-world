#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

int n,m,o;
long double* probs;
int* curr_batch;
int* vacc_in_batch;
int* slots;
int* zone_vaccines;
int waiting_studs = 0;
int* attempt_count;
int* succ;
int* zone_curr_company;
int* inVaccPhase;
int* alloc_slots;
int* store;
int doneCase = 0;
struct arg* a;
struct arg* b;
struct arg* zz;
int registered = 0;

pthread_mutex_t* stud_lock;
pthread_mutex_t* zone_lock;
pthread_mutex_t* comp_lock;
pthread_mutex_t lock_for_all;

struct arg{
// id of company / student / zone
	int id;
};

int returnRandom(int lower, int upper) 
{
	// returns random integers in given inclusive range
	int num = (rand() % (upper - lower + 1)) + lower; 
	return num;  
}

int vaccSuccessful(int comp)
{
	// determines if a vaccination to a student was successful

	long double antibod = ((long double)rand()/(long double)(RAND_MAX));

	if(antibod > probs[comp])
		return 0;		// vaccination didn't work
	else
	{
		return 1;		// vaccination injected anti-bodies
	}
}

void produceVaccines(int i)
{
	// produces a random number of batches of vaccines, for a given company number i

	int batches = returnRandom(1, 5);		// number of batches going to be prepared
	printf("Pharmaceutical Company %d is preparing %d batches of vaccines which have success probability %Lf\n",
			i, batches, probs[i]);
	sleep(returnRandom(2, 5));	// simulating vaccine production
	printf("Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %Lf. Waiting for all the vaccines to be used to resume production\n",
			i, batches, probs[i]);
	pthread_mutex_lock(&comp_lock[i]);
	curr_batch[i] = batches;				// denotes current number of batches it possesses
	vacc_in_batch[i] = returnRandom(10, 20);	// denotes number of vaccines in a batch
	pthread_mutex_unlock(&comp_lock[i]);	
}

int updateSlots(int i, int inQueue)
{
	// updates the slots for a zone i, if it has some vaccines with it
	pthread_mutex_lock(&zone_lock[i]);
	int minn = (inQueue < zone_vaccines[i]) ? 
		(inQueue) : (zone_vaccines[i]);
	minn = (minn < 8) ? minn : 8;

	if(minn > 0)
		store[i] = slots[i] = returnRandom(1, minn);		// allocate slots and also store it in a temporary 'store' array
	else
	{
		// printf("ZEEEEEEEEEEEEEEEr\n");
	}
	
	printf("Vaccination Zone %d is ready to vaccinate with %d slots\n", i, slots[i]);
	pthread_mutex_unlock(&zone_lock[i]);
}

int allStudentsDone()
{
	return (doneCase >= o) ? (1) : (0);		// determines if all students have completed the vaccination procedure either successfully or not
}

void getSlots(int curr_zone)
{
	//given zone checks if atleast one student is waiting, otherwise terminate

	int inQueue;

	while((inQueue = waiting_studs) <= 0)		//given zone waits until it finds atleast one waiting student willing to take up a slot
	{
		if(allStudentsDone())
			return;				// returns if all students finished procedure
	}

	updateSlots(curr_zone, inQueue);		//zone updates slots if it sees atleast one waiting student
}

void* stud_dispatch(void *stud)
{
	// student threads reach here

	sleep(returnRandom(1, 4));		// student arrives at random time
	struct arg* studs = (struct arg*) stud;
	int studd = studs->id; 		// student number

	while(attempt_count[studd] <= 3)
	{
		attempt_count[studd]++;		// current attempt number of current student is incremented

		if(attempt_count[studs->id] == 4)
		{
			succ[studd] = 0;
			pthread_mutex_lock(&lock_for_all);
			doneCase++;			// student received three failed vaccinations consecutively, hence sent home
			pthread_mutex_unlock(&lock_for_all);
			printf("College has given up on Student %d\n", studs->id);
			return NULL;
		}

		printf("Student %d has arrived for his round %d of vaccination\n", studd, attempt_count[studd]);

		printf("Student %d is waiting to be allocated a slot on a Vaccination Zone\n", studd);
		pthread_mutex_lock(&lock_for_all);
		waiting_studs++;		// currently waiting students
		pthread_mutex_unlock(&lock_for_all);
		int askZone = -1, curr_zone = 1;

		while(!allStudentsDone())
		{
			if(allStudentsDone())
				return NULL;
				
			pthread_mutex_lock(&zone_lock[curr_zone]);

			if(!inVaccPhase[curr_zone] && slots[curr_zone] > 0)  		// waiting for a zone to allocate a slot
			{
				askZone = curr_zone;		// zone assigned a slot to this student
				printf("Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n", studd, askZone);
				pthread_mutex_lock(&lock_for_all);
				waiting_studs--;		// number of waiting students decreases by 1
				pthread_mutex_unlock(&lock_for_all);
				slots[curr_zone]--;
				pthread_mutex_unlock(&zone_lock[curr_zone]);

				while(!inVaccPhase[askZone])
				{
						// waiting for that allocated zone to enter vaccination phase
				}

				sleep(1);		// vaccination going on

				printf("Student %d on Vaccination Zone %d has been vaccinated which has success probability %Lf\n", 
						studd, askZone, probs[zone_curr_company[askZone]]);

				pthread_mutex_lock(&zone_lock[askZone]);
				zone_vaccines[askZone]--;		// updating vaccines for the zone
				alloc_slots[askZone]--;		
				pthread_mutex_unlock(&zone_lock[askZone]);
				
				sleep(1);		// student getting tested if the vaccination was a success

				if(vaccSuccessful(zone_curr_company[curr_zone]) == 1)	// vaccinating student and checking its success status
				{
					succ[studd] = 1;	
					pthread_mutex_lock(&lock_for_all);
					doneCase++;			// student sent to college as vaccination was successful
					pthread_mutex_unlock(&lock_for_all);
					printf("Student %d has tested positive for antibodies.\n", studd);
					return NULL;
				}

				// vaccination didn't work in this attempt for this student
				printf("Student %d has tested negative for antibodies.\n", studd);
				break;
			}

			pthread_mutex_unlock(&zone_lock[curr_zone]);
			curr_zone = (curr_zone % m) + 1;	// iterating through zones
		}

		if(allStudentsDone())
			return NULL;
	}

	return NULL;
}

void* company_dispatch(void *comp)
{
	// company threads reach here

	struct arg* c = (struct arg*) comp;
	int curr_comp = c->id;		// current company number

	while(!allStudentsDone())
	{
		produceVaccines(curr_comp);		// vaccine produnction resumes as company doesn't have any batches 

		while(curr_batch[curr_comp] > 0)	// wait for all batches to be used by various zones
		{
			if(allStudentsDone())		// if Vaccination procedure for all students completed, terminate
				return NULL;
		}

		printf("All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n", curr_comp);
	}

	return NULL;
}

void* zone_dispatch(void *zone)
{
	// zone threads reach here

	struct arg* z = (struct arg*) zone;
	int curr_zone = z->id;	// current zone number
	int call_this = 0, comp_num = 1;	// stores the company number the zone can choose
	int refilledNow = 0;		// describes if this zone refilled in a given iteration

	while(!allStudentsDone())
	{	
		refilledNow = 0;
		pthread_mutex_lock(&zone_lock[curr_zone]);
		inVaccPhase[curr_zone] = 0;		// zone is not in vaccination phase yet
		slots[curr_zone] = 0;			// zone didn't allocate any slots yet
		
		if(zone_vaccines[curr_zone] == 0 && !allStudentsDone())
		{
			 refilledNow = 1;
			 //zone doesn't have vaccines, waits for a company to deliver; as vaccines are neede before slots are declared
			 pthread_mutex_unlock(&zone_lock[curr_zone]);
			 call_this = 0;
			 printf("Vaccination Zone %d doesn't have any vaccines\n", curr_zone);

			while(!allStudentsDone())
			{
				if(allStudentsDone())		// return if all students processed
					return NULL;

				pthread_mutex_lock(&comp_lock[comp_num]);

				if(curr_batch[comp_num] > 0)
				{
					call_this = comp_num;		// company has atleast one batch, take one batch
					pthread_mutex_unlock(&comp_lock[comp_num]);
					break;
				}

				pthread_mutex_unlock(&comp_lock[comp_num]);
				comp_num = (comp_num % n) + 1;	// wait till a company with atleast one batch is found
			}

			if(allStudentsDone())
				return NULL;

			printf("Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %Lf\n",
					call_this, curr_zone, probs[call_this]);

			pthread_mutex_lock(&zone_lock[curr_zone]);
			zone_curr_company[curr_zone] = call_this;		// zone is accessing the vaccines provided by company 'call_this'
			zone_vaccines[curr_zone] = vacc_in_batch[call_this];		// updating vaccine count of zone, after company provides it
			pthread_mutex_unlock(&zone_lock[curr_zone]);
			sleep(1);		// company delivering to zone

			printf("Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n",
					call_this, curr_zone);

		}

		if(refilledNow == 0)
			pthread_mutex_unlock(&zone_lock[curr_zone]);

		getSlots(curr_zone);	// declare slots based on current number of vaccines
		if(allStudentsDone())	
			return NULL;		// return if all students processed
		pthread_mutex_lock(&zone_lock[curr_zone]);
		inVaccPhase[curr_zone] = 0;		// slots not allocated to students yet
		pthread_mutex_unlock(&zone_lock[curr_zone]);
		

		while(waiting_studs > 0 && slots[curr_zone] > 0)
		{
			// zone is waiting to allocate slots for students currently waiting, provided it has enough slots
			if(allStudentsDone())		// return if all students have processed
				return NULL;
		}

		pthread_mutex_lock(&zone_lock[curr_zone]);
		alloc_slots[curr_zone] = store[curr_zone] - slots[curr_zone];	// count of number of students who turned up at this zone and got a slot allocated here.
		printf("Vaccination Zone %d entering Vaccination Phase\n", curr_zone);
		inVaccPhase[curr_zone] = 1;				// zone entering vaccination phase now
		pthread_mutex_unlock(&zone_lock[curr_zone]);

		if(allStudentsDone())
			return NULL;

		while(alloc_slots[curr_zone] > 0)
		{
			// waiting till all students allocated a slot here are vaccinated

			if(allStudentsDone())		// return if all students have processed
				return NULL;
		}

		pthread_mutex_lock(&zone_lock[curr_zone]);
        slots[curr_zone] = 0;		// slots for this phase have been processed
		inVaccPhase[curr_zone] = 0;		// exitting from vaccination phase
		pthread_mutex_unlock(&zone_lock[curr_zone]);


		if(refilledNow == 1)
		{
			pthread_mutex_lock(&comp_lock[call_this]);
			curr_batch[call_this]--;		// update batch count for company
			pthread_mutex_unlock(&comp_lock[call_this]);
		}
		
		// repeat the same procedure until all students are processed
	}

	return NULL;
}


int main()
{
	srand(time(0));
	scanf("%d%d%d", &n, &m, &o);

	if(n < 0 || m < 0 || o < 0)
	{
		printf("Oops! Simulation is not possible :( \n");		// Invalid values
		return 0;
	}

	if(n == 0 || m == 0 || o == 0)
	{
		if(o == 0)		// there are no students
		{
			printf("Oops! No student could get into IIIT-H this time\n");
		}

		if(n == 0)		// there are no companies
		{
			printf("There are no companies to manufacture vaccines, have fun with the online sem :')\n");
		}

		if(m == 0)		// there are no vaccination zones
		{
			printf("No zones to distribute vaccines, volunteers?? \n");
		}

		return 0;
	}

	a = malloc((o+1)*sizeof(struct arg));
	zz = malloc((m+1)*sizeof(struct arg));
	b = malloc((n+1)*sizeof(struct arg));
	

	pthread_t company[n+1];
	pthread_t vaccine_zone[m+1];
	pthread_t student[o+1];			// all company, zone, student threads

	stud_lock = (pthread_mutex_t *)malloc((o+1)*sizeof(pthread_mutex_t));
	zone_lock = (pthread_mutex_t *)malloc((m+1)*sizeof(pthread_mutex_t));
	comp_lock = (pthread_mutex_t *)malloc((n+1)*sizeof(pthread_mutex_t));	// locks for all three entity types

	probs = (long double *) malloc((n+1)*sizeof(long double));
	curr_batch = (int *) malloc((n+1)*sizeof(int));
	vacc_in_batch = (int *) malloc((n+1)*sizeof(int));
	slots = (int *) malloc((m+1)*sizeof(int));
	store = (int *) malloc((m+1)*sizeof(int));
	inVaccPhase = (int *) malloc((m+1)*sizeof(int));
	alloc_slots = (int *) malloc((m+1)*sizeof(int));
	zone_vaccines = (int *) malloc((m+1)*sizeof(int));
	zone_curr_company = (int *) malloc((m+1)*sizeof(int));
	attempt_count = (int *) malloc((o+1)*sizeof(int));
	succ = (int *) malloc((o+1)*sizeof(int));

	// probabilities of success of vaccine delivered by each company
	for(int i=1;i<=n;i++)
		scanf("%Lf", &probs[i]);

	// initializing batch count, vaccines in a batch, lock for every company

	for(int i=1;i<=n;i++)
	{    
		pthread_mutex_init(&comp_lock[i], NULL);
		curr_batch[i] = 0;
		vacc_in_batch[i] = 0;
	}

	// initializing slots declared, slots allocated, current company providing vaccines, number of vaccines, vaccination phase or not, locks etc. for every zone

	for(int i=1;i<=m;i++)
	{
		pthread_mutex_init(&zone_lock[i], NULL);
		slots[i] = 0;
		store[i] = 0;
		alloc_slots[i] = 0;
		zone_vaccines[i] = 0;
		inVaccPhase[i] = 0;
		zone_curr_company[i] = 0;
	}

	// initializing attempt count, vaccination success status, lock for every student
	for(int i=1;i<=o;i++)
	{
		pthread_mutex_init(&stud_lock[i], NULL);
		attempt_count[i] = 0;
		succ[i] = -1;
	}

	pthread_mutex_init(&lock_for_all, NULL);

	// dispatching student, zone, company threads

	for(int i=1;i<=o;i++)
	{
		a[i].id = i;
		pthread_create(&student[i], NULL, stud_dispatch, &a[i]);
	}

	for(int i=1;i<=m;i++)
	{
		zz[i].id = i;
		pthread_create(&vaccine_zone[i], NULL, zone_dispatch, &zz[i]);
	}

	for(int i=1;i<=n;i++)
	{
		b[i].id = i;
		pthread_create(&company[i], NULL, company_dispatch, &b[i]);
	}

	// waiting for all students to be processed
	while(!allStudentsDone());

	// joining all threads and destroying all locks
	for(int i=1;i<=n;i++)
	{
		pthread_join(company[i], NULL);
	}

	for(int i=1;i<=m;i++)
	{
		pthread_join(vaccine_zone[i], NULL);
	}

	for(int i=1;i<=o;i++)
	{
		pthread_join(student[i], NULL);
	}

	pthread_mutex_destroy(&lock_for_all);

	for(int i=1;i<=n;i++)
		pthread_mutex_destroy(&comp_lock[i]);

	for(int i=1;i<=m;i++)
		pthread_mutex_destroy(&zone_lock[i]);

	for(int i=1;i<=o;i++)
		pthread_mutex_destroy(&stud_lock[i]);

	printf("Simulation Over :) \n");
	return 0;
}
