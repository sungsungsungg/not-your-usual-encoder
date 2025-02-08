#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#define BUFFER_SIZE 4096
#define MAX_SIZE 250000


pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t task_id_mutex = PTHREAD_MUTEX_INITIALIZER;

int num_thread = 0;
int working =0;
int task_id =0;


typedef struct{
  char letter;
  unsigned int count;
}Elem;


typedef struct{
  Elem* elements;
  int index;
  int num_element;


}Result;

typedef struct{
  Result* results;


  pthread_cond_t available;
  int next_to_process;


}ResultArray;



ResultArray* resultf;




typedef struct{
  char *data;
  int index;
  size_t length;

} Task;


typedef struct{
  Task **tasks;
  int front;
  int rear;
  bool done;
  int count;
  pthread_cond_t not_empty;
  pthread_mutex_t mutex;


}TaskQueue;







TaskQueue* createqueue(void){
  TaskQueue* tq = malloc(sizeof(TaskQueue));
  tq->tasks = malloc(sizeof(Task*)*MAX_SIZE);
  tq->front =0;
  tq->rear=0;
  tq->count=0;
  tq->done = false;

  pthread_mutex_init(&tq->mutex, NULL);
  pthread_cond_init(&tq-> not_empty, NULL);

  return tq;
}


void enqueue(TaskQueue *queue, Task *task){
  pthread_mutex_lock(&queue->mutex);
  queue->tasks[queue->rear]= task;
  queue ->rear = (queue->rear+1)%MAX_SIZE;
  queue ->count++;
  pthread_cond_signal(&queue->not_empty);
  pthread_mutex_unlock(&queue->mutex);

}






Task* dequeue(TaskQueue *queue){

  pthread_mutex_lock(&queue->mutex);
  while(queue->count==0 && !queue->done){
    pthread_cond_wait(&queue->not_empty,&queue->mutex);
  }

    

  Task *task = queue->tasks[queue->front++];
  queue->front = queue->front%MAX_SIZE;
  queue->count--;
  pthread_mutex_unlock(&queue->mutex);
  return task;
}

void enqueue_done(TaskQueue *queue){
  pthread_mutex_lock(&queue->mutex);
  queue->done = true;
  pthread_cond_broadcast(&queue->not_empty);
  pthread_mutex_unlock(&queue->mutex);
}

Result* processing(char* data,int len){


  char prev;
  int count=0;
  int ind =0;

  Result* res = malloc(sizeof(Result));
  res->num_element =0;
  res->elements = (Elem*)malloc(4098*sizeof(Elem));
  

  for(int j=0; j<len; j++){
      if (j == 0){
        prev = data[j];
        count = 1;
      }
      else{
        if (prev == data[j]){
          count++;
        }
        else{

          res->elements[ind].letter = prev;
          res->elements[ind++].count = count;

          res->num_element++;
          count =1;
          prev = data[j];
        }

      }
    }

    res->elements[ind].letter = prev;
    res->elements[ind++].count = count;
    res->num_element++;
    // res->elements[ind].letter = '\0';


    return res;
}

void put_result(Result *res){
  
  pthread_mutex_lock(&result_mutex);

  resultf->results[res->index] = *res;
  if(res->index == resultf->next_to_process){
    pthread_cond_broadcast(&resultf->available);
  }

  pthread_mutex_unlock(&result_mutex);

}




void *working_thread(void *arg){
  TaskQueue *queue = (TaskQueue *)arg;

  while(1){


    Task *task= dequeue(queue);

    if (task==NULL) break;



    Result *result =processing(task->data,task->length);
    //  malloc(sizeof(Result));
    // *result = processing(task->data);
    result->index = task->index;


    put_result(result);


    free(task->data);
    free(task);


  }
  return NULL;

}





int main(int arg, char **argv) {



  char prev;
  int count =0;
  int a =0;




  if(strcmp("-j",argv[1])==0){

    resultf = malloc(sizeof(ResultArray));
    resultf->results = malloc(sizeof(Result)*MAX_SIZE);
    resultf->next_to_process=0;


    for(int i =0; i<MAX_SIZE;i++){
      resultf->results[i].elements = NULL;
    }

    
    if(arg <4){
      exit(0);
    }

    if(atoi(argv[2])<1) exit(0);

    num_thread = atoi(argv[2]);

    TaskQueue *task_queue = createqueue();
    pthread_t threads[num_thread];



    //Created Threads

    for (int i=0; i< num_thread; i++){
      pthread_create(&threads[i],NULL,working_thread,(void *)task_queue);
    }



//********************************************************************** Enqueueing Task

    for (int i=3; i<= arg; i++){
      int starting =0;
      if (argv[i]==NULL) continue;
      int fd = open(argv[i],O_RDONLY);

      struct stat sb;
      if (fstat(fd,&sb)==-1){
        close(fd);
        
      }
    

      char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      if(addr==MAP_FAILED){
        close(fd);
      }



      while(starting < sb.st_size){
        size_t buffer_size = BUFFER_SIZE;
        if (starting+BUFFER_SIZE > sb.st_size){
          buffer_size = sb.st_size-starting;
        }

        Task *task = malloc(sizeof(Task));
        task->data = malloc(buffer_size);

        memcpy(task->data,addr+starting,buffer_size);
        // task->data[buffer_size]='\0';
        task->length = buffer_size;
        // pthread_mutex_lock(&task_id_mutex);
        task->index = task_id++;
        // pthread_mutex_unlock(&task_id_mutex);
        enqueue(task_queue,task);
        starting += buffer_size;


      }


      close(fd);
      munmap(addr,sb.st_size);

      
    }


    if(task_id==0){
      for (int i=0; i< num_thread;i++){
      enqueue(task_queue, NULL);
    }




    for(int i=0;i<num_thread; i++){
      pthread_join(threads[i],NULL);
    }
      char ending;
      int ending2=0;
      fwrite(&ending,1,1,stdout);
      fwrite(&ending2,1,1,stdout);
      exit(0);
    }
// ********************************************************************* End of Enqueueing
    enqueue_done(task_queue);


    Elem* temp =NULL;
    for(int i=0;i<task_id;i++){
      if(i==0){
        pthread_mutex_lock(&result_mutex);
        while(resultf->results[0].elements==NULL){
          pthread_cond_wait(&resultf->available,&result_mutex);
        }
        // pthread_mutex_unlock(&result_mutex);

      }
      else{
        pthread_mutex_lock(&result_mutex);
        while(resultf->results[i].elements==NULL){
          pthread_cond_wait(&resultf->available,&result_mutex);
        }
        // pthread_mutex_unlock(&result_mutex);

        if(i==0){

        }
        if(temp->letter==resultf->results[i].elements[0].letter){
          resultf->results[i].elements[0].count += temp->count;
        }
        else{
          fwrite(&temp->letter,1,1,stdout);
          fwrite(&temp->count,1,1,stdout);
        }
      }
      for(int j=0;j<resultf->results[i].num_element-1;j++){
        fwrite(&resultf->results[i].elements[j].letter,1,1,stdout);
        fwrite(&resultf->results[i].elements[j].count,1,1,stdout);

      }
      temp = &resultf->results[i].elements[resultf->results[i].num_element-1];  
      resultf->next_to_process++;
      pthread_mutex_unlock(&result_mutex);
    }



    fwrite(&temp->letter,1,1,stdout);
    fwrite(&temp->count,1,1,stdout);






    for (int i=0; i< num_thread;i++){
      enqueue(task_queue, NULL);
    }




    for(int i=0;i<num_thread; i++){
      pthread_join(threads[i],NULL);
    }

    pthread_mutex_destroy(&task_queue->mutex);
    pthread_cond_destroy(&task_queue->not_empty);
    free(task_queue->tasks);
    free(task_queue);
    free(resultf->results->elements);
    free(resultf);
    pthread_mutex_destroy(&result_mutex);
    
    pthread_cond_destroy(&resultf->available);



  }





  else{
    for (int i =1; i <= arg; i++){
      if (argv[i]== NULL) continue;


      int fd = open(argv[i],O_RDONLY);
      if (fd == -1){
        close(fd);

      }



      struct stat sb;
      if (fstat(fd,&sb)==-1){
        close(fd);

      }

      char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);




      if(addr==MAP_FAILED){
        close(fd);

      }


      for(int j=0; j<sb.st_size; j++){

        if (a == 0){
          prev = addr[j];
          a++;
          count = 1;
        }

        else{
          if (prev == addr[j]){
            count++;
          }
          else{
            // printf("%c",prev);
            fwrite(&prev, 1,1,stdout);
            fwrite(&count, 1,1,stdout);
            count =1;
            prev = addr[j];
          }

        }
      }
      


    }
    fwrite(&prev, 1,1,stdout);
    fwrite(&count, 1,1,stdout);
    

  }







}


