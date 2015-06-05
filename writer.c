#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>
#define SEMPERM 0600
#define TRUE 1
#define FALSE 0
typedef union   _semun {
             int val;
             struct semid_ds *buf;
             ushort *array;
             } semun;

int initsem (key_t semkey, int n)
{
   int status = 0, semid;
   if ((semid = semget (semkey, 1, SEMPERM | IPC_CREAT | IPC_EXCL)) == -1)
   {
       if (errno == EEXIST)
                semid = semget (semkey, 1, 0);
   }
   else
   {
       semun arg;
       arg.val = n;
       status = semctl(semid, 0, SETVAL, arg);
   }
   if (semid == -1 || status == -1)
   {
       perror("initsem failed");
       return (-1);
   }
   return (semid);
}

int p (int semid)
{
struct sembuf p_buf;
p_buf.sem_num = 0;
p_buf.sem_op = -1;
p_buf.sem_flg = SEM_UNDO;
if (semop(semid, &p_buf, 1) == -1)
{
perror ("p(semid) failed");
exit(1);
}
return (0);
}

int v (int semid)
{
struct sembuf v_buf;
v_buf.sem_num = 0;
v_buf.sem_op = 1;
v_buf.sem_flg = SEM_UNDO;
if (semop(semid, &v_buf, 1) == -1)
{
perror ("v(semid) failed");
exit(1);
}
return (0);
}

void readLength(int *length){
	FILE* lenr = fopen("okToWrite_Length", "r");
	while(feof(lenr)==0){
		fscanf(lenr, "%d", length);	
	}
	fclose(lenr);
}

void writeLength(int *length){
		FILE* lenw = fopen("okToWrite_Length", "a");
		fprintf(lenw, "%d\n", *length);
		fclose(lenw);
}

int wait(int cond, int lock){
	int length=0;
	
	readLength(&length);
	length++;
	writeLength(&length);

	v(lock);	//lock->Release();
	p(cond);	//s.P();
	p(lock);	//lock -> Acquire();

	return(0);
}

int signal(int cond, int lock){
	int length=0;
	readLength(&length);
	if(length>0){
		v(cond);
		//readLength(&length);
		length--;
		writeLength(&length);
	}
	return(0);	
}

int broadCast(int cond, int lock){
	int length=0;
	readLength(&length);
	while(length>0){
		v(cond);
		//readLength(&length);
		length--;
		writeLength(&length);
		readLength(&length);
	}
	return(0);
}

void readFile(int *AW,int *AR,int *WW,int *WR){
		FILE* AWr = fopen("AW", "r");
		while(feof(AWr)==0){
			fscanf(AWr, "%d", AW);	
		}
		fclose(AWr);

		FILE* ARr = fopen("AR", "r");
		while(feof(ARr)==0){
			fscanf(ARr, "%d", AR);	
		}
		fclose(ARr);

	 	FILE* WWr = fopen("WW", "r");
		while(feof(WWr)==0){
			fscanf(WWr, "%d", WW);	
		}
		fclose(WWr);

	 	FILE* WRr = fopen("WR", "r");
		while(feof(WRr)==0){
			fscanf(WRr, "%d", WR);	
		}
		fclose(WRr);

}
void writeFile(int *AW,int *AR,int *WW,int *WR){
	
		time_t     current_time;
   		struct tm *struct_time;
  		time( &current_time);
  		struct_time = localtime( &current_time);

 //  printf( "%d시%d분%d초", struct_time->tm_hour , struct_time->tm_min, struct_time->tm_sec     );


		FILE* AWw = fopen("AW", "a");
		fprintf(AWw, "%d %d%d  ",getpid(),struct_time->tm_min,struct_time->tm_sec);
		fprintf(AWw, "%d\n",*AW);
		fclose(AWw);

		FILE* WWw = fopen("WW", "a");
		fprintf(WWw, "%d %d%d  ",getpid(),struct_time->tm_min,struct_time->tm_sec);
		fprintf(WWw, "%d\n",*WW);
		fclose(WWw);
		
		FILE* ARw = fopen("AR", "a");
		fprintf(ARw, "%d %d%d  ",getpid(),struct_time->tm_min,struct_time->tm_sec);
		fprintf(ARw, "%d\n",*AR);
		fclose(ARw);	
		
		FILE* WRw = fopen("WR", "a");
		fprintf(WRw, "%d %d%d  ",getpid(),struct_time->tm_min,struct_time->tm_sec);
		fprintf(WRw, "%d\n",*WR);
		fclose(WRw);
}


void main(int argc, char *argv[])
{
	sleep(atoi(argv[1]));
   	key_t semkey = 0x200; 
	key_t key_m = 2011331500;	//lock
	key_t key_okR = 2011331501;//okread (condi)
	key_t key_okW = 2011331502;//okwirte (condi)
	int semid,lock,okToRead,okToWrite;

	pid_t pid;

	pid = getpid();

	if ((lock = initsem(key_m,1)) < 0)    
		exit(1);
	if ((okToRead = initsem(key_okR,0)) < 0)
		exit(1);
	if ((okToWrite = initsem(key_okW,0)) < 0)
		exit(1);

	printf("process %d in critical section\n",pid);

	// First check self into system
	p(lock);					//lock.Acquire();
		int AW=0,AR=0,WW=0,WR=0;
		readFile(&AW,&AR,&WW,&WR);

		while ((AW + AR) > 0) { 		// Is it safe to read?
			//readFile(&AW,&AR,&WW,&WR);
			WW++;			// No. Writers exist
			writeFile(&AW,&AR,&WW,&WR);

			wait(okToWrite,lock);		//okToRead.wait(&lock);

			readFile(&AW,&AR,&WW,&WR);
			WW--;			// No longer waiting
			writeFile(&AW,&AR,&WW,&WR);
		}
		AW++;				// Now we are active!
		writeFile(&AW,&AR,&WW,&WR);
	v(lock);					//lock.Release();

		sleep(atoi(argv[2]));


	// Now, check out of system
	p(lock);	
		readFile(&AW,&AR,&WW,&WR);				
		AW--;			// No longer active
		writeFile(&AW,&AR,&WW,&WR);

		readFile(&AW,&AR,&WW,&WR);
		if (WW > 0){ 			// No other active readers
			signal(okToWrite,lock);	//okToWrite.signal();
		}

		//readFile(&AW,&AR,&WW,&WR);
		else if(WR > 0){
			broadCast(okToRead,lock);	//okToRead.broadcast();
		}
	v(lock);					//lock.Release();

	exit(0);
}
