#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<libspe2.h>
#include<pthread.h>


#include"definitions.h"
#include"btc.h"

extern spe_program_handle_t tema3_spu;


void btc_decompress_serial(struct img* image, struct c_img* c_image){
	int block_row_start, block_col_start, i, j, nr_blocks, k;
	unsigned char a, b;

	nr_blocks = c_image->width * c_image->height / (BLOCK_SIZE * BLOCK_SIZE);

	image->width = c_image->width;
	image->height = c_image->height;
	image->pixels = _alloc(image->width * image->height * sizeof(short int));
	block_row_start = block_col_start = 0;
	for (i=0; i<nr_blocks; i++){
		k = block_row_start * image->width + block_col_start;
		a = c_image->blocks[i].a;
		b = c_image->blocks[i].b;
		for (j=0; j<BLOCK_SIZE * BLOCK_SIZE; j++){
			image->pixels[k++] = (c_image->blocks[i].bitplane[j] ? b : a);
			if ((j + 1) % BLOCK_SIZE == 0){
				k -= BLOCK_SIZE; //back to the first column of the block
				k += image->width ; //go to the next line
			}
		}
		block_col_start += BLOCK_SIZE;
		if (block_col_start >= image->width){
			block_col_start = 0;
			block_row_start += BLOCK_SIZE;
		}
	}
}


void* ppu_pthread_function(void *thread_arg) {

	spe_context_ptr_t ctx;
	
	
	/*Create SPE context */
	if((ctx = spe_context_create (0, NULL)) == NULL) {
           perror ("Failed creating context");
           exit (1);
       }

	/*Load SPE program into context */
	if(spe_program_load (ctx, &tema3_spu)) {
           perror ("Failed loading program");
           exit (1);
       }

	/*Run SPE context */
	unsigned int entry = SPE_DEFAULT_ENTRY;
	/*TODO: transferati prin argument adresa si dimensiunea transferului initial (vezi cerinta 2) */
	if(spe_context_run(ctx, &entry, 0, thread_arg,(void*)sizeof(pointers_t), NULL) < 0) { 
		perror("Failed running context");
		exit(1);
	}
	
	
	/*Destroy context */
	if(spe_context_destroy (ctx) != 0) {
           perror("Failed destroying context");
           exit (1);
       }
	pthread_exit(NULL);
}


int main(int argv, char **argc)
{
   DIE(argv != 6, "numar param gresit\n");
   int i, spu_threads, image_size;
   pthread_t threads[MAX_SPU_THREADS];
   pointers_t thread_arg[MAX_SPU_THREADS] __attribute__ ((aligned(16)));    
   int block_size_spe, nr_blocks;
   struct c_img compressed_img __attribute__ ((aligned(16)));
   struct img out_img __attribute__((aligned(16)));    


  /*
   * Read and open PGM image  
  */
   struct img in_img __attribute__((aligned(16)));
   read_pgm(argc[3], &in_img);
   read_pgm(argc[3], &out_img);	/*(pt alocare de mem -> suprascrie aici datele) */
   image_size = in_img.width * in_img.height;
   compressed_img.width  = in_img.width;
   compressed_img.height = in_img.height;
   nr_blocks = image_size >> 6; 
   printf("[PPU] Image Size: %d bytes\n", image_size);

  
  /*
   * Alocate memory for compressed IMG in main mem
  */
   compressed_img.blocks = (struct block*) malloc_align(nr_blocks * sizeof(struct block), 16);
   struct block_and_size *block_and_size = (struct block_and_size*) malloc_align(sizeof(struct block_and_size), 16);
   block_and_size->dim   = image_size;
   block_and_size->block = compressed_img.blocks; 
   block_and_size->out_img = &out_img;
   printf("adresa locala:%x\n", block_and_size->block);
 
 
  /* 
   * Determine the number of SPE threads to create.
  */
   spu_threads = atoi(argc[2]);
   if (spu_threads > MAX_SPU_THREADS)
   	spu_threads = MAX_SPU_THREADS;


   /* 
    * Create several SPE-threads to execute 'simple_spu'.
   */
   block_size_spe = image_size / spu_threads; 
   for(i = 0; i < spu_threads; i++) {
       
    thread_arg[i].pixel_data = in_img.pixels; 
	thread_arg[i].width  = in_img.width;
	thread_arg[i].spu_id = i;
	thread_arg[i].spu_nr = spu_threads;	
	thread_arg[i].block_and_size = block_and_size; 
	
       if (pthread_create (&threads[i], NULL, &ppu_pthread_function, &thread_arg[i]))  {
           perror ("Failed creating thread");
           exit (1);
       }
   }


   /* Wait for SPU-thread to complete execution.  */
   for (i = 0; i < spu_threads; i++) {
       if (pthread_join (threads[i], NULL)) {
           perror("Failed pthread_join");
           exit (1);
       }
   }
   
   
   /* Write BTC */
   write_btc(argc[4], &compressed_img);


   /* Decompress */
   btc_decompress_serial(&out_img, &compressed_img); 


   /* Write to file */
   write_pgm(argc[5], &out_img);        


   free_align(compressed_img.blocks);
   free_align(block_and_size);
   return 0;
}
