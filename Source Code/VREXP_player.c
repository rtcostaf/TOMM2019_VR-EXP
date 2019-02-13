#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#define BUFFER_LIMIT 2
#define INTERVAL 1 //time to sleep
#define SEGMENT_TIME 1 
#define AVERAGE_WINDOW_SIZE 5

// ---- Global variables
//default value
int NUM_THREADS = 3;

char *video_id;
char *destination;

int mleni;  //vertical (height)
int mlenj; 

char resolutionVp[5];
char resolutionAdj[5];
char resolutionOut[5];

int* viewport;
int* adjacency;
int* outside;

int* viewport_error;
int* adjacency_error;
int* outside_error;

struct timeval beg, end, last_load, former_last_load;

FILE *f;
FILE *flog;

double *avg_window;
CURL *curl;

int current_index;

pthread_mutex_t mutex_control;


//statistics
static int tiles_count_global = 0;
static double maxbitrate = 0;
static double avgbitratez1 = 0.0, avgbitratez2 = 0.0, avgbitratez3 = 0.0;
static int    contz1 = 0, contz2 = 0, contz3 = 0;
static int    cont720z1 = 0, cont1080z1 = 0, cont4kz1 = 0;
static int    cont720z2 = 0, cont1080z2 = 0, cont4kz2 = 0;
static int    cont720z3 = 0, cont1080z3 = 0, cont4kz3 = 0;


struct arg_struct {
  int i; 
  int lower_limit;
  int upper_limit;
}args;

double randfrom(double min, double max) 
{
    double range = (max - min); 
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

double tvdiff_secs(struct timeval newer, struct timeval older)
{
  if(newer.tv_sec != older.tv_sec)
    return (double)(newer.tv_sec-older.tv_sec)+
      (double)(newer.tv_usec-older.tv_usec)/1000000.0;
  return (double)(newer.tv_usec-older.tv_usec)/1000000.0;
}

void buildURL(char* url, char* destination, char* video_id, int mleni, int mlenj, char* resolution, int tile, int segment){
    
    
    url[0] = '\0';
    
    char mleniToChar[5], mlenjToChar[5];

    sprintf(mleniToChar, "%d", mleni); 
    sprintf(mlenjToChar, "%d", mlenj);

    strcat(url, "http://");
    strcat(url, destination);
    strcat(url, "/");
    strcat(url, video_id);
    strcat(url, "/");
    
    strcat(url, mlenjToChar);
    strcat(url, "x");
    strcat(url, mleniToChar);
    
    strcat(url, "/");
    strcat(url, "out");
    strcat(url, "_");
    strcat(url, video_id);
    strcat(url, "_");
    strcat(url, resolution);
    strcat(url, "_");
    
    strcat(url, mlenjToChar);
    strcat(url, "x");
    strcat(url, mleniToChar);

    strcat(url, "_");
    strcat(url, "tiled_dash");
    strcat(url, "_");
    strcat(url, "track");
            
    char tileToChar[5];
    sprintf(tileToChar, "%d", tile);
    strcat(url, tileToChar);
            
    strcat(url, "_");
    char index[8];
    sprintf(index, "%d", segment);
    strcat(url, index);
    strcat(url, ".m4s");
    strcat(url, "\0");

} 

/*
Thread function which implements the download of tiles in Z2 (adjacency layer). 
*/
void downloadZ2(void *ptr){


  struct arg_struct *aux = (struct arg_struct *) ptr;

  double elapsed; 
  double volume;
  double ttime;
  double bitrate;
  double local_avg;
  double refAdj;

  char url[200];
  CURLcode res;

  int j, jTile;

  CURL *curl2 = curl_easy_init();

  if(!curl2) {
      printf("memory allocation CURL z2\n");
      exit(-1);
  }

  for (jTile = aux->lower_limit; jTile <= aux->upper_limit; jTile++){
            
            buildURL(url, destination, video_id, mleni, mlenj, resolutionAdj, adjacency[jTile], aux->i);
            
            curl_easy_setopt(curl2, CURLOPT_URL, url);
            curl_easy_setopt(curl2, CURLOPT_WRITEDATA, f);

            res = curl_easy_perform(curl2);

            curl_easy_getinfo(curl2, CURLINFO_SIZE_DOWNLOAD, &volume);
            curl_easy_getinfo(curl2, CURLINFO_TOTAL_TIME, &ttime);
            

            gettimeofday(&last_load, NULL);
            elapsed = tvdiff_secs(last_load, beg);
            bitrate = (8.0 * volume) / ttime;
            if (bitrate > maxbitrate) maxbitrate = bitrate; 
            
            double local_sum = 0.0; 
            
            pthread_mutex_lock(&mutex_control);
           
            avgbitratez2 += bitrate;
            tiles_count_global++;
            contz2++;
            avg_window[current_index] = bitrate;
            int local_avg_window_size = tiles_count_global < AVERAGE_WINDOW_SIZE ? tiles_count_global : AVERAGE_WINDOW_SIZE;
            
            for (j = 0; j < local_avg_window_size; j++){
               local_sum += avg_window[j];
            }
            
            pthread_mutex_unlock(&mutex_control);

            local_avg = local_sum / local_avg_window_size;
            
            refAdj = local_avg; //< bitrate ? local_avg : bitrate);
            
            pthread_mutex_lock(&mutex_control);
            fprintf(flog, "%lf;%d;%s;%lf;%lf;%lf;%lf;%lf;adj\n", elapsed, aux->i, resolutionAdj, refAdj, local_avg, bitrate, volume, ttime);
            fflush(flog);
            pthread_mutex_unlock(&mutex_control);

        }
        curl_easy_cleanup(curl2);
       
        //free(ptr);
}

/*
Thread function which implements the download of tiles in Z3 (outside layer). 
*/
void downloadZ3(void *ptr){


  struct arg_struct *aux = (struct arg_struct *) ptr;

  double elapsed; 
  double volume;
  double ttime;
  double bitrate;
  double local_avg;
  double refOut;

  char url[200];
  CURLcode res;

  int j, jTile;

  CURL *curl2 = curl_easy_init();

  if(!curl2) {
      printf("memory allocation CURL z3\n");
      exit(-1);
  }

  for (jTile = aux->lower_limit; jTile <= aux->upper_limit; jTile++){
            
            buildURL(url, destination, video_id, mleni, mlenj, resolutionOut, outside[jTile], aux->i);
            
            curl_easy_setopt(curl2, CURLOPT_URL, url);
            curl_easy_setopt(curl2, CURLOPT_WRITEDATA, f);

            res = curl_easy_perform(curl2);

            curl_easy_getinfo(curl2, CURLINFO_SIZE_DOWNLOAD, &volume);
            curl_easy_getinfo(curl2, CURLINFO_TOTAL_TIME, &ttime);
            

            gettimeofday(&last_load, NULL);
            elapsed = tvdiff_secs(last_load, beg);
            bitrate = (8.0 * volume) / ttime;
            
            if (bitrate > maxbitrate) maxbitrate = bitrate; 
            double local_sum = 0.0;

            pthread_mutex_lock(&mutex_control);
            avgbitratez3 += bitrate;
            avg_window[current_index] = bitrate;
            tiles_count_global++;
            contz3++;

            int local_avg_window_size = tiles_count_global < AVERAGE_WINDOW_SIZE ? tiles_count_global : AVERAGE_WINDOW_SIZE;
      
            for (j = 0; j < local_avg_window_size; j++){
               local_sum += avg_window[j];
            }

            pthread_mutex_unlock(&mutex_control);
            
            local_avg = local_sum / local_avg_window_size;
            
            refOut = local_avg;
            
            pthread_mutex_lock(&mutex_control);
            fprintf(flog, "%lf;%d;%s;%lf;%lf;%lf;%lf;%lf;out\n", elapsed, aux->i, resolutionOut, refOut, local_avg, bitrate, volume, ttime);
            fflush(flog);
            pthread_mutex_unlock(&mutex_control);

        }
        curl_easy_cleanup(curl2);
        //free(ptr);
}



/*
Function which translates coordinates to the right Viewport tile. 
Observe tha the viewport (as implemented) can only be 1x1.
*/
void coordToViewPort(int** matrix, int** output, int mleni, int mlenj, float x, float y, int viewporth, int viewportw){

  float stepX = 0.0, stepY = 0.0;
 
  int tileX = 0;
  int tileY = 0;
  
  int i,j;

  if (x > 0){
    stepY = 3.14 / (float)(mlenj/2);
    tileY = y/stepY;
    j = tileY;

  }else{
    stepY = 3.14 / (float)(mlenj/2);
    tileY = -1 * y/stepY;
    j = mlenj - 1 - tileY;
  }

  if (y > 0){
    stepX = 1.57 / (float)(mleni/2);
    tileX = x/stepX;
    i = tileX;
   
  }else{
    stepX = 1.57 / (float)(mleni/2);
    tileX = -1 * x/stepX;
    i = mleni - 1 - tileX;
  }
  
  if (i >= mleni) i = mleni-1;
  if (j >= mlenj) j = mlenj-1;
  if (i < 0) i = 0;
  if (j < 0) j = 0;

  output[i][j] = 1;
  
}

/*
Function which fills output matrix with corresponding tiles indications.
When output[i][j] == 1 -> represent viewport tiles
When output[i][j] == 2 -> represent adjacency (Z2) tiles
When output[i][j] == 3 -> represent outside (Z3) tiles
*/
void viewPortToAdjacency(int **matrix, int **output, int mleni, int mlenj, int adjlen, 
                        int* viewport, int* contviewport, int* adjacency, int* contadjacency,
                        int* outside, int* contoutside){

  int i,j,k;

  for(i = 0; i < mleni; i++){
    for(j = 0; j < mlenj; j++){
      
      if(output[i][j] == 1){
        
        for(k = 1; k <= adjlen; k++){
          
          //right
          if (output[i][(j+k)%mlenj] == 3) output[i][(j+k)%mlenj] = 2;
          //up
          if (output[(i+k)%mleni][j] == 3) output[(i+k)%mleni][j] = 2;
        
          //down
          if (i-k>=0){
            if (output[i-k][j] == 3) output[i-k][j] = 2;
          }else{
            if (output[mleni-(-1*(i-k))][j] == 3) output[mleni-(-1*(i-k))][j] = 2;
          }
          //left
          if (j-k >= 0){
            if (output[i][j-k] == 3) output[i][j-k] = 2;
          }else{
            if (output[i][mlenj-(-1*(j-k))] == 3) output[i][mlenj-(-1*(j-k))] = 2;
          }
          
          //diagonal: (i+1)(j+1)
          if (output[(i+k)%mleni][(j+k)%mlenj] == 3) output[(i+k)%mleni][(j+k)%mlenj] = 2;

          //diagonal:(i+1)(j-1)
          if (j-k>=0){
            if (output[(i+k)%mleni][j-k] == 3) output[(i+k)%mleni][j-k] = 2;
          }else{
            if (output[(i+k)%mleni][mlenj-(-1*(j-k))] == 3) output[(i+k)%mleni][mlenj-(-1*(j-k))] = 2;
          }

          //diagonal:(i-1)(j+1)
          if (i-k>=0){
            if (output[i-k][(j+k)%mlenj] == 3) output[i-k][(j+k)%mlenj] = 2;
          }else{
            if (output[mleni-(-1*(i-k))][(j+k)%mlenj] == 3) output[mleni-(-1*(i-k))][(j+k)%mlenj] = 2;
          }

          //diagonal: (i-1)(j-1)
          if (i-k>=0 && j-k>=0){
            if (output[i-k][j-k] == 3) output[i-k][j-k] = 2;
          }else if(i-k>=0 && j-k < 0){
            if (output[i-k][mlenj-(-1*(j-k))] == 3) output[i-k][mlenj-(-1*(j-k))] = 2;
          }else if(i-k < 0 && j-k>=0){
            if (output[mleni-(-1*(i-k))][j-k] == 3) output[mleni-(-1*(i-k))][j-k] = 2;
          }else{
            if (output[mleni-(-1*(i-k))][mlenj-(-1*(j-k))] == 3) output[mleni-(-1*(i-k))][mlenj-(-1*(j-k))] = 2;
          }

        }
        
      
      }

    }
  }

  
  //counting
  
  *contviewport = 0;
  *contadjacency = 0;
  *contoutside = 0;


  for(i = 0; i < mleni; i++){
    for(j = 0; j < mlenj; j++){
      
      if(output[i][j] == 1){
        viewport[(*contviewport)++] = matrix[i][j];
      }
      if(output[i][j] == 2){
        adjacency[(*contadjacency)++] = matrix[i][j];
      }
      if(output[i][j] == 3){
        outside[(*contoutside)++] = matrix[i][j];
      } 
    }
  }

 
}

int main(int argc, char **argv)
{

  int contIR=0;

  CURLcode res;
  
  int stall_count = 0;
  
  pthread_mutex_init(&mutex_control, NULL);

  curl_global_init(CURL_GLOBAL_ALL);

  srand(time(0));

  avg_window = (double *)malloc(AVERAGE_WINDOW_SIZE * sizeof(double));
  
  if(avg_window == NULL){
    printf("Problem allocating memory: avg_window ");
    return 0;
  } 
  
  int j;
  for (j = 0; j < AVERAGE_WINDOW_SIZE; j++){
     avg_window[j] = 0.0;
  }
  
  int segment_count_i;
  
  double elapsed, elapsedTrace, startup_time, buffer_time, time_from_last_load, stall_len, bitrate;
  
  int i = 1; 
  
  if (argc < 11){
     printf("usage: cplayer <destination> <video_id> <segment_count> <uuid> <tracefile> <time_limit(sec)> <lines_i(4)> <column_j(8)> <error_rate> <num_threads>\n:");
     return(-1);
  }

  
  mleni = atoi(argv[7]);  //vertical (height)
  mlenj = atoi(argv[8]);  //horizontal (weidth)
  int error_rate = atoi(argv[9]);
  NUM_THREADS = atoi(argv[10]);

  pthread_t threadsId[NUM_THREADS + 1];

  int k,l;
  int c = 1;

  int **matrix = (int**)malloc( (mleni + 1) * sizeof(int*));
  int **output = (int**)malloc( (mleni + 1) * sizeof(int*));

  int **output_error = (int**)malloc( (mleni + 1) * sizeof(int*));

  if(matrix == NULL || output == NULL){
    printf("Problem allocating memory: matrix/output");
    return 0;
  } 
  

  for(k = 0; k < mleni; k++){

    matrix[k] = (int *) malloc ( (mlenj + 1) * sizeof(int));
    output[k] = (int *) malloc ( (mlenj + 1) * sizeof(int));
    
    output_error[k] = (int *) malloc ( (mlenj + 1) * sizeof(int));
    
    if(matrix[k] == NULL || output[k] == NULL || output_error[k] == NULL){
      printf("Problem allocating memory: matrix[k]/output[k] ");
      return 0;
    } 
  
  }

  for(k = 0; k < mleni; k++){
    for(l = 0; l < mlenj; l++){
      matrix[k][l] = c++;
      output[k][l] = 3;
      output_error[k][l] = 3;
    }
  }

  double volume;
  double ttime;
  destination = argv[1];
  video_id = argv[2];
  char *segment_count = argv[3];
  char *uuid = argv[4];
  char *tracefile = argv[5];
  int  timelimit = atoi(argv[6]);



  segment_count_i = atoi(segment_count);
  char url[200];
  
  char resolution[5];
  resolution[0] = '\0';
  strcat(resolution, "720");
  strcat(resolution, "\0");       


  
  resolutionVp[0] = '\0';
  strcat(resolutionVp, "720");
  strcat(resolutionVp, "\0");

  resolutionAdj[0] = '\0';
  strcat(resolutionAdj, "720");
  strcat(resolutionAdj, "\0");
  
  resolutionOut[0] = '\0';
  strcat(resolutionOut, "720");
  strcat(resolutionOut, "\0");


  char filename[80];
  filename[0] = '\0';
  strcat(filename, "videologs/");
  strcat(filename, uuid);
  strcat(filename, ".log");
  strcat(filename, "\0");
  //filename logfile
  flog = fopen(filename, "wb");


  curl = curl_easy_init();
  
  if(curl) {
  
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    f = fopen("/dev/null", "wb");

    //from trace
    float time,x,y;

    viewport = (int*) malloc (mleni * mlenj * sizeof(int));
    adjacency = (int*) malloc (mleni * mlenj * sizeof(int));
    outside  = (int*) malloc (mleni * mlenj * sizeof(int));

    viewport_error = (int*) malloc (mleni * mlenj * sizeof(int));
    adjacency_error = (int*) malloc (mleni * mlenj * sizeof(int));
    outside_error  = (int*) malloc (mleni * mlenj * sizeof(int));
    
    int switchVp = 0;
    int switchAdj = 0;
    int switchOut = 0;

    int stall_count_vp = 0;
    int stall_count_adj = 0;
    int stall_count_out = 0;

    FILE *ftrace = fopen(tracefile, "r");
    
    if (ftrace == NULL) {
      printf("Error: opening trace file");
      exit(-1);
    }

    gettimeofday(&beg, NULL);

    double contmili = 0;

    contmili = 0.0;

   
    double refOut = 0;
    double refAdj = 0;
    double refVp = 0;

    int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = 0, c6 = 0, c7 = 0;
    
    //i represents the current segment.
    while (i <= segment_count_i){

        if(viewport == NULL || adjacency == NULL || outside == NULL){
          printf("Problem allocating memory: viewport/adjacency/outside");
          return 0;
        }  

        int len_viewport  = 0; 
        int len_adjacency = 0; 
        int len_outside   = 0;

        //syncronization
        sleep(0.1);
  
        gettimeofday(&last_load, NULL);
        elapsedTrace = tvdiff_secs(last_load, beg);
        
        //find the right coordinates in the trace file.
        do{

          fscanf(ftrace,"%f;%f,%f",&time, &x, &y);
          contmili += time;
          
        }while(contmili < (elapsedTrace*1000) - 1000);
        
        //re-initialize output matrix.
        for(k = 0; k < mleni; k++){
          for(l = 0; l < mlenj; l++){
             output[k][l] = 3;
             output_error[k][l] = 3;
           }
        }

        //From coordinates X and Y (from trace file), we build and output matrix accordinly. 
        coordToViewPort(matrix, output, mleni, mlenj, x, y, 1, 1);
        
        //We do the same to build the adjancy and outside representation. 
        viewPortToAdjacency(matrix, output, mleni, mlenj, 1, viewport, &len_viewport, adjacency, &len_adjacency, outside, &len_outside);
        
        current_index = (i - 1) % AVERAGE_WINDOW_SIZE;
    
        int jTile;
       
        double local_avg = 0;

        if (i > 1){
          
          refOut = maxbitrate;
          
          //lower bitrate -> if this is the case, then all zone are in 720p
          if (refOut < 2000000){
             
             if(strcmp(resolutionVp, "720") != 0){
                    switchVp++;
                    resolutionVp[0] = '\0';
                    strcat(resolutionVp, "720");
                    strcat(resolutionVp, "\0");
                }
                if(strcmp(resolutionAdj, "720") != 0){
                    switchAdj++;
                    resolutionAdj[0] = '\0';
                    strcat(resolutionAdj, "720");
                    strcat(resolutionAdj, "\0");
                }
                if(strcmp(resolutionOut, "720") != 0){
                    switchOut++;
                    resolutionOut[0] = '\0';
                  strcat(resolutionOut, "720");
                  strcat(resolutionOut, "\0");
                }
                c1++;
                
            
          }else if(refOut < 2120000){
               //vp: 1080p; adj: 720; out: 720 
              if(strcmp(resolutionVp, "1080") != 0){
                    switchVp++;
                    resolutionVp[0] = '\0';
                    strcat(resolutionVp, "1080");
                    strcat(resolutionVp, "\0");
                }
                if(strcmp(resolutionAdj, "720") != 0){
                    switchAdj++;
                    resolutionAdj[0] = '\0';
                    strcat(resolutionAdj, "720");
                    strcat(resolutionAdj, "\0");
                }
                if(strcmp(resolutionOut, "720") != 0){
                    switchOut++;
                    resolutionOut[0] = '\0';
                  strcat(resolutionOut, "720");
                  strcat(resolutionOut, "\0");
                }
                c2++;
                
          }else if(refOut < 2370000){
              //vp: 4k; adj: 720; out: 720   
              if(strcmp(resolutionVp, "4K") != 0){
                    switchVp++;
                    resolutionVp[0] = '\0';
                    strcat(resolutionVp, "4K");
                    strcat(resolutionVp, "\0");
                }
                if(strcmp(resolutionAdj, "720") != 0){
                    switchAdj++;
                    resolutionAdj[0] = '\0';
                    strcat(resolutionAdj, "720");
                    strcat(resolutionAdj, "\0");
                }
                if(strcmp(resolutionOut, "720") != 0){
                    switchOut++;
                    resolutionOut[0] = '\0';
                  strcat(resolutionOut, "720");
                  strcat(resolutionOut, "\0");
                }
                c3++;
                
          }else if(refOut < 3120000){
              //vp: 4k; adj: 1080; out: 720                
              if(strcmp(resolutionVp, "4K") != 0){
                    switchVp++;
                    resolutionVp[0] = '\0';
                    strcat(resolutionVp, "4K");
                    strcat(resolutionVp, "\0");
                }
                if(strcmp(resolutionAdj, "1080") != 0){
                    switchAdj++;
                    resolutionAdj[0] = '\0';
                    strcat(resolutionAdj, "1080");
                    strcat(resolutionAdj, "\0");
                }
                if(strcmp(resolutionOut, "720") != 0){
                    switchOut++;
                    resolutionOut[0] = '\0';
                  strcat(resolutionOut, "720");
                  strcat(resolutionOut, "\0");
                }
                c4++;
                 
          }else if(refOut < 3840000){
              //vp: 4k; adj: 4k; out: 720   
              if(strcmp(resolutionVp, "4K") != 0){
                    switchVp++;
                    resolutionVp[0] = '\0';
                    strcat(resolutionVp, "4K");
                    strcat(resolutionVp, "\0");
                }
                if(strcmp(resolutionAdj, "4K") != 0){
                    switchAdj++;
                    resolutionAdj[0] = '\0';
                    strcat(resolutionAdj, "4K");
                    strcat(resolutionAdj, "\0");
                }
                if(strcmp(resolutionOut, "720") != 0){
                    switchOut++;
                    resolutionOut[0] = '\0';
                  strcat(resolutionOut, "720");
                  strcat(resolutionOut, "\0");
                }
                c5++;
                
          }else if(refOut < 6000000){
              //vp: 4k; adj: 4k; out: 1080   
              if(strcmp(resolutionVp, "4K") != 0){
                    switchVp++;
                    resolutionVp[0] = '\0';
                    strcat(resolutionVp, "4K");
                    strcat(resolutionVp, "\0");
                }
                if(strcmp(resolutionAdj, "4K") != 0){
                    switchAdj++;
                    resolutionAdj[0] = '\0';
                    strcat(resolutionAdj, "4K");
                    strcat(resolutionAdj, "\0");
                }
                if(strcmp(resolutionOut, "1080") != 0){
                    switchOut++;
                    resolutionOut[0] = '\0';
                  strcat(resolutionOut, "1080");
                  strcat(resolutionOut, "\0");
                }
                c6++;
                
          }else{
              //vp: 4k; adj: 4k; out: 4k   
              if(strcmp(resolutionVp, "4K") != 0){
                    switchVp++;
                    resolutionVp[0] = '\0';
                    strcat(resolutionVp, "4K");
                    strcat(resolutionVp, "\0");
                }
                if(strcmp(resolutionAdj, "4K") != 0){
                    switchAdj++;
                    resolutionAdj[0] = '\0';
                    strcat(resolutionAdj, "4K");
                    strcat(resolutionAdj, "\0");
                }
                if(strcmp(resolutionOut, "4K") != 0){
                    switchOut++;
                    resolutionOut[0] = '\0';
                  strcat(resolutionOut, "4K");
                  strcat(resolutionOut, "\0");
                }
                c7++;
               
          }

        }

        maxbitrate = 0;

        if(strcmp(resolutionVp, "720") == 0){
            cont720z1 += len_viewport;
        }else if (strcmp(resolutionVp, "1080") == 0){
            cont1080z1 += len_viewport;
        }else{
            cont4kz1 += len_viewport;
        }  

        refVp = 0;

        //Download firts tiles from viewport
        for (jTile = 0; jTile < len_viewport; jTile++){
            
            tiles_count_global++;
            
            //build URL
            buildURL(url, destination, video_id, mleni, mlenj, resolutionVp, viewport[jTile], i);
            /* Perform the request, res will get the return code */

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

            res = curl_easy_perform(curl);

            gettimeofday(&last_load, NULL);
            elapsed = tvdiff_secs(last_load, beg);

            curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &volume);
            curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ttime);
            
            bitrate = (8.0 * volume) / ttime;

            if (bitrate > maxbitrate) maxbitrate = bitrate; 
           
            avgbitratez1 += (double) bitrate;
            
            contz1++;

            avg_window[current_index] = bitrate;
            int local_avg_window_size = tiles_count_global < AVERAGE_WINDOW_SIZE ? tiles_count_global : AVERAGE_WINDOW_SIZE;
        
            double local_sum = 0.0; 
            for (j = 0; j < local_avg_window_size; j++){
               local_sum += avg_window[j];
            }
            
            local_avg = local_sum / (double) local_avg_window_size;
        
            refVp = local_avg;// < bitrate ? local_avg : bitrate);

            fprintf(flog, "%lf;%d;%s;%lf;%lf;%lf;%lf;%lf;vp\n", elapsed, i, resolutionVp, refVp, local_avg, bitrate, volume, ttime);
            fflush(flog);
        
        }
      
        local_avg = 0;
        refAdj = 0;
        

        if(strcmp(resolutionAdj, "720") == 0){
            cont720z2 += len_adjacency;
        }else if (strcmp(resolutionAdj, "1080") == 0){
            cont1080z2 += len_adjacency;
        }else{
            cont4kz2 += len_adjacency;
        }


        int div = len_adjacency / NUM_THREADS; 
        int lower = 0, upper = div; 

        int auxNum = (NUM_THREADS < len_adjacency ? NUM_THREADS : len_adjacency);

        //donwload tiles from adjacency (or zone Z2)
        for(int cont_thread = 0; cont_thread < auxNum; cont_thread++){
            if(cont_thread == NUM_THREADS - 1){ 
              
              upper = len_adjacency-1; 
              
              struct arg_struct *args = malloc(sizeof(struct arg_struct));
              args->i = i;
              args->lower_limit = lower;
              args->upper_limit = upper;
              
              pthread_create(&threadsId[cont_thread], NULL, downloadZ2, (void *)args);
          
          }else{
          
              struct arg_struct *args = malloc(sizeof(struct arg_struct));
              args->i = i;
              args->lower_limit = lower;
              args->upper_limit = upper;
              pthread_create(&threadsId[cont_thread], NULL, downloadZ2, (void *)args);
              lower = upper + 1;
              upper += div; 
          }
    
        }
        
        //wait to download all tiles in zone z2
        for(int cont_thread = 0; cont_thread < auxNum; cont_thread++){
            pthread_join(threadsId[cont_thread], NULL);
        }

        
        local_avg = 0;
        

        if(strcmp(resolutionOut, "720") == 0){
            cont720z3 += len_outside;
        }else if (strcmp(resolutionOut, "1080") == 0){
            cont1080z3 += len_outside;
        }else{
            cont4kz3 += len_outside;
        }

        
        div = len_outside / NUM_THREADS; 
        lower = 0, upper = div; 

        auxNum = (NUM_THREADS < len_outside ? NUM_THREADS : len_outside);

        upper = len_outside-1; 
        
        //donwload tiles from outside (or zone Z3)
        for(int cont_thread = 0; cont_thread < auxNum; cont_thread++){
            if(cont_thread == NUM_THREADS - 1){ 
              
              upper = len_outside-1; 
              struct arg_struct *args = malloc(sizeof(struct arg_struct));
              args->i = i;
              args->lower_limit = lower;
              args->upper_limit = upper;
              
              pthread_create(&threadsId[cont_thread], NULL, downloadZ3, (void *)args);
          
          }else{
          
              struct arg_struct *args = malloc(sizeof(struct arg_struct));
              args->i = i;
              args->lower_limit = lower;
              args->upper_limit = upper;
              pthread_create(&threadsId[cont_thread], NULL, downloadZ3, (void *)args);
              lower = upper + 1;
              upper += div; 
          }
    
        }

        //wait to download all tiles in zone z3
        for(int cont_thread = 0; cont_thread < auxNum; cont_thread++){
            pthread_join(threadsId[cont_thread], NULL);
        }

     
        
        if (i == 1){
        
           startup_time = elapsed;
           buffer_time = SEGMENT_TIME;
        
        }else{

          
          time_from_last_load = tvdiff_secs(last_load, former_last_load);
          buffer_time = buffer_time - time_from_last_load;

          if (buffer_time < 0){
                stall_count++;
                stall_len = stall_len - buffer_time;
                buffer_time = SEGMENT_TIME;
           }else{
               buffer_time = buffer_time + SEGMENT_TIME;
           }

        }
        
        former_last_load = last_load;
        
        if (buffer_time > BUFFER_LIMIT){
           sleep(INTERVAL); //Interval = segment length (in seconds)
        }

        //increment segment count
        i++;

        if (tvdiff_secs(former_last_load, beg) > timelimit){
          break;
        }
        
    }

    gettimeofday(&end, NULL);
    fprintf(flog, "%lu.%06lu;%.6lf;%.6lf;%.6lf;%d;%d;%d;%d\n", end.tv_sec, end.tv_usec, tvdiff_secs(end, beg), startup_time, stall_len, stall_count, switchVp, switchAdj,switchOut);
    printf("%lu.%06lu;%.6lf;%.6lf;%.6lf;%d;%d;%d;%d\n", end.tv_sec, end.tv_usec, tvdiff_secs(end, beg), startup_time, stall_len, stall_count, switchVp, switchAdj, switchOut);
    
    printf("%lf %d\n",avgbitratez1, contz1 );
    
    fprintf(flog, "|TBD|: %d %d %d %d %d %d %d\n", c1,c2,c3,c4,c5,c6,c7);
    fprintf(flog, "tiles_720_z1: %d\n", cont720z1);
    fprintf(flog, "tiles_1080_z1: %d\n", cont1080z1);
    fprintf(flog, "tiles_4k_z1: %d\n", cont4kz1);

    fprintf(flog, "tiles_720_z2: %d\n", cont720z2);
    fprintf(flog, "tiles_1080_z2: %d\n", cont1080z2);
    fprintf(flog, "tiles_4k_z2: %d\n", cont4kz2);

    fprintf(flog, "tiles_720_z3: %d\n", cont720z3);
    fprintf(flog, "tiles_1080_z3: %d\n", cont1080z3);
    fprintf(flog, "tiles_4k_z3: %d\n", cont4kz3);


    fprintf(flog, "avgbitratez1: %.6lf\n", (double) avgbitratez1 / (double) (contz1 + stall_count_vp));
    fprintf(flog, "avgbitratez2: %.6lf\n", (double) avgbitratez2 / (double) (contz2 + stall_count_adj));
    fprintf(flog, "avgbitratez3: %.6lf\n", (double) avgbitratez3 / (double) (contz3 + stall_count_out));

    fprintf(flog, "qualityswitch1: %d\n", switchVp);
    fprintf(flog, "qualityswitch2: %d\n", switchAdj);
    fprintf(flog, "qualityswitch3: %d\n", switchOut);

    fprintf(flog, "stall: %.6lf\n", stall_len);
    fprintf(flog, "startuptime: %.6lf\n", startup_time);


    free(viewport);
    free(adjacency);
    free(outside);

    fclose(f);
    fclose(ftrace);
    fclose(flog);
    free(avg_window);
    /* always cleanup */
    curl_easy_cleanup(curl);

    /* Deallocating memory */
    for(k = 0; k < mleni; k++){
      free(matrix[k]);
    }

    free(matrix);


  }else{
      printf("ERROR: %s\n", strerror(errno));
      return(-1);
  }
  return 0;
}
