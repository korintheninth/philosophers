#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

typedef struct philo
{
	long long int lasteat;
	int num;
	int eat;
	int sleep;
	int think;
	int *running;

	pthread_t philosopher;
	pthread_mutex_t *leftFork;
	pthread_mutex_t *rightFork;
	pthread_mutex_t eatLock;
	pthread_mutex_t *writeLock;
} t_philo;

typedef struct warden
{
	t_philo **philos;
	int deathTime;
	int *running;
	pthread_t warden;
	pthread_mutex_t *writeLock;
} t_warden;

void writeinf(char *str, int num, pthread_mutex_t *writelock)
{
	struct timeval currentTime;
	long long int curinms;
	
	gettimeofday(&currentTime, NULL);
	curinms = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
	pthread_mutex_lock(writelock);
	printf(str, num, curinms);
	pthread_mutex_unlock(writelock);
}

void *philosopher(void *arg)
{
	t_philo *self;

	self = (t_philo*)arg;
	struct timeval currentTime;
	long long int curinms;
	
	while (*self->running)
	{
		pthread_mutex_lock(self->rightFork);
		writeinf("philosopher %d grabbed a fork. Timestamp: %lld\n", self->num, self->writeLock);
		pthread_mutex_lock(self->leftFork);
		writeinf("philosopher %d grabbed a fork. Timestamp: %lld\n", self->num, self->writeLock);
		writeinf("philosopher %d eating. Timestamp: %lld\n", self->num, self->writeLock);
		pthread_mutex_lock(&self->eatLock);
		gettimeofday(&currentTime, NULL);
		curinms = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
		self->lasteat = curinms;
		pthread_mutex_unlock(&self->eatLock);
		usleep(self->eat);
		pthread_mutex_unlock(self->rightFork);
		pthread_mutex_unlock(self->leftFork);
		writeinf("philosopher %d sleeping. Timestamp: %lld\n", self->num,  self->writeLock);
		usleep(self->sleep);
		writeinf("philosopher %d thinking. Timestamp: %lld\n", self->num, self->writeLock);
		usleep(self->think);
	}
	return (NULL);
}

void *wardenfn(void *arg)
{
	t_warden *warden;
	int running;
	int i;
	struct timeval currentTime;
	long long int curinms;

	warden = (t_warden*)arg;
	while (*warden->running)
	{
		i = 0;
		while (warden->philos[i] != NULL && *warden->running)
		{
			gettimeofday(&currentTime, NULL);
			curinms = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
			pthread_mutex_lock(&warden->philos[i]->eatLock);
			if (curinms - warden->philos[i]->lasteat > warden->deathTime)
			{
				writeinf("Philosopher %d died. Timestamp: %lld\n", i, warden->writeLock);
				warden->running = 0;
			}
			pthread_mutex_unlock(&warden->philos[i]->eatLock);
			i++;
		}
	}
	return (NULL);
}

int main(void)
{
	const int numberofp = 5;
	int eat = 2000000;
	int sleep = 5000000;
	int think = 5000000;
	int deathTime = 1200;
	int running = 1;
	struct timeval currentTime;
	long long int curinms;
	pthread_mutex_t writelock;
	
	t_philo **philos = malloc(sizeof(t_philo*) * (numberofp + 1));
	pthread_mutex_t *forks = malloc(sizeof(pthread_mutex_t) * numberofp);

	pthread_mutex_init(&writelock, NULL);
	philos[numberofp] = NULL;
	int i = 0;
	while (i < numberofp)
	{
		pthread_mutex_init(&forks[i], NULL);
		i++;
	}
	i = 0;
	while (i < numberofp)
	{
		philos[i] = malloc(sizeof(t_philo));
		philos[i]->rightFork = &forks[i];
		philos[i]->leftFork = &forks[(i + 1) % numberofp];
		pthread_mutex_init(&philos[i]->eatLock, NULL);
		philos[i]->num = i;
		gettimeofday(&currentTime, NULL);
		curinms = currentTime.tv_sec * 1000 + currentTime.tv_usec/1000;
		philos[i]->lasteat = curinms;
		philos[i]->eat = eat;
		philos[i]->sleep = sleep;
		philos[i]->think = think;
		philos[i]->running = &running;
		philos[i]->writeLock = &writelock;
		i++;
	}
	i = 0;
	while (i < numberofp)
	{
		pthread_create(&philos[i]->philosopher, NULL, philosopher, (void*)philos[i]);
		usleep(1);
		i++;
	}
	t_warden warden;

	warden.deathTime = deathTime;
	warden.philos = philos;
	warden.running = &running;
	warden.writeLock = &writelock;
	pthread_create(&warden.warden, NULL, wardenfn, (void*)&warden);
	pthread_join(warden.warden, NULL);
	i = 0;
	while (i < numberofp)
	{
		pthread_join(philos[i]->philosopher, NULL);
		free(philos[i]);
		i++;
	}
	i = 0;
	while (i < numberofp)
	{
		free(philos[i]);
		i++;
	}
	free(forks);
	free(philos);
}