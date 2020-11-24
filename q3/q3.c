#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<semaphore.h>
#include<string.h>
#include<unistd.h>


#define ll long long 

pthread_t performer1[400+5];
pthread_t performer2[400+5];
pthread_t performer3[400+5];
pthread_t sparethreads[405];
pthread_t extrathread[405];


pthread_mutex_t performerlock[405];
pthread_mutex_t stagelock[405];


typedef struct node{
    int id;
}performerinfo;

performerinfo* performerinput1[400+5];
performerinfo* performerinput2[400+5];
performerinfo* performerinput3[400+5];
performerinfo* spareinput[400+5];

sem_t semacoustic;
sem_t semelectric;
sem_t common;
sem_t semcoordinate;

int k,a,e,c,t1,t2,t;
char name[405][100];
char instrument[405];
int arrival[405]={0}; // stored the time at which it arrived.
int didheperform[405][3]={0};
int isstagefree[405]={0};
int performeronstage[405]={0};
int istheresinger[405]={0};


ll randomnumber(ll a,ll b)
{
    return a+rand()%(b-a+1); // generates a random number between a and b
}

void* coordinator(void* inp)
{
    performerinfo* input = (performerinfo*)inp;
    int num=input->id;
    // printf("\033[1;35m");
    // printf("%s waiting for collecting T-shirt\n",name[num]);
    // printf("\033[0m");
    sem_wait(&semcoordinate);
    printf("\033[0;35m%s is in the process of collecting T-shirt\n\033[0m",name[num]);
    sleep(2);
    printf("\033[1;35m%s collected T-shirt\n\033[0m",name[num]);
    sem_post(&semcoordinate);
    return NULL;
}
void* electric(void* inp)
{
    performerinfo* input = (performerinfo*)inp;
    int num = input->id;  
    int sinind =-1; // represents any performer joined our current performer if yes then it represents the index of that performer. 
    pthread_mutex_lock(&performerlock[num]);
    if(instrument[num]=='v') 
    {
        didheperform[num][1]=-1; // This variable represnts that performer is certainly not allowed to perform on electric if it is -1.
        if(didheperform[num][0]+didheperform[num][2]==-2)
        {
            printf("\033[1m%s %c left because of impatience\n\033[0m",name[num],instrument[num]);
            pthread_mutex_unlock(&performerlock[num]);
            return NULL;         
        }
        pthread_mutex_unlock(&performerlock[num]);
        return NULL;
    }
    pthread_mutex_unlock(&performerlock[num]);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // get the structure which has absolute time.
    ts.tv_sec += t; // we incremented the ts.tv_sec by t so that we can know the time after t sec.
    if(sem_timedwait(&semelectric,&ts)==-1)
    {
        pthread_mutex_lock(&performerlock[num]);
        didheperform[num][1]=-1; 
        if(didheperform[num][0]+didheperform[num][2]==-2)
        {
            printf("\033[1m%s %c left because of impatience\n\033[0m",name[num],instrument[num]);
            pthread_mutex_unlock(&performerlock[num]);
            return NULL;
        }
        pthread_mutex_unlock(&performerlock[num]);
        return NULL;
    }
    else
    {
       pthread_mutex_lock(&performerlock[num]);
       didheperform[num][1]=1; // set the value such that this thread sucessfully got its slice
       if(didheperform[num][0]==1||didheperform[num][2]==1) // checking if this is the first to get the slice.
       {
           pthread_mutex_unlock(&performerlock[num]);
           sem_post(&semelectric);
           return NULL;
       }
       pthread_mutex_unlock(&performerlock[num]);
       
       int flfl=-1,sta;
       for(int i=a+1;i<=a+e;i++)
       {
           pthread_mutex_lock(&stagelock[i]);
           if(isstagefree[i]==0) // checking for an empty electric stage.
           {
               if(instrument[num]=='s')
               isstagefree[i]=1; // Made the empty one filled by singer or musician according to the threads instrumnet.
               else isstagefree[i]=2;
                flfl=1;
           }
           pthread_mutex_unlock(&stagelock[i]);
           if(flfl==1)
           {
               sta=i;
               pthread_mutex_lock(&stagelock[i]);
               performeronstage[i]=num; // Performer on a stage is set to the currently exectuting performer id.
               pthread_mutex_unlock(&stagelock[i]);
               break;
           }
       }
       pthread_mutex_lock(&performerlock[num]);
       istheresinger[num]=0; // Initiliaed that there is no singer along with current performer.
       pthread_mutex_unlock(&performerlock[num]);

       if(instrument[num]!='s')
       sem_post(&common); // if performer is not singer then we increase the common semaphore so that a singer can potentially claim this spot. 

       ll var=randomnumber(t1,t2);
       printf("\033[1;33m%s performing %c at electric stage numbered %d for %lld sec\n\033[0m",name[num],instrument[num], sta,var);
       sleep(var); // random amount of sleep (time taken to complete performance) between t1 and t2.
       if(instrument[num]!='s')
       {
           int flag=-1;
           if(isstagefree[sta]==3) // checking if there is anyone who joined the current stage along with current performer.
           {
               sleep(2); // In that case we will sleep for 2 sec.
               flag=1;
           }
           else if(sem_trywait(&common)==-1) // This will fail when there are singers who took all increments made by performers that implies that since even this musician incremented common semephore there thould be atleast one singer who is about to take this stage or join this stage as well so we assume that he will join shortly and make the extension of performance by 2 sec.
           {
               sleep(2);
               flag=1;
           }
        }
        pthread_mutex_lock(&stagelock[sta]);
        isstagefree[sta]=0; // Once the performence is done we make the stage free once again.
        performeronstage[sta]=0; // And we set the performer status to be nobody is performing is currently performing.
        pthread_mutex_unlock(&stagelock[sta]);
        printf("\033[1;33m%s performance at electric stage ended\n\033[0m",name[num]);
        sinind = istheresinger[num]; // This will give -1 if there is no singer + musican combinaqtion on stage else it would give the singer id who joined.
        sem_post(&semelectric);
    }
    
    pthread_create(&sparethreads[num],NULL,coordinator,(void*)spareinput[num]);
    if(sinind>0)
    {
        pthread_create(&sparethreads[sinind],NULL,coordinator,(void*)spareinput[sinind]);
        pthread_join(sparethreads[sinind],NULL);
    }
    pthread_join(sparethreads[num],NULL);    
    return NULL;
}

void* acoustic(void* inp)
{
    performerinfo* input = (performerinfo*)inp;
    int num = input->id;
    int sinind =-1;
    // printf("%s enterned\n",name[num]);
    pthread_mutex_lock(&performerlock[num]);
    if(instrument[num]=='b')
    {
        didheperform[num][0]=-1; // this is saying that whosoever the perfoemer maybe his attempt to perform on acoustic stage failed. 
        if(didheperform[num][1]+didheperform[num][2]==-2) // If the other two already failed then the performer can leave the college with no further delay.
        {
            printf("\033[1m%s %c left because of impatience\n\033[0m",name[num],instrument[num]);
            pthread_mutex_unlock(&performerlock[num]);
            return NULL;         
        }
        pthread_mutex_unlock(&performerlock[num]);
        return NULL;
    }
    pthread_mutex_unlock(&performerlock[num]);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += t; // As described earlier this will give me the time untill which the performer can try to aquire the stage after which he will leave with impatience.
    if(sem_timedwait(&semacoustic,&ts)==-1) // This will return -1 when the time slice expired.
    {
        pthread_mutex_lock(&performerlock[num]);
        didheperform[num][0]=-1; // As earlier this meant that he failed to auire stage within time.
        if(didheperform[num][1]+didheperform[num][2]==-2) // if this the last to fail then the performer can leave the college without performing.
        {
            printf("\033[1m%s %c left because of impatience\n\033[0m",name[num],instrument[num]);
            pthread_mutex_unlock(&performerlock[num]);
            return NULL;
        }
        pthread_mutex_unlock(&performerlock[num]);
        return NULL;
    }
    else
    {
       pthread_mutex_lock(&performerlock[num]);
       didheperform[num][0]=1; // Incase he gets the stage within the time then he will be given the stage.
       if(didheperform[num][1]==1||didheperform[num][2]==1) // he should check if he already is sucessful in getting other stages inthat case since he already got those he can no longer required to get theis stage hence he will leave this stage immediately.
       {
           pthread_mutex_unlock(&performerlock[num]);
           sem_post(&semacoustic);
           return NULL;
       }
       pthread_mutex_unlock(&performerlock[num]);
       
       int flfl=-1,sta;
       for(int i=1;i<=a;i++)
       {
           // iterating through all the acoustic stages available to see if anyoine is free.
           pthread_mutex_lock(&stagelock[i]);
           if(isstagefree[i]==0)
           {
               if(instrument[num]=='s')
               isstagefree[i]=1;// depending on whether he is singer or not we  will update the persons who are currently performing on this stage.
               else isstagefree[i]=2;
                flfl=1;
           }
           pthread_mutex_unlock(&stagelock[i]);
           if(flfl==1)
           {
               sta=i; // storing the stage number.
             pthread_mutex_lock(&stagelock[i]);
               performeronstage[i]=num; // Storing the performer index at this stage number. 
               pthread_mutex_unlock(&stagelock[i]);
               break;
           }
       }
       pthread_mutex_lock(&performerlock[num]);
       istheresinger[num]=0; // Making sure that we initialize that there is no additional singer along with our current performer.
       pthread_mutex_unlock(&performerlock[num]);

       if(instrument[num]!='s')
       sem_post(&common);

       ll var=randomnumber(t1,t2);
       printf("\033[1;34m%s performing %c at acoustic stage numbered %d for %lld sec\n\033[0m",name[num],instrument[num], sta,var);
       sleep(var);
       if(instrument[num]!='s')
       {
           int flag=-1;
           if(isstagefree[sta]==3)
           {
               sleep(2);
               flag=1;
           }
           else if(sem_trywait(&common)==-1)
           {
               sleep(2);
               flag=1;
           }
        }
        pthread_mutex_lock(&stagelock[sta]);
        isstagefree[sta]=0; // Making this stage free for others to occupy.
        performeronstage[sta]=0;// No one is there on the current stage.
        pthread_mutex_unlock(&stagelock[sta]);
        printf("\033[1;34m%s performance at acoustic stage ended\n\033[0m",name[num]);
        sinind = istheresinger[num]; // storing the index of the singer who joined the performer if any else it will be -1.
        sem_post(&semacoustic);
    }
    
    pthread_create(&sparethreads[num],NULL,coordinator,(void*)spareinput[num]);
    if(sinind>0) // If there is a singer additionally joining  in the middle then even he must go and collect T-shirt.
    {
        pthread_create(&sparethreads[sinind],NULL,coordinator,(void*)spareinput[sinind]);
        pthread_join(sparethreads[sinind],NULL);
    }
    pthread_join(sparethreads[num],NULL);    
    return NULL;
}

void* both(void* inp)
{
    performerinfo* input = (performerinfo*)inp;
    int num = input->id;
    pthread_mutex_lock(&performerlock[num]);
    if(instrument[num]!='s')
    {
        // since only singer can join a musican in middle rest all are made as unsucessful.
        didheperform[num][2]=-1; 
        if(didheperform[num][0]+didheperform[num][1]==-2)// if rest all are already unsucessfull then straight away say that this performer has no option other than to leave.
        {
            printf("\033[1m%s %c left because of impatience\n\033[0m",name[num],instrument[num]);
            pthread_mutex_unlock(&performerlock[num]);
            return NULL;         
        }
        pthread_mutex_unlock(&performerlock[num]);
        return NULL;
    }
    pthread_mutex_unlock(&performerlock[num]);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += t;
    if(sem_timedwait(&common,&ts)==-1)
    {
        // This will be executed only when the time slice expired.
        pthread_mutex_lock(&performerlock[num]);
        didheperform[num][2]=-1;
        if(didheperform[num][0]+didheperform[num][1]==-2)
        {
            printf("\033[1m%s %c left because of impatience\n\033[0m",name[num],instrument[num]);
            pthread_mutex_unlock(&performerlock[num]);
            return NULL;
        }
        pthread_mutex_unlock(&performerlock[num]);
        return NULL;
    }
    else
    {
       pthread_mutex_lock(&performerlock[num]);
       didheperform[num][2]=1;
       if(didheperform[num][0]==1||didheperform[num][1]==1)
       {
           pthread_mutex_unlock(&performerlock[num]);
           sem_post(&common);
           return NULL;
       }
       pthread_mutex_unlock(&performerlock[num]);
       
       int flfl=-1,sta;
       for(int i=1;i<=a+e;i++)
       {
           pthread_mutex_lock(&stagelock[i]);
           if(isstagefree[i]==2)// if there is some performer other than singer who is currently executing.
           {
               isstagefree[i]=3; // we will update it to singer jopined him in the middle.
               pthread_mutex_unlock(&stagelock[i]);
               istheresinger[performeronstage[i]]=num; // We will also make an updation that there is an additional singer who is singing.
               printf("\033[1;36m%s joined %s's performance, performence extended by 2sec\n\033[0m",name[num],name[i]);
               break;
           }
           pthread_mutex_unlock(&stagelock[i]);
       }
    }       
    return NULL;
}

void* fperform(void * inp)
{
    performerinfo* input = (performerinfo*)inp;
    int num = input->id;

    sleep(arrival[num]);
    printf("\033[1;32m%s %c arrived\n\033[0m",name[num],instrument[num]);
    pthread_create(&performer1[num],NULL,acoustic,(void*)performerinput1[num]);
    pthread_create(&performer2[num],NULL,electric,(void*)performerinput2[num]);
    pthread_create(&performer3[num],NULL,both,(void*)performerinput3[num]);
    pthread_join(performer1[num],NULL);
    pthread_join(performer2[num],NULL);
    pthread_join(performer3[num],NULL);
    return NULL;
}

int main()
{
    char ch;
    char* tem;
    tem = (char*)malloc(200*sizeof(char));
    scanf("%d %d %d %d %d %d %d",&k,&a,&e,&c,&t1,&t2,&t);
    if(c==0)
    {
        printf("Number of coordinators shouldnt be zero (Design choice) as it may lead to infinite loop\n");
        return 0;
    }
    sem_init(&semacoustic,0,a);
    sem_init(&semelectric,0,e);
    sem_init(&common,0,0);
    sem_init(&semcoordinate,0,c);

    for(int i=1;i<=k;i++)
    {
        pthread_mutex_init(&performerlock[i],NULL);
    }
    for(int i=1;i<=a+e;i++)
    {
        pthread_mutex_init(&stagelock[i],NULL);
    }
    for(int i=1;i<=k;i++)
    {
        fflush(stdin);
        scanf("%s ",name[i]);
        fflush(stdin);
        scanf("%c",&instrument[i]);
        scanf("%d",&arrival[i]);
        // printf("%s %c %d\n",name[i],instrument[i],arrival[i]);
        //fflush(stdin);
    }
    // for(int l=1;l<=k;l++)
    // {        
    //     size_t si=200;
    //     getline(&tem,&si,stdin);
    //     for(int i=0;i<strlen(tem);i++)
    //     {
    //         if(tem[i]==' ')
    //         {
    //             tem[i]='\0';
    //             strcpy(name[l],tem);
    //             tem[i]=' ';
    //             for(int j=i;j<strlen(tem);j++)
    //             {
    //                 if(tem[j]!=' ')
    //                 {
    //                     instrument[l]=tem[j];
    //                     ll nn=0;
    //                     for(int lp=j+1;lp<strlen(tem);lp++)
    //                     {
    //                         if(tem[lp]<='9'&&tem[lp]>='0')
    //                         {
    //                             nn=nn*10;
    //                             nn+=tem[lp]-'0';
    //                         }
    //                         else if(tem[lp]=='\n')break;
    //                     }
    //                     arrival[l]=nn;
    //                     break;
    //                 }
    //             }
    //             break;       
    //         }
    //     }
    // }
    for(int i=1;i<=k;i++)
    {
        performerinput1[i]=(performerinfo*)malloc(sizeof(performerinfo));
        performerinput1[i]->id=i;
        performerinput2[i]=(performerinfo*)malloc(sizeof(performerinfo));
        performerinput2[i]->id=i;
        performerinput3[i]=(performerinfo*)malloc(sizeof(performerinfo));
        performerinput3[i]->id=i;
        spareinput[i]=(performerinfo*)malloc(sizeof(performerinfo));
        spareinput[i]->id=i;        
    }
    for(int i=1;i<=k;i++)
    {
        pthread_create(&extrathread[i], NULL,fperform,(void*)performerinput1[i]);
    }

    for(int i=1;i<=k;i++)
    {
        // pthread_join(performer1[i],NULL);
        // pthread_join(performer2[i],NULL);
        // pthread_join(performer3[i],NULL);
        // pthread_join(sparethreads[i],NULL);
        pthread_join(extrathread[i],NULL);
    }
        
    sem_destroy(&semacoustic);
    sem_destroy(&semelectric);
    sem_destroy(&common);
    sem_destroy(&semcoordinate);

    for(int i=1;i<=k;i++)
    {
        pthread_mutex_destroy(&performerlock[i]);
    }
    for(int i=1;i<=a+e;i++)
    {
        pthread_mutex_destroy(&stagelock[i]);
    }

    printf("\033[1;31mFinished\n\033[0m");
    return 0;
}