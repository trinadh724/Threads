#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>


#define ll long long 

typedef struct node{
    int id;     // this is id of the company  
} companyinfo;

typedef struct nod{
    int id;     // id of the student
} studentinfo;

typedef struct nodes{
    int id;     // id of the vaccination.
} vacinationinfo;

ll numbercompany, numbervacination , numberstudent;
long double probability[1005],prob[1005];
int roundnumber[1005],ishecured[1005]={0},numberofstudentwaiting=0,slotsleft[1005]={0};
int peoplewhogotresult=0,vacines[1005]={0};
int peopleleft[1005]={0};
int batchesuntouched[1005]={0},vacinesinbatch[1005];
int batchescompleted[1005]={0};
int batchesproduced[1005]={0};
int inzone[1005]={0};

pthread_mutex_t lock;   // general lock used for incrementing and decrementing some variables.
pthread_mutex_t studentlock;
pthread_mutex_t vacinelock[1005];
pthread_mutex_t companylock[1005];


ll min(ll a,ll b)
{
    if(a>b)return b;
    else return a;
}

void takeinput()
{
    scanf("%Ld %Ld %Ld",&numbercompany,&numbervacination,&numberstudent); 
    for(int i=1;i<=numbercompany;i++)
    {
        scanf("%Lf",&probability[i]);
    }
}

ll randomnumber(ll a,ll b)
{
    return a+rand()%(b-a+1); // generates a random number between a and b
}

void* student(void * inp)
{
    sleep(randomnumber(1,3));
    studentinfo* input = (studentinfo*)inp;
    pthread_mutex_lock(&lock);
    numberofstudentwaiting++;   // This variable represents the number of students who did not get any result yet this variable is to keep track of number of students who should still be processes.
    pthread_mutex_unlock(&lock);
    int flag=-1; // remember that this is added to print the round statement in the code.   
    while(roundnumber[input->id]<=3&&ishecured[input->id]==0&&peoplewhogotresult<numberstudent)
    { // flag is set to -1 when there is a change in round number and is initialized to -1 so that first round will be printed.
        if(flag==-1)
        {
            printf("Student %d has arrived for his round %d of Vaccination\n",input->id,roundnumber[input->id]);
            printf("Student %d is waiting to be allocated a slot on a Vaccination Zone\n",input->id);
            flag=1;
        }
        for(int i=1;i<=numbervacination;i++)
        {
            pthread_mutex_lock(&vacinelock[i]);        
            if(slotsleft[i]>0) // Slotsleft in the vaccination zone i.
            {
                pthread_mutex_lock(&lock);
                numberofstudentwaiting--; // Since the slots are there numberof students ating are reduced since this student is assigned to a vaccine zone, and rest of the vaccine zones shouldnt confuse hence reducing the thread.
                pthread_mutex_unlock(&lock);
                peopleleft[i]++;    // This keeps track of the people in the current zone.
                slotsleft[i]--; // Decreased the slots left in the zone.
                vacines[i]--;   // After this student came vaccines left in  the zone should be reduced.
                flag=-1;        // this is used in two ways one to print next round if failed or to update the variables after this if condition.
                printf("Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n",input->id,i);
            }
            pthread_mutex_unlock(&vacinelock[i]);
            if(flag==-1)
            {
                roundnumber[input->id]++; // 
                while(slotsleft[i]>0&&numberofstudentwaiting>0&&peoplewhogotresult<numberstudent){} // To make sure that this thread is waiting untill all the required threads board in the vaccination zone.
                while(inzone[i]==0){}// when ith vaccination zone is being near completion of execution then inzone[i] will be turned to one so that the processes in the zone can be now print their messages..
                printf("Student %d on Vaccination Zone %d has been vaccinated which has success probability %Lf\n",input->id,i,prob[i]);
                ll var=100*(prob[i]); // scaling the current probability by 100
                // this is to chsck if the randomly generated number is less thancalculated thing or not.
                if(var>=randomnumber(0,100)) 
                {
                    printf("Student %d has tested positive for antibodies.\n",input->id);
                    ishecured[input->id]=1;
                    pthread_mutex_lock(&lock);
                    peoplewhogotresult++; // if any student tested positive then he is immediuately sent ot college. So the people who got result got incremented.
                    pthread_mutex_unlock(&lock);
                    pthread_mutex_lock(&vacinelock[i]);
                    peopleleft[i]--; // This varaible will take care as to all persons who entered the zone exited or not. This is incremented when somenbody enters and decrements when someone leaves.
                    pthread_mutex_unlock(&vacinelock[i]);
                    return NULL;
                }
                else
                {
                    printf("Student %d has tested negative for antibodies.\n",input->id);
                    pthread_mutex_lock(&lock);
                    numberofstudentwaiting++; // If the person tested negetive then number of threads waiting should be increased because 
                    pthread_mutex_unlock(&lock);
                    pthread_mutex_lock(&vacinelock[i]);
                    peopleleft[i]--; // This varaible will take care as to all persons who entered the zone exited or not. This is incremented when somenbody enters and decrements when someone leaves.
                    pthread_mutex_unlock(&vacinelock[i]);
                    break;
                }
            }
        }
    }
    printf("Student %d need to go back to his home for one more online semester\n",input->id);
    pthread_mutex_lock(&lock);
    peoplewhogotresult++; // if all the three rounds are done then the student is no lonhger given any chance so he need to get back to home.
    numberofstudentwaiting--; // Since this student is no longer waiting we decrements its count and aslo increment the number of people who got result.
    pthread_mutex_unlock(&lock);
    return NULL;
}
void* company(void * inp)
{
    companyinfo* input =(companyinfo*)inp;
    int num = input->id; 
    while(peoplewhogotresult<numberstudent) // the company will only check the vaccines count until there is atlest one student who needs to vaccinated at the time of checking.
    {
        if(batchescompleted[num]==batchesproduced[num]) // if produced = completely exhauseted batches then reproduction needs to be done.
        {
            printf("All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n",num);
            int ti=randomnumber(2,5); // generting random number time for which it should wait.
            int vvv=randomnumber(1,5);// generting random number of batches 
            printf("Pharmaceutical Company %d is preparing %d batches of vaccines which have success probability %Lf\n",num,vvv,probability[num]);

            sleep(ti);
            pthread_mutex_lock(&companylock[num]);

            batchesproduced[num]=vvv; // Once the sleep is done the number of batches produces is updates.
            batchesuntouched[num]=batchesproduced[num];// number of batches which are not even touched are initalized to number of batches produced.
            vacinesinbatch[num]=randomnumber(10,20);// This will generate vacines per batch
            batchescompleted[num]=0;// number of vaccines completd is 0.
            printf("Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %Lf. Waiting for all the vaccines to be used to resume production\n",num,batchesproduced[num],probability[num]);
            pthread_mutex_unlock(&companylock[num]);
        }
    }
    return NULL;
}
void* vacination(void * inp)
{
    vacinationinfo* input = (vacinationinfo*)inp;
    int num=input->id;
    int currcomp=-1; // this will shows the company of the last batch which it took.
    while(peoplewhogotresult<numberstudent)
    {
        if(vacines[input->id]==0)
        {
            if(currcomp!=-1)
            {
                pthread_mutex_lock(&companylock[currcomp]);
                batchescompleted[currcomp]++;// If all the vacdcines are done then the vaccines which are completed are increased.
                pthread_mutex_unlock(&companylock[currcomp]);
            }
            printf("Vaccination Zone %d has run out of vaccines\n",input->id);
            while(vacines[input->id]==0&&peoplewhogotresult<numberstudent)
            {
                // printf("%d %d\n",peoplewhogotresult,numberofstudentwaiting);
                for(int j=1;j<=numbercompany;j++)
                {

                    // printf("%d..",peoplewhogotresult);
                    pthread_mutex_lock(&companylock[j]);
                    // printf("%d %d %d\n",batchesuntouched[j],batchescompleted[j],batchesproduced[j]);
                    if(batchesuntouched[j]>0) // we are checking if ith company has any untouched vaccines.
                    {
                        currcomp=j; // if it has untouched batch the give it to this zone and update the currcomp(any) variable.
                        printf("Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success  probability %Lf and number of vaccines in the batch are %d\n",j,num,probability[j],vacinesinbatch[j]);
                        sleep(0.2);
                        printf("Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n",j,input->id);
                        prob[num]=probability[j]; // Make the probability of the current zones vaccines equal to the company from which the vaccines are taken.
                        batchesuntouched[j]--; // Number of untouched batches need to be decresed.
                        pthread_mutex_lock(&vacinelock[num]);
                        vacines[input->id]=vacinesinbatch[j]; // number of vacines are made equal to the number of vaccines in the company's batch it took from.
                        pthread_mutex_unlock(&vacinelock[num]);
                    }
                    pthread_mutex_unlock(&companylock[j]);
                    if(vacines[input->id]!=0)break;
                }
            }
        }
        int vv=min(8,min(vacines[num],numberofstudentwaiting)); //This is to calculate the maximum number of slots which the zone can allocate.
        if(vv==0)continue;  
        pthread_mutex_lock(&vacinelock[num]);
        peopleleft[num]=0; // This is the variable which stores number of students in the current zone.
        pthread_mutex_unlock(&vacinelock[num]);
        printf("Vaccination Zone %d is ready to vaccinate with %d slots\n",num,vv);
        pthread_mutex_lock(&vacinelock[num]);
        slotsleft[num]=vv; // Slots left is made equal to the number we cacluted before.
        pthread_mutex_unlock(&vacinelock[num]);
        while(slotsleft[num]>0&&numberofstudentwaiting>0&&peoplewhogotresult<numberstudent){
            // printf("%d %d===\n",peoplewhogotresult,numberofstudentwaiting);
            // This statement makes sure that all the possible stiudets who are wating will be given slots if they are availabel.
            }
        pthread_mutex_lock(&vacinelock[num]);
        slotsleft[num]=0; // We will now stop the intake of the students by making the slots left in  zone equl to 0, irrespectiuve of what happnes latter. 
        pthread_mutex_unlock(&vacinelock[num]);
        if(peopleleft[num]==0)
        {
            // if no one enerterd we will not proceed any further.
            printf("Nobody came to the vaccination zone %d although it is free\n",num);
            continue;
        }
        // printf("%d\n",slotsleft[num]);
        printf("Vaccination Zone %d entering Vaccination Phase\n",num);
        sleep(3); // processing time
        pthread_mutex_lock(&vacinelock[num]);
        inzone[num]=1; // This variable represents that all the processes in the current zone are free to print their result.
        pthread_mutex_unlock(&vacinelock[num]);
        while(peopleleft[num]>0&&peoplewhogotresult<numberstudent){} // makes sure that all the students in the curtrent thread gave the result.
        pthread_mutex_lock(&vacinelock[num]);
        inzone[num]=0; // Once all are done we should once again lock this zone.
        pthread_mutex_unlock(&vacinelock[num]);
        printf("Vaccination Zone %d finished Vaccination Phase\n",num);
    }
    return NULL;
}
int main()
{
    takeinput();
    if(numbervacination==0)
    {
        printf("Number of vaccination zones cannot be zero may lead to infinite loop\n");
        return 0;
    }
    if(numbercompany==0)
    {
        printf("Number of companies cannot be zero since this may lead to infinite loop\n");
        return 0;
    }
    pthread_t studentthread[numberstudent+1]; // Created threads for each students.
    pthread_t comapanythread[numbercompany+1]; // Cretaed threads for company.
    pthread_t vacinationthread[numbervacination+1]; // Created threads for vaccination.
    for(int i=0;i<1005;i++)
    {
        roundnumber[i]=1; // Round numbers of all students are initialized to 1.
        ishecured[i]=0;   //  This represent if the student got result.
    }
    pthread_mutex_init(&lock,NULL);
    pthread_mutex_init(&studentlock,NULL);
    for(int i=0;i<=numbercompany;i++) 
    pthread_mutex_init(&companylock[i],NULL);
    for(int i=0;i<=numbervacination;i++)
    pthread_mutex_init(&vacinelock[i],NULL);

    studentinfo* studentinput[numberstudent+1];
    for(int i=1;i<=numberstudent;i++)
    {
        studentinput[i]= (studentinfo*)malloc(sizeof(studentinfo));
        studentinput[i]->id = i;
    }
    companyinfo* companyinput[numbercompany+1];
    for(int i=1;i<=numbercompany;i++)
    {
        companyinput[i]= (companyinfo*)malloc(sizeof(companyinfo));
        companyinput[i]->id = i;
    }
    vacinationinfo* vacinationinput[numbervacination+1];
    for(int i=1;i<=numbervacination;i++)
    {
        vacinationinput[i]= (vacinationinfo*)malloc(sizeof(vacinationinfo));
        vacinationinput[i]->id = i;
    }
    for(int i=1;i<=numbercompany;i++)
    {
        pthread_create(&comapanythread[i],NULL,company,(void*)(companyinput[i]));
    }
    for(int i=1;i<=numberstudent;i++)
    {
        pthread_create(&studentthread[i],NULL,student,(void*)(studentinput[i]));
    }
    for(int i=1;i<=numbervacination;i++)
    {
        pthread_create(&vacinationthread[i],NULL,vacination,(void*)(vacinationinput[i]));
    }
    for(int i=1;i<=numbercompany;i++)
    {
        pthread_join(comapanythread[i],NULL);
    }
    for(int i=1;i<=numberstudent;i++)
    {
        pthread_join(studentthread[i],NULL);
    }
    for(int i=1;i<=numbervacination;i++)
    {
        pthread_join(vacinationthread[i],NULL);
    }

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&studentlock);
    for(int i=0;i<=numbercompany;i++)
    pthread_mutex_destroy(&companylock[i]);
    for(int i=0;i<=numbervacination;i++)
    pthread_mutex_destroy(&vacinelock[i]);

    printf("Simulation Over\n");
    return 0;
}