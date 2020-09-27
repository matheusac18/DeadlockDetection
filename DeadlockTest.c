#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>

//recursos
long int r1 = 0;
long int r2 = 0;

// semaforos 
sem_t s1;
sem_t s2;

void *p1(void *args)
{
    while(1)
    {
        if(sem_wait(&s2)==-2)
            printf("DEADLOCK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        if(sem_wait(&s1)==-2)
            printf("DEADLOCK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        printf("P1: em r1: %ld \n",++r1);
        printf("P1: em r2: %ld \n",++r2);
        sem_post(&s1);
        sem_post(&s2);
    }
}

void *p2(void *args)
{
    while(1)
    {
        sem_wait(&s1);
        sem_wait(&s2);
        printf("P2: em r1: %ld \n",++r1);
        printf("P2: em r2: %ld \n",++r2);
        sem_post(&s1);
        sem_post(&s2);
    }
}

int main(int argc, char **argv)
{
    pthread_t t1, t2;
    sem_init(&s1,0,1);
    sem_init(&s2,0,1);

    pthread_create(&t1, NULL,p1,NULL);
    pthread_create(&t2, NULL,p2,NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    return 0;
}


