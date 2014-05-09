#ifndef __DEFINITIONS_H
#define __DEFINITIONS_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#define MAX_SPU_THREADS     1
#define ARR_SIZE	    8000
#define ERR_CODE		-1

#define DIE(assertion, call_description)				\
	do {												\
		if (assertion) {								\
			fprintf(stderr, "(%s, %d): ",				\
					__FILE__, __LINE__);				\
			perror(call_description);					\
			exit(EXIT_FAILURE);							\
		}												\
	} while(0)


struct block_and_size {
	struct block *block;
	int dim;
	struct img *out_img;
}  __attribute__ ((aligned(16)));

typedef struct {
	short int *pixel_data;
	int width;
	short int spu_id;
	short int spu_nr;
	struct block_and_size *block_and_size;
} __attribute__ ((aligned(16))) pointers_t;


typedef struct {
	int mod;
	int num_spus;
	char *image_in;
	char *image_outC;
	char *image_out;
} input_data;


input_data* read_user_input(int argc, char **argv)	{
	  
	input_data* date_in = NULL;
	DIE(argc != 6 ,"the input should be:mod num_spus in.pgm out.btc out.pgm");
	
	date_in = (input_data*) malloc(sizeof(input_data));
	date_in->mod       = atoi(argv[1]);
	date_in->num_spus  = atoi(argv[2]);
	date_in->image_in  = argv[3];
	date_in->image_outC= argv[4];
	date_in->image_out = argv[5];
	  
	if(!date_in->image_in || !date_in->image_out || !date_in->image_outC)
		return NULL;
	else
		return date_in;
}
  
#endif
