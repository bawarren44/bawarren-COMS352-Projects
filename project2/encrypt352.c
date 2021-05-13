#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "encrypt.h"
#include "encrypt.c"

pthread_t readt, writet, encryptt, inCountt, outCountt;
char *inBuf, *outBuf, *inFile, *outFile;
int bufSize;
sem_t in, out, inF, outF, readE, writeF, encF, encE;
pthread_mutex_t inMutex, outMutex, readMutex, writeMutex, encMutex;


/**
* To check if a character is a letter. Returns 1 if c is a letter, else 0. 
*/
char letter(char c){
	c = tolower(c);
	char *alphabet = "abcdefghijklmnopqrstuvwxyz";
	if(strchr(alphabet, c) != NULL){
		return c;
	}
	return '~';
}

/**
* Used to print the number of each letter for input and output counts
*/ 
void printCounts(){
	int j = 17; //Start for A
	int k = 65; //Start in counts for A
	printf("Input file contains\n");
	for(int i = 0; i < 26; i++){
		char c = (j+i) + '0';
		printf("%c:%d ", c,input_counts[k+i]);
	}
	printf("\nOutput file contains\n");
	for(int i = 0; i < 26; i++){
		char c = (j+i) + '0';
		printf("%c:%d ", c,output_counts[k+i]);
	}
	printf("\n");
}
void reset_requested(){ 
	pthread_mutex_lock(&readMutex);
	pthread_mutex_lock(&inMutex);
	pthread_mutex_lock(&encMutex);
	pthread_mutex_lock(&writeMutex);
	pthread_mutex_lock(&outMutex);
	printCounts();
}

void reset_finished(){
	pthread_mutex_unlock(&readMutex);
	pthread_mutex_unlock(&inMutex);
	pthread_mutex_unlock(&encMutex);
	pthread_mutex_unlock(&writeMutex);
	pthread_mutex_unlock(&outMutex);
	printf("reset finished\n");
}

void *readFunc(void *arg){
	int index = 0;
	while(1){
		pthread_mutex_lock(&readMutex);
		char c = read_input(inFile);
		sem_wait(&readE);
		sem_wait(&in);
		inBuf[index] = c;
		index = (index+1) % bufSize;
		sem_post(&inF);
		sem_post(&encF);
		pthread_mutex_unlock(&readMutex);
		if(c == EOF){
			break;
		}
	}
}

void *writeFunc(void *arg){
	int index = 0;
	while(1){
		sem_wait(&writeF);
		char c = outBuf[index];
		write_output(c);
		index = (index+1)%bufSize;
		sem_post(&encE);
		sem_post(&out);
		int lock;
		sem_getvalue(&writeF, &lock);
		if(lock){
			pthread_mutex_lock(&writeMutex);
		}else {
			pthread_mutex_unlock(&writeMutex);
		}
		if(c == EOF){
			break;
		}
	}
}

void *encryptp(void *arg){
	int index = 0;
	while(1){
		sem_wait(&encF);
		char c = caesar_encrypt(inBuf[index]);
		sem_post(&readE);
		sem_wait(&encE);
		sem_wait(&out);
		outBuf[index] = c;
		index = (index+1) % bufSize;
		sem_post(&outF);
		sem_post(&writeF);
		int lock;
		sem_getvalue(&encF, &lock);
		if(lock){
			pthread_mutex_lock(&encMutex);
		}else {
			pthread_mutex_unlock(&encMutex);
		}
		if(c == EOF){
			break;
		}
	}
}

void *inCount(void *arg){
	int index = 0;
	while(1){
		sem_wait(&inF);
		char c = inBuf[index];
		index = (index+1) % bufSize;
		count_input(c);
		sem_post(&in);
		int lock;
		sem_getvalue(&inF, &lock);
		if(lock){
			pthread_mutex_lock(&inMutex);
		}else {
			pthread_mutex_unlock(&inMutex);
		}
		if(c == EOF){
			break;
		}
	}
}

void *outCount(void *arg){
	int index = 0;
	while(1){ 
		sem_wait(&outF);
		char c = outBuf[index];
		index = (index+1) % bufSize;
		count_output(c);
		sem_post(&out);
		int lock;
		sem_getvalue(&outF, &lock);
		if(lock){
			pthread_mutex_lock(&outMutex);
		}else {
			pthread_mutex_unlock(&outMutex);
		}
		if(c == EOF){
			break;
		}
	}
}
/**
* Main thread
*/ 
int main(int argc, char** argv){
	//check for correct commandline input
	if(argc != 3){
		fprintf(stderr, "Error: input must contain 3 arguments\n"
		"Format: \"\\.encrypt352 <input file> <output file>\"\n");
		exit(1);
	}

	//initialize inFile and outFile
	inFile = argv[1];
	outFile = argv[2];

	//open input and output files
	open_input(inFile);
	open_output(outFile);

	//get user input
	printf("Please input buffer size: ");
	scanf("%s", &bufSize);

	//allocate space in inBuffer and outBuffer according to user input
	inBuf = malloc(sizeof(char) * bufSize);
	outBuf = malloc(sizeof(char) * bufSize);
			
	//initialize semaphores
	sem_init(&in, 0, 1);
	sem_init(&out, 0, 1);
	sem_init(&inF, 0, 0);
	sem_init(&outF, 0, 0);
 	sem_init(&readE, 0, bufSize);
	sem_init(&writeF, 0, 0);	
	sem_init(&encF, 0, 0);
	sem_init(&encE, 0, bufSize);
	
	//initialize mutexes
	pthread_mutex_init(&inMutex, NULL);
	pthread_mutex_init(&outMutex, NULL);
	pthread_mutex_init(&encMutex, NULL);
	pthread_mutex_init(&writeMutex, NULL);
	pthread_mutex_init(&readMutex, NULL);


	//create threads
	pthread_create(&readt, NULL, readFunc, NULL);
	pthread_create(&inCountt, NULL, inCount, NULL);
	pthread_create(&encryptt, NULL, encryptp, NULL);
	pthread_create(&writet, NULL, writeFunc, NULL);
	pthread_create(&outCountt, NULL, outCount, NULL);

	//join threads
	pthread_join(readt, NULL);
	pthread_join(inCountt, NULL);
	pthread_join(encryptt, NULL);
	pthread_join(writet, NULL);
	pthread_join(outCountt,NULL);

	//destroy semaphores
	sem_destroy(&in);
	sem_destroy(&out);
	sem_destroy(&inF);
	sem_destroy(&outF);
	sem_destroy(&readE);
	sem_destroy(&writeF);
	sem_destroy(&encF);
	sem_destroy(&encE);
	
	

	printCounts();
	printf("End of file reached\n");

	return 0;
}
