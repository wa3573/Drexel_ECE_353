/* Reference code that recursively searches the file system for a user-specified string. 
 * 
 * Author: Naga Kandasamy
 * Date: 17 August 2018
 *
 * Last update:
 * Author: William Anderson
 * Data: 25 August 2018
 *
 * Compile the code as follows: gcc -o mini_grep min_grep.c queue_utils.c -std=c99 -lpthread - Wall
 *
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include "queue.h"

/* Max elements for elements[] array in to add to each thread, would use another queue instead,
 * in future */
#define MAX_ELEMENTS_Q 4096

typedef struct args_for_thread_t
{
	int threadID; // thread ID
	queue_element_t* elements[MAX_ELEMENTS_Q];
	int num_elements;
	char* search_string;
} ARGS_FOR_THREAD;

typedef struct SHARED_t
{
	queue_t* queue_files;
	int count;

} SHARED_t;

int serial_search(char **);
int parallel_search_static(char **);
int parallel_search_dynamic(char **);

/* Set VERBOSE to "true" to enable verbose output, or use last command line argument*/
static volatile bool VERBOSE = true;

static int* RESULTS;
pthread_mutex_t mutex_shared = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_file = PTHREAD_MUTEX_INITIALIZER;
static SHARED_t SHARED;

void SHARED_init()
{
	pthread_mutex_lock(&mutex_shared);

	SHARED.queue_files = create_queue();

	pthread_mutex_unlock(&mutex_shared);
}

void SHARED_insert_file_element(queue_element_t* el)
{
	pthread_mutex_lock(&mutex_shared);
	insert_element(SHARED.queue_files, el);
	pthread_mutex_unlock(&mutex_shared);
}

int /* Serial search of the file system starting from the specified path name. */
serial_search(char **argv)
{
	int num_occurrences = 0;
	queue_element_t *element, *new_element;
	struct stat file_stats;
	int status;
	DIR *directory = NULL;
	struct dirent *result = NULL;
	struct dirent *entry = (struct dirent *)malloc(
			sizeof(struct dirent) + MAX_LENGTH);

	queue_t *queue = create_queue(); /* Create and initialize the queue data structure. */
	element = (queue_element_t *)malloc(sizeof(queue_element_t));
	if (element == NULL)
	{
		perror("malloc");
		exit( EXIT_FAILURE);
	}

	strcpy(element->path_name, argv[2]); /* Copy the initial path name */
	element->next = NULL;
	insert_element(queue, element); /* Insert the initial path name into the queue. */

	while (queue->head != NULL)
	{ /* While there is work in the queue, process it. */
		queue_element_t *element = remove_element(queue);

		/* Obtain information about the file. */
		status = lstat(element->path_name, &file_stats);
		if (status == -1)
		{
			printf("Error obtaining stats for %s \n", element->path_name);
			free((void *)element);
			continue;
		}

		if (S_ISLNK(file_stats.st_mode))
		{ 	/* Ignore symbolic links. */
		} else if (S_ISDIR(file_stats.st_mode))
		{ 	/* If directory, descend in and post work to queue. */
			if (VERBOSE)
			{
				printf("%s is a directory. \n", element->path_name);
			}
			directory = opendir(element->path_name);
			if (directory == NULL)
			{
				printf("Unable to open directory %s \n", element->path_name);
				continue;
			}

			while (1)
			{
				status = readdir_r(directory, entry, &result); /* Read directory entry. */
				if (status != 0)
				{
					printf("Unable to read directory %s \n",
							element->path_name);
					break;
				}
				if (result == NULL) /* End of directory. */
					break;

				if (strcmp(entry->d_name, ".") == 0) /* Ignore the "." and ".." entries. */
					continue;

				if (strcmp(entry->d_name, "..") == 0)
					continue;

				/* Insert this directory entry in the queue. */
				new_element = (queue_element_t *)malloc(
						sizeof(queue_element_t));
				if (new_element == NULL)
				{
					perror("malloc");
					exit(EXIT_FAILURE);
				}

				/* Construct the full path name for the directory item stored in entry. */
				strcpy(new_element->path_name, element->path_name);
				strcat(new_element->path_name, "/");
				strcat(new_element->path_name, entry->d_name);
				insert_element(queue, new_element);
			}

			closedir(directory);
		} else if (S_ISREG(file_stats.st_mode))
		{ 	/* Directory entry is a regular file. */
			if (VERBOSE)
			{
				printf("%s is a regular file. \n", element->path_name);
			}
			FILE *file_to_search;
			char buffer[MAX_LENGTH];
			char *bufptr, *searchptr, *tokenptr;

			/* Search the file for the search string provided as the command-line argument. */
			file_to_search = fopen(element->path_name, "r");
			if (file_to_search == NULL)
			{
				printf("Unable to open file %s \n", element->path_name);
				continue;
			} else
			{
				while (1)
				{
					bufptr = fgets(buffer, sizeof(buffer), file_to_search); /* Read in a line from the file. */
					if (bufptr == NULL)
					{
						if (feof(file_to_search))
							break;
						if (ferror(file_to_search))
						{
							printf("Error reading file %s \n",
									element->path_name);
							break;
						}
					}
					/* Break up line into tokens and search each token. */
					tokenptr = strtok(buffer, " ,.-");
					while (tokenptr != NULL)
					{
						searchptr = strstr(tokenptr, argv[1]);
						if (searchptr != NULL)
						{
							if (VERBOSE)
							{
								printf(
										"Found string %s within \
                                    file %s. \n",
										argv[1], element->path_name);
							}
							num_occurrences++;
						}
						tokenptr = strtok(NULL, " ,.-"); /* Get next token from the line. */
					}
				}
			}

			fclose(file_to_search);
		} else
		{
			if (VERBOSE)
			{
				printf("%s is of type other. \n", element->path_name);
			}
		}

		free((void *)element);
	}

	return num_occurrences;
}

unsigned int round_up(unsigned int dividend, unsigned int divisor)
{
	return (dividend + (divisor - 1)) / divisor;
}

void* parallel_search_static_thread(void* this_arg)
{
	ARGS_FOR_THREAD* args_for_me = (ARGS_FOR_THREAD *)this_arg; // Typecast the argument passed to this function to the appropriate type

	int thread_id = args_for_me->threadID;
	int num_elements = args_for_me->num_elements;
	char* search_string = args_for_me->search_string;
	queue_t* queue;
	queue_element_t** input_elements = args_for_me->elements;
	queue_element_t* element, *new_element;
	struct stat file_stats;
	int status;
	DIR* directory = NULL;
	struct dirent* result = NULL;
	struct dirent* entry = (struct dirent*)malloc(
			sizeof(struct dirent) + MAX_LENGTH);
	int i;
	int num_occurrences = 0;

	/* internal queue */
	queue = create_queue(); /* Create and initialize the queue data structure. */
	element = (queue_element_t *)malloc(sizeof(queue_element_t));
	if (element == NULL)
	{
		perror("malloc");
		exit( EXIT_FAILURE);
	}

	/* Add input elements to queue */
	for (i = 0; i < num_elements; i++)
	{
		insert_element(queue, input_elements[i]);
	}

	while (queue->head != NULL)
	{ /* While there is work in the queue, process it. */

		element = remove_element(queue);

		/* Obtain information about the file. */
		status = lstat(element->path_name, &file_stats);
		if (status == -1)
		{
			printf("Thread %d: Error obtaining stats for %s \n", thread_id,
					element->path_name);
			free((void*)element);
			continue;
		}

		if (S_ISLNK(file_stats.st_mode))
		{ /* Ignore symbolic links. */
		} else if (S_ISDIR(file_stats.st_mode))
		{ /* If directory, descend in and post work to queue. */
			if (VERBOSE)
			{
				printf("Thread %d: %s is a directory. \n", thread_id,
						element->path_name);
			}
			directory = opendir(element->path_name);
			if (directory == NULL)
			{
				printf("Thread %d: Unable to open directory %s \n", thread_id,
						element->path_name);
				continue;
			}

			while (1)
			{
				status = readdir_r(directory, entry, &result); /* Read directory entry. */
				if (status != 0)
				{
					printf("Thread %d: Unable to read directory %s \n",
							thread_id, element->path_name);
					break;
				}
				if (result == NULL) /* End of directory. */
					break;

				if (strcmp(entry->d_name, ".") == 0) /* Ignore the "." and ".." entries. */
					continue;

				if (strcmp(entry->d_name, "..") == 0)
					continue;

				/* Insert this directory entry in the queue. */
				new_element = (queue_element_t *)malloc(
						sizeof(queue_element_t));
				if (new_element == NULL)
				{
					perror("malloc");
					exit(EXIT_FAILURE);
				}

				/* Construct the full path name for the directory item stored in entry. */
				strcpy(new_element->path_name, element->path_name);
				strcat(new_element->path_name, "/");
				strcat(new_element->path_name, entry->d_name);
				insert_element(queue, new_element);
			}

			closedir(directory);
		} else if (S_ISREG(file_stats.st_mode))
		{ 	/* Directory entry is a regular file. */

			if (VERBOSE)
			{
				printf("Thread %d: %s is a regular file. \n", thread_id,
						element->path_name);
			}

			FILE *file_to_search;
			char buffer[MAX_LENGTH];
			char *bufptr, *searchptr, *tokenptr;

			/* Search the file for the search string provided as the command-line argument. */
			file_to_search = fopen(element->path_name, "r");

			if (file_to_search == NULL)
			{
				printf("Thread %d: Unable to open file %s \n", thread_id,
						element->path_name);
				continue;
			} else
			{
				/* Note: strtok() is not thread safe:
				 * strtok() is prone to data races when used in concurrent threads. This causes
				 * inaccurate counts when run on xunil. The remediation is to use strtok_r(), this
				 * solves the issue and provides a similar syntax.
				 */
				while (1)
				{
					bufptr = fgets(buffer, sizeof(buffer), file_to_search); /* Read in a line from the file. */
					if (bufptr == NULL)
					{
						if (feof(file_to_search))
							break;
						if (ferror(file_to_search))
						{
							printf("Thread %d: Error reading file %s \n",
									thread_id, element->path_name);
							break;
						}
					}

					/* Break up line into tokens and search each token. */
					char* saveptr;	// save pointer for strtok_r()
					tokenptr = strtok_r(buffer, " ,.-", &saveptr);
					while (tokenptr != NULL)
					{
						searchptr = strstr(tokenptr, search_string);
						if (searchptr != NULL)
						{
							if (VERBOSE)
							{
								printf(
										"Thread %d: Found string %s within \
									file %s. \n",
										thread_id, search_string,
										element->path_name);
							}

							num_occurrences++;
						}
						tokenptr = strtok_r(NULL, " ,.-", &saveptr);
					}
				}
			}

			fclose(file_to_search);

		} else
		{
			if (VERBOSE)
			{
				printf("Thread %d: %s is of type other. \n", thread_id,
						element->path_name);
			}
		}

		free((void *)element);

	}

	RESULTS[thread_id] = num_occurrences;

	return ((void *)0);
}

int /* Parallel search with static load balancing accross threads. */
parallel_search_static(char** argv)
{
	int num_occurrences = 0;

	const int NUM_THREADS = atoi(argv[3]);
	pthread_t worker_thread[NUM_THREADS];
	RESULTS = (int*)malloc(sizeof(int) * NUM_THREADS);
	int* RESULTS_original = RESULTS;
	ARGS_FOR_THREAD* args_for_thread;
	queue_element_t* element, *new_element;
	struct stat file_stats;
	int status;
	DIR* directory = NULL;
	struct dirent *result = NULL;
	struct dirent *entry = (struct dirent *)malloc(
			sizeof(struct dirent) + MAX_LENGTH);
	int i;

	queue_t *queue = create_queue(); /* Create and initialize the queue data structure. */
	element = (queue_element_t*)malloc(sizeof(queue_element_t));
	if (element == NULL)
	{
		perror("malloc");
		exit( EXIT_FAILURE);
	}

	strcpy(element->path_name, argv[2]); /* Copy the initial path name */
	element->next = NULL;
	insert_element(queue, element); /* Insert the initial path name into the queue. */

	element = remove_element(queue);

	/* Obtain information about the file. */
	status = lstat(element->path_name, &file_stats);
	if (status == -1)
	{
		printf("Error obtaining stats for %s \n", element->path_name);
		free((void *)element);
	}

	/* Add all items in directory to queue */
	if (S_ISDIR(file_stats.st_mode))
	{
		/* If directory, descend in and post work to queue. */
		if (VERBOSE)
		{
			printf("%s is a directory. \n", element->path_name);
		}

		directory = opendir(element->path_name);
		if (directory == NULL)
		{
			printf("Unable to open directory %s \n", element->path_name);
		}

		while (1)
		{
			status = readdir_r(directory, entry, &result); /* Read directory entry. */
			if (status != 0)
			{
				printf("Unable to read directory %s \n", element->path_name);
				break;
			}
			if (result == NULL) /* End of directory. */
				break;

			if (strcmp(entry->d_name, ".") == 0) /* Ignore the "." and ".." entries. */
				continue;

			if (strcmp(entry->d_name, "..") == 0)
				continue;

			/* Insert this directory entry in the queue. */
			new_element = (queue_element_t *)malloc(sizeof(queue_element_t));
			if (new_element == NULL)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			/* Construct the full path name for the directory item stored in entry. */
			strcpy(new_element->path_name, element->path_name);
			strcat(new_element->path_name, "/");
			strcat(new_element->path_name, entry->d_name);
			insert_element(queue, new_element);
		}

		closedir(directory);
	}

	int num_el = num_elements(queue);
	int items_per_thread = round_up(num_el, NUM_THREADS);
	if (VERBOSE)
	{
		printf("Items per thread = %d\n", items_per_thread);
	}
	int num_el_to_threads = 0;
	int num_el_to_thread = 0;
	int j;

	if (VERBOSE)
	{
		printf("Main thread: creating %d worker threads \n", NUM_THREADS);
	}

	for (i = 0; i < NUM_THREADS; i++)
	{
		num_el_to_thread = 0; 	// Number of elements added to this thread
		/* Allocate memory for the structure that will be used to pack the arguments */
		args_for_thread = (ARGS_FOR_THREAD *)malloc(sizeof(ARGS_FOR_THREAD));
		args_for_thread->search_string = (char*)malloc(sizeof(char) * 128);
		args_for_thread->threadID = i;

		/* Add calculated "num_el" elements to each thread */
		for (j = 0; j < items_per_thread; j++)
		{
			if (num_el_to_threads < num_el)
			{
				args_for_thread->elements[j] = remove_element(queue);
				num_el_to_thread++;
				num_el_to_threads++;
			} else
			{
				break;
			}
		}

		args_for_thread->num_elements = num_el_to_thread;

		/* Copy the search string into the struct */
		strcpy(args_for_thread->search_string, argv[1]);
		if (VERBOSE)
		{
			printf("creating thread #%d... \n", i);
		}
		if ((pthread_create(&worker_thread[i], NULL,
				parallel_search_static_thread, (void *)args_for_thread)) != 0)
		{
			printf("Cannot create thread \n");
			exit(0);
		}
	}

	/* Wait for all the worker threads to finish */
	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(worker_thread[i], NULL);

	/* Sum occurrences */
	for (i = 0; i < NUM_THREADS; i++)
	{
		num_occurrences = num_occurrences + RESULTS[i];
	}

	free(RESULTS_original);

	return num_occurrences;
}

/* For each file in the shared queue, search for search_string */
void* parallel_search_dynamic_search_thread(void* this_arg)
{
	ARGS_FOR_THREAD* args_for_me = (ARGS_FOR_THREAD *)this_arg; // Typecast the argument passed to this function to the appropriate type
	int thread_id = args_for_me->threadID;
	int num_elements = args_for_me->num_elements;
	char* search_string = args_for_me->search_string;
	queue_t* queue;
	queue_element_t** input_elements = args_for_me->elements;
	queue_element_t* element;
	struct stat file_stats;
	int status;
	int i;
	int num_occurrences = 0;

	/* internal queue */
	queue = create_queue(); /* Create and initialize the queue data structure. */

	/* Add (seed) input elements into queue */
	for (i = 0; i < num_elements; i++)
	{
		insert_element(queue, input_elements[i]);
	}

	while (queue->head != NULL)
	{ /* While there is work in the queue, process it. */

		element = remove_element(queue);

		/* Obtain information about the file. */
		status = lstat(element->path_name, &file_stats);
		if (status == -1)
		{
			printf("Thread %d: Error obtaining stats for %s \n", thread_id,
					element->path_name);
			free((void *)element);
			continue;
		}

		if (S_ISREG(file_stats.st_mode))
		{ /* Directory entry is a regular file. */
			if (VERBOSE)
			{
				printf("Thread %d: %s is a regular file. \n", thread_id,
						element->path_name);
			}

			FILE *file_to_search;
			char buffer[MAX_LENGTH];
			char *bufptr, *searchptr, *tokenptr;

			/* Search the file for the search string provided as the command-line argument. */
			file_to_search = fopen(element->path_name, "r");
			if (file_to_search == NULL)
			{
				printf("Thread %d: Unable to open file %s \n", thread_id,
						element->path_name);
				continue;
			} else
			{
				while (1)
				{
					bufptr = fgets(buffer, sizeof(buffer), file_to_search); /* Read in a line from the file. */
					if (bufptr == NULL)
					{
						if (feof(file_to_search))
							break;
						if (ferror(file_to_search))
						{
							printf("Thread %d: Error reading file %s \n",
									thread_id, element->path_name);
							break;
						}
					}
					/* Break up line into tokens and search each token. */
					char* saveptr;
					tokenptr = strtok_r(buffer, " ,.-", &saveptr);
					while (tokenptr != NULL)
					{
						searchptr = strstr(tokenptr, search_string);
						if (searchptr != NULL)
						{
							if (VERBOSE)
							{
								printf(
										"Thread %d: Found string %s within \
									file %s. \n",
										thread_id, search_string,
										element->path_name);
							}

							num_occurrences++;
						}
						tokenptr = strtok_r(NULL, " ,.-", &saveptr);
					}
				}
			}

			fclose(file_to_search);
		} else
		{
			if (VERBOSE)
			{
				printf("Thread %d: %s is of type other. \n", thread_id,
						element->path_name);
			}
		}

		free((void *)element);
	}

	RESULTS[thread_id] = num_occurrences;

	if (VERBOSE)
	{
		printf("Thread %d: finished! \n", thread_id);
	}

	return ((void*)0);
}

/* Descend breadth-first and add all regular files to the shared queue */
void* parallel_search_dynamic_files_thread(void* this_arg)
{
	ARGS_FOR_THREAD* args_for_me = (ARGS_FOR_THREAD *)this_arg; // Typecast the argument passed to this function to the appropriate type

	int thread_id = args_for_me->threadID;
	int num_elements = args_for_me->num_elements;
	queue_t* queue;
	queue_element_t** input_elements = args_for_me->elements;
	queue_element_t* element, *new_element;
	struct stat file_stats;
	int status;
	DIR* directory = NULL;
	struct dirent* result = NULL;
	struct dirent* entry = (struct dirent*)malloc(
			sizeof(struct dirent) + MAX_LENGTH);
	int i;

	/* internal queue */
	queue = create_queue(); /* Create and initialize the queue data structure. */
	element = (queue_element_t *)malloc(sizeof(queue_element_t));
	if (element == NULL)
	{
		perror("malloc");
		exit( EXIT_FAILURE);
	}

	/* Add (seed) input elements into queue */
	for (i = 0; i < num_elements; i++)
	{
		insert_element(queue, input_elements[i]);
	}

	while (queue->head != NULL)
	{ /* While there is work in the queue, process it. */

		element = remove_element(queue);

		/* Obtain information about the file. */
		status = lstat(element->path_name, &file_stats);
		if (status == -1)
		{
			printf("Thread %d: Error obtaining stats for %s \n", thread_id,
					element->path_name);
			free((void *)element);
			continue;
		}

		if (S_ISLNK(file_stats.st_mode))
		{ /* Ignore symbolic links. */
		} else if (S_ISDIR(file_stats.st_mode))
		{ /* If directory, descend in and post work to queue. */
			if (VERBOSE)
			{
				printf("Thread %d: %s is a directory. \n", thread_id,
						element->path_name);
			}
			directory = opendir(element->path_name);
			if (directory == NULL)
			{
				printf("Thread %d: Unable to open directory %s \n", thread_id,
						element->path_name);
				continue;
			}

			while (1)
			{
				status = readdir_r(directory, entry, &result); /* Read directory entry. */
				if (status != 0)
				{
					printf("Thread %d: Unable to read directory %s \n",
							thread_id, element->path_name);
					break;
				}
				if (result == NULL) /* End of directory. */
					break;

				if (strcmp(entry->d_name, ".") == 0) /* Ignore the "." and ".." entries. */
					continue;

				if (strcmp(entry->d_name, "..") == 0)
					continue;

				/* Insert this directory entry in the queue. */
				new_element = (queue_element_t *)malloc(
						sizeof(queue_element_t));
				if (new_element == NULL)
				{
					perror("malloc");
					exit(EXIT_FAILURE);
				}

				/* Construct the full path name for the directory item stored in entry. */
				strcpy(new_element->path_name, element->path_name);
				strcat(new_element->path_name, "/");
				strcat(new_element->path_name, entry->d_name);
				insert_element(queue, new_element);
			}

			closedir(directory);
		} else if (S_ISREG(file_stats.st_mode))
		{ /* Directory entry is a regular file. */
			if (VERBOSE)
			{
				printf("Thread %d: %s is a regular file. \n", thread_id,
						element->path_name);
			}

			/* Insert the file into the shared queue */

			new_element = (queue_element_t *)malloc(sizeof(queue_element_t));
			if (new_element == NULL)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			/* Construct the full path name for the directory item stored in entry. */
			strcpy(new_element->path_name, element->path_name);

			/* SHARED functions take care of mutex_shared */
			SHARED_insert_file_element(new_element);
		} else
		{
			if (VERBOSE)
			{
				printf("Thread %d: %s is of type other. \n", thread_id,
						element->path_name);
			}
		}

		free((void *)element);
	}

	return ((void *)0);
}

int /* Parallel search with dynamic load balancing. */
parallel_search_dynamic(char **argv)
{
	int num_occurrences = 0;

	const int NUM_THREADS = atoi(argv[3]);
	pthread_t worker_thread[NUM_THREADS];
	RESULTS = (int*)malloc(sizeof(int) * NUM_THREADS);
	int* RESULTS_original = RESULTS;
	ARGS_FOR_THREAD* args_for_thread;
	queue_element_t* element, *new_element;
	struct stat file_stats;
	int status;
	DIR* directory = NULL;
	struct dirent *result = NULL;
	struct dirent *entry = (struct dirent *)malloc(
			sizeof(struct dirent) + MAX_LENGTH);

	int i;

	queue_t *queue = create_queue(); /* Create and initialize the queue data structure. */
	element = (queue_element_t*)malloc(sizeof(queue_element_t));
	if (element == NULL)
	{
		perror("malloc");
		exit( EXIT_FAILURE);
	}

	/* Initialize the SHARED data structure, which will be shared between threads */
	SHARED_init();

	strcpy(element->path_name, argv[2]); /* Copy the initial path name */
	element->next = NULL;
	insert_element(queue, element); /* Insert the initial path name into the queue. */

	/* Get the first element (starting directory) */
	element = remove_element(queue);

	/* Obtain information about the file. */
	status = lstat(element->path_name, &file_stats);
	if (status == -1)
	{
		printf("Error obtaining stats for %s \n", element->path_name);
		free((void *)element);
	}

	/* Add all items in start directory to queue */
	if (S_ISDIR(file_stats.st_mode))
	{
		/* If directory, descend in and post work to queue. */
		if (VERBOSE)
		{
			printf("%s is a directory. \n", element->path_name);
		}
		directory = opendir(element->path_name);
		if (directory == NULL)
		{
			printf("Unable to open directory %s \n", element->path_name);
		}

		while (1)
		{
			status = readdir_r(directory, entry, &result); /* Read directory entry. */
			if (status != 0)
			{
				printf("Unable to read directory %s \n", element->path_name);
				break;
			}
			if (result == NULL) /* End of directory. */
				break;

			if (strcmp(entry->d_name, ".") == 0) /* Ignore the "." and ".." entries. */
				continue;

			if (strcmp(entry->d_name, "..") == 0)
				continue;

			/* Insert this directory entry in the queue. */
			new_element = (queue_element_t *)malloc(sizeof(queue_element_t));
			if (new_element == NULL)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			/* Construct the full path name for the directory item stored in entry. */
			strcpy(new_element->path_name, element->path_name);
			strcat(new_element->path_name, "/");
			strcat(new_element->path_name, entry->d_name);
			insert_element(queue, new_element);
		}

		closedir(directory);
	}

	/* "SHARED.queue_files" now hold all files in the starting directory, "queue" holds all directories
	 * within the starting directory
	 */
	int num_el = num_elements(queue);	// Num elements in the queue
	int items_per_thread = round_up(num_el, NUM_THREADS);// round_up( num_el / NUM_THREADS )
	if (VERBOSE)
	{
		printf("Items (directories) per thread = %d\n", items_per_thread);
	}
	int num_el_to_threads = 0;	// Number elements added to all threads
	int num_el_to_thread = 0;	// Number elements added to current thread

	if (VERBOSE)
	{
		printf("Main thread: creating %d file finding worker threads \n",
				NUM_THREADS);
	}

	/* Create threads */
	for (i = 0; i < NUM_THREADS; i++)
	{
		args_for_thread = (ARGS_FOR_THREAD *)malloc(sizeof(ARGS_FOR_THREAD)); // Allocate memory for the structure that will be used to pack the arguments
		args_for_thread->search_string = (char*)malloc(sizeof(char) * 256);
		strcpy(args_for_thread->search_string, argv[1]); // Copy search string
		args_for_thread->threadID = i;						// Label threadID
		num_el_to_thread = 0;	// Number elements already added to this thread

		for (int j = 0; j < items_per_thread; j++)
		{
			/* Add items to thread as long as we have not exceeded the total number of elements in the queue.
			 * Another way to implement this would have been to just include a queue, with the elements already
			 * inserted, in the args_for_thread structure.
			 */
			if (num_el_to_threads < num_el)
			{
				args_for_thread->elements[j] = remove_element(queue);
				num_el_to_thread++;
				num_el_to_threads++;
			} else
			{
				// Break loop if reached end of queue (could have done "if (queue->head != NULL) {} else {}" instead)
				break;
			}
		}

		args_for_thread->num_elements = num_el_to_thread;

		if (VERBOSE)
		{
			printf("creating thread #%d... \n", i);
		}
		if ((pthread_create(&worker_thread[i], NULL,
				parallel_search_dynamic_files_thread, (void *)args_for_thread))
				!= 0)
		{
			printf("Cannot create thread \n");
			exit(0);
		}
	}

	// Wait for all the file finding worker threads to finish
	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(worker_thread[i], NULL);

	if (VERBOSE)
	{
		printf("File finding workers completed. \n");
	}

	/* Once all file finding threads have completed, start search threads.
	 * This uses the same method as above to split the queue into even(ish) workloads
	 * and then distributes those workloads to the different threads
	 * */
	num_el = num_elements(SHARED.queue_files);
	items_per_thread = round_up(num_el, NUM_THREADS);
	if (VERBOSE)
	{
		printf("Main thread: Items (files) per thread = %d\n",
				items_per_thread);
	}
	num_el_to_threads = 0;
	num_el_to_thread = 0;
	int j;

	if (VERBOSE)
	{
		printf("Main thread: creating %d keyword searching worker threads \n",
				NUM_THREADS);
	}

	/* Same as above */
	for (i = 0; i < NUM_THREADS; i++)
	{
		args_for_thread = (ARGS_FOR_THREAD*)malloc(sizeof(ARGS_FOR_THREAD)); // Allocate memory for the structure that will be used to pack the arguments
		args_for_thread->search_string = (char*)malloc(sizeof(char) * 128);
		args_for_thread->threadID = i;
		num_el_to_thread = 0;

		for (j = 0; j < items_per_thread; j++)
		{
			if (num_el_to_threads < num_el)
			{
				args_for_thread->elements[j] = remove_element(
						SHARED.queue_files);
				num_el_to_thread++;
				num_el_to_threads++;
			} else
			{
				break;
			}
		}

		args_for_thread->num_elements = num_el_to_thread;
		strcpy(args_for_thread->search_string, argv[1]);
		if (VERBOSE)
		{
			printf("creating thread #%d... \n", i);
		}
		if ((pthread_create(&worker_thread[i], NULL,
				parallel_search_dynamic_search_thread, (void *)args_for_thread))
				!= 0)
		{
			printf("Cannot create thread \n");
			exit(0);
		}
	}

	// Wait for all the keyword searching worker threads to finish
	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(worker_thread[i], NULL);

	/* Tally up the num_occurrences from the shared RESULTS[] array */
	for (i = 0; i < NUM_THREADS; i++)
	{
		num_occurrences = num_occurrences + RESULTS[i];
	}

	/* Free shared data structures */
	free(RESULTS_original);

	return num_occurrences;
}

int main(int argc, char** argv)
{

	if (argc < 5)
	{
		printf("%s search-string path num-threads static [VERBOSE]\n", argv[0]);
		printf("or \n");
		printf("%s search-string path num-threads dynamic [VERBOSE]\n",
				argv[0]);
		printf(
				"[VERBOSE] - optional, enter 'true' for verbose output, 'false' for minimal output\n");
		exit(EXIT_FAILURE);
	}

	/* Check for extra VERBOSE argument */
	if (argc == 6)
	{
		if (strcmp(argv[5], "true") == 0)
		{
			VERBOSE = true;
		} else if (strcmp(argv[5], "false") == 0)
		{
			VERBOSE = false;
		} else
		{
			printf("Unknown extra argument, proceeding with VERBOSE = true\n");
		}
	} else
	{
		printf("No extra argument, proceeding with VERBOSE = true\n");

	}

	int num_occurrences;
	struct timeval start, stop;

	if (pthread_mutex_init(&mutex_shared, NULL) != 0)
	{
		perror("mutex_lock");
		exit(1);
	}

	if (pthread_mutex_init(&mutex_file, NULL) != 0)
	{
		perror("mutex_lock");
		exit(1);
	}

	gettimeofday(&start, NULL); /* Start timing */

	num_occurrences = serial_search(argv); /* Perform a serial search of the file system. */

	gettimeofday(&stop, NULL); /* Stop timing */
	printf("\n The string %s was found %d times within the file system. \n",
			argv[1], num_occurrences);

	printf("\n Overall execution time = %fs.",
			(float)(stop.tv_sec - start.tv_sec
					+ (stop.tv_usec - start.tv_usec) / (float)1000000));

	/* Perform a multi-threaded search of the file system. */
	if (strcmp(argv[4], "static") == 0)
	{
		printf(
				"\n Performing multi-threaded search using static load balancing. \n");

		gettimeofday(&start, NULL); /* Start timing */
		num_occurrences = parallel_search_static(argv);
		gettimeofday(&stop, NULL); /* Stop timing */

		printf("\n The string %s was found %d times within the file system.",
				argv[1], num_occurrences);
		printf("\n Overall execution time = %fs.",
				(float)(stop.tv_sec - start.tv_sec
						+ (stop.tv_usec - start.tv_usec) / (float)1000000));
	} else if (strcmp(argv[4], "dynamic") == 0)
	{
		printf(
				"\n Performing multi-threaded search using dynamic load balancing. \n");

		gettimeofday(&start, NULL); /* Start timing */
		num_occurrences = parallel_search_dynamic(argv);
		gettimeofday(&stop, NULL); /* Stop timing */

		printf("\n The string %s was found %d times within the file system.",
				argv[1], num_occurrences);
		printf("\n Overall execution time = %fs.",
				(float)(stop.tv_sec - start.tv_sec
						+ (stop.tv_usec - start.tv_usec) / (float)1000000));
	} else
	{
		printf(
				"\n Unknown load balancing option provided. Defaulting to static load balancing. \n");

		gettimeofday(&start, NULL); /* Start timing */
		num_occurrences = parallel_search_static(argv);
		gettimeofday(&stop, NULL); /* Stop timing */

		printf("\n The string %s was found %d times within the \file system.",
				argv[1], num_occurrences);
		printf("\n Overall execution time = %fs.",
				(float)(stop.tv_sec - start.tv_sec
						+ (stop.tv_usec - start.tv_usec) / (float)1000000));
	}

	printf("\n");

	exit(EXIT_SUCCESS);
}
