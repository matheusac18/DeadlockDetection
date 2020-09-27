#define _GNU_SOURCE
#define MAX 100
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

enum Type{PROCESS, RESOURCE};

int (*_sem_wait)(sem_t *) = NULL;
int (*_sem_post)(sem_t *) = NULL;

typedef struct sourceType
{
    long int id;
    enum Type type;//Process or resource(0-1)
}SourceType;

typedef struct vertex{
    SourceType s;
    struct vertex *next;
}Vertex;

typedef struct taskGraph
{
    Vertex list[MAX];
    int NVertex;
}TaskGraph;

TaskGraph *TG = NULL;
int path[MAX+1];
int visited[MAX];
int k = 0;
int deadlock = 0;

Vertex *create_vertex(SourceType newSourceType)
{
    Vertex *newVertex = (Vertex*)malloc(sizeof(Vertex));
    newVertex->s = newSourceType;
    newVertex->next = NULL;
    return newVertex;
}

int search_vertex(SourceType v)
{
    for(int i = 0; i < TG->NVertex; i++)
    {
        
        if(TG->list[i].s.type == v.type && TG->list[i].s.id == v.id)
        {
            
            return i;
        }
    }
    return -1;
}

void add_vertex(SourceType newVertex)
{
    if(search_vertex(newVertex) == -1){
        TG->list[TG->NVertex].s = newVertex;
        TG->list[TG->NVertex].next = NULL;
        TG->NVertex++;
    }
}

int add_edge(SourceType i, SourceType j)
{
    add_vertex(i);
    add_vertex(j);

    Vertex *v = &(TG->list[search_vertex(i)]);
    
    while(v->next != NULL)
    {
        v = v->next;
    }

    v->next = create_vertex(j);
}

int verify_edge(SourceType i, SourceType j)
{
    if(TG->NVertex == 0)
        return 0;

    int i_index = search_vertex(i);

    if(i_index == -1)
        return 0; 

    Vertex *v = &(TG->list[i_index]);
    
    while(v != NULL)
    {
        if(v->s.id == j.id)
            return 1;

        v = v->next;
    }
    return 0;
}

int remove_edge(SourceType i, SourceType j)
{
    int indexI = search_vertex(i);
    int indexJ = search_vertex(j);
    if(indexI != -1 && indexJ != -1)
    {
        Vertex *v = &(TG->list[indexI]);
        Vertex *remove;
        while(v->next != NULL)
        {
            if(v->next->s.id == j.id)
            {   
                remove = v->next;
                v->next = v->next->next;
                free(remove);
                break;
            }
            v = v->next;   
        }
    }
}

void print_adj_list()
{
    for(int i = 0; i < TG->NVertex; i++)
    {
        Vertex *v = &(TG->list[i]);
        while(v != NULL)
        {
            printf("[ID: %ld Type: %d]->",v->s.id,v->s.type);
            v = v->next;
        }
        printf("\n");
    }
}

void inform_deadlock()
{
    printf("\nEncontrou deadlock! Ciclo: Processo: %ld\n",pthread_self());
    deadlock = 1;

    for(int i=0; i < 10000000; i++)
    {
        printf("DEADLOCK!!!");
    }

    for(int i=0; i < k-1; i++)
    {
        printf("%ld->",TG->list[path[i]].s.id);
    }
    printf("%ld",TG->list[path[k-1]].s.id);

}

void DFS(int i)
{
    if(deadlock == 1)
        return;

    Vertex *v = &(TG->list[i]);
    if(visited[i] == 1)
    {
        path[k++] = i;
        inform_deadlock();
    }

    visited[i] = 1; //discovered
    path[k++] = i;
    while(v->next !=NULL)
    {
        DFS(search_vertex(v->next->s));
        v = v->next;
        
    }
}

int search_for_cycle(int testIndex)
{
    k = 0;
    for(int i = 0; i < TG->NVertex;i++)
    {
        visited[i] = 0;//not visited
    }
    for(int i = 0; i <= MAX;i++)
    {
        path[i] = -1 ;
    }


    DFS(testIndex);
}




void create_graph()
{
    if(TG==NULL)
    {
        TG = (TaskGraph*)malloc(sizeof(TaskGraph));
        TG->NVertex = 0;
    }
}

int sem_wait(sem_t *sem)
{
    
    int r = -1;
    if(!_sem_wait)
    {
        _sem_wait = dlsym(RTLD_NEXT, "sem_wait");//irá apontar para o sem_wait original
    }

    create_graph();
    deadlock = 0;
    SourceType process;
    process.type = PROCESS;
    process.id = pthread_self();

    SourceType resource;
    resource.type = RESOURCE;
    resource.id = (long int)sem;

    //add edge from process to resource(process is requesting resource)
    if(verify_edge(process,resource) == 0)
    {
        add_edge(process,resource);
        //verify if there's a cycle(ie deadlock) in graph
        search_for_cycle(search_vertex(process));
    }

    
    if(deadlock == 1)//found deadlock
    {
        
        remove_edge(process,resource);
        
        return -2;
    }
    else
    {
       
        int sval = 0;
        sem_getvalue(sem,&sval);
        if(sval>0)
        {
            remove_edge(process,resource);
            add_edge(resource,process);
            r = _sem_wait(sem);
        }
    }
    
    return(r);
}

int sem_post(sem_t *sem)
{
    int r = 0;
    if(!_sem_post)
    {
        _sem_post = dlsym(RTLD_NEXT, "sem_post");//irá apontar para o sem_post original
    }

    SourceType process;
    process.type = PROCESS;
    process.id = pthread_self();

    SourceType resource;
    resource.type = RESOURCE;
    resource.id = (long int)sem;
    
    remove_edge(resource,process);

    r = _sem_post(sem);
    return(r);
}

/*int main(){
    TG = (TaskGraph*)malloc(sizeof(TaskGraph));
    TG->NVertex = 0;

    SourceType newVertex;
    newVertex.id = 1;
    newVertex.type = PROCESS;
    add_vertex(newVertex);

    SourceType newResource;
    newResource.id = 2;
    newResource.type = RESOURCE;
    add_vertex(newResource);

    SourceType newVertex1;
    newVertex1.id = 3;
    newVertex1.type = PROCESS;
    add_vertex(newVertex1);
    
    SourceType newResource1;
    newResource1.id = 4;
    newResource1.type = RESOURCE;
    add_vertex(newResource1);

    add_edge(newResource, newVertex);
    add_edge(newResource1, newVertex1);
    add_edge(newVertex, newResource1);
    add_edge(newVertex1,newResource);

    print_adj_list();

    search_for_cycle(search_vertex(newVertex1));

    return 0;
}*/