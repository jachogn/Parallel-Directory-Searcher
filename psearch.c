/**
 * psearch.c
 *
 * Performs a parallel directory search over the file system.
 *
 * Author: Joshua Chong
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <limits.h>

void print_usage(char *argv[]);
void *search_thread(void *opts);

// global variables
sem_t thread_semaphore;
//current working directory
char cwd[1024];
//global variable for directory
char* dir;
//variable for maximum number of threads
int max_threads;
//boolean to check if option -e is used
bool exactname;

//called in main using the arguments in argv starting at the index optind
//input : opts = the string that is used to search the directory by
void *search_thread(void *opts)
{
    char *arg = (char*) opts;
    searchdir(dir,arg);
    sem_post(&thread_semaphore);
    return 0;
}
//Searches through a directory recursively for files that contain a substring, or match a string exactly
// Inputs: path = the directory to start searching at
// name = the string that we use to compare the names of files, in order to check if the name of the file contains the string "name" as a substring, or matches the string
// "name" exactly
//Outputs: prints the names of the files that match the string "name"
int searchdir(char *path, char* name) {
    //going through name and changing all the characters to lowercase
    for(int i = 0; name[i];i++){
    name[i] = tolower(name[i]);
    }
    //variable for a subdirectory
    char* subdir;
    //variable for directory where file that matches the string "name" is found
    char* direct;
    DIR *directory;
    if ((directory = opendir(path)) == NULL) {
        perror("opendir");
        return 1;
    }
    struct dirent *entry;
    char buf[PATH_MAX + 1];
    while ((entry = readdir(directory)) != NULL) {
        //Checks to see if the file is a directory, and checks if the file is not the current directory or parent directory
        if((entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)){
            subdir = malloc(strlen(path) + strlen(entry->d_name) + 2);
            strcpy(subdir, path);
            strcat(subdir, "/");
            strcat(subdir, entry->d_name);
            //calls the function on the subdirectories of the current directory
            searchdir(subdir, name);
            free(subdir);
        }
        //checks if command line option "-e" is used
        else{
            if(exactname == true){
            //compares to see if the name of the file matches the string "name" exactly
             if(strcmp(entry->d_name, name) == 0){
             //allocate memory for string direct
                direct = malloc(strlen(dir) + strlen(entry->d_name) + 2);
                //copies the current directory to String variable direct    
                strcpy(direct, dir);
                //adds "/filename" to the end of the String direct
                strcat(direct, "/");
                strcat(direct, entry->d_name);  
                realpath(direct, buf);
                printf ("[%s]\n", buf);
            }
        }
        //if command line option "-e" is not used
         else{
            //Checks to see if "name" is a substring of the file name
            if(strstr(entry->d_name, name) != NULL){
                //allocate memory for string direct
                direct = malloc(strlen(dir) + strlen(entry->d_name) + 2);
                //copies the current directory to String variable direct    
                strcpy(direct, dir);
                //adds "/filename" to the end of the String direct
                strcat(direct, "/");
                strcat(direct, entry->d_name); 
                realpath(direct, buf);
                printf ("[%s]\n", buf);
            }
        }
        
    }

    closedir(directory);

    return 0;
}

void print_usage(char *argv[])
{
    printf("Usage: %s [-eh] [-d directory] [-t threads] "
            "search_term1 search_term2 ... search_termN\n" , argv[0]);
    printf("\n");
    printf("Options:\n"
           "    * -d directory    specify start directory (default: CWD)\n"
           "    * -e              print exact name matches only\n"
           "    * -h              show usage information\n"
           "    * -t threads      set maximum threads (default: # procs)\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    int c;
    opterr = 0;
    bool c_flag = false;
    max_threads = get_nprocs();
    dir = getcwd(cwd, sizeof(cwd));
    /* Handle command line options */
    while ((c = getopt(argc, argv, "d:eht:")) != -1) {
        //first checks to see if command line option "-h" is used, if it is
        //we print out usage information and nothing else
        switch (c) {
            case 'h':
            c_flag = true;
            print_usage(argv);
            break;
            case 'd':
            if (c_flag){
                print_usage(argv);
                break;
            }
            //changes the value of dir to the directory the user inputs, used to search within that directory for matching file names
            dir = optarg;
            break;
            case 't':
            if(c_flag){
                print_usage(argv);
                break;
            }
            //set the value of max_threads to what the user inputs
            if(isdigit(atoi(optarg))){
                max_threads = atoi(optarg);
                break;
            }
            else{
                printf("%s\n", "Please enter integer values only!");
                break
            }
            case 'e':
            if(c_flag){
                print_usage(argv);
                break;
            }
            //sets boolean value of exactname to true, which is used in the search function
            exactname = true;
            break;
        }
    }
    sem_init(&thread_semaphore, 0, max_threads);
    pthread_t *pthreads;
    //allocating memory for pthreads
    pthreads = malloc(max_threads * sizeof(pthread_t));
    //# for pthread
    int threadnumber = 0;
    for (int i = optind; i < argc; i++) { 
        sem_wait(&thread_semaphore);
        //calls the search_thread method, which calls the searchdir method and passes the strings from argv[optind] to argv[argc -1], which are
        //used as the variable "name" in the method searchdir. These variables are used to check if the file contains the string as a substring or matches
        //the string exactly.
        pthread_create(&pthreads[threadnumber], NULL, search_thread, (void *) argv[i]);
        threadnumber++;
        }
    //Joining all pthreads 
    for(int i = 0; i < threadnumber; i++){
        pthread_join(pthreads[i], NULL);
    }

    return 0;
}