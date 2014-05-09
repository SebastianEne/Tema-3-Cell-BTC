#include <stdio.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <math.h>

#include "header.h"


#define waitag(t) mfc_write_tag_mask(1<<t); mfc_read_tag_status_all();
#define BUFFER_SIZE_SPE  128000
#define SMALL_BLOCK_SIZE 8

int main(unsigned long long speid, unsigned long long argp, unsigned long long envp)
{

	unsigned int i=0, j=0, counter=0;
	pointers_t p __attribute__ ((aligned(16)));
	struct block_and_size block_and_size __attribute__ ((aligned(16)));
	uint32_t tag_id = mfc_tag_reserve();
	short int spe_buffer[BUFFER_SIZE_SPE] __attribute__ ((aligned(16)));		
	short int *addr_counter __attribute__ ((aligned(16)));	
	short int *addr_final   __attribute__ ((aligned(16)));
	unsigned int block_size;
	int nr_tinny_blocks;	
	int image_resolution;
	int offset = 0;
	struct block block[1600]  __attribute__  ((aligned(16)));


	if (tag_id==MFC_TAG_INVALID){
		printf("SPU: ERROR can't allocate tag ID\n"); return -1;
	}
	

	/* Aici obtin structura bloc pe care o proceseaza SPU 
 	 * cu tot cu adresa
        */	
	mfc_get((void*)&p, argp, (int)envp, tag_id, 0, 0);
	waitag(tag_id);

	mfc_get((void*)&block_and_size, p.block_and_size, sizeof(struct block_and_size), tag_id, 0, 0);
	waitag(tag_id);	
	image_resolution = block_and_size.dim;
	block_size = p.width << 3;
	addr_counter = p.pixel_data + p.spu_id * block_size ;  
 	addr_final = p.pixel_data + image_resolution;	
	nr_tinny_blocks = p.width >> 3;


	/* Populez buffer-ul local */
	while(addr_counter < addr_final)  {
		printf("[SPU %d] ..copying %d bytes from main memory DMA to local storage\n", p.spu_id, block_size);
		mfc_get((void*)spe_buffer, addr_counter, block_size * sizeof(short int), tag_id, 0, 0);
		
	
		/* Process each block stored in SPE stack memory */
		for(counter = 0; counter < nr_tinny_blocks; counter++)  {
			
			float mean = 0, stdev = 0;
			float a, b;
			float q = 0, bt_index = 0;
			float f1, f2;
						
			struct block test __attribute__ ((aligned(16)));	
			/* Compute mean */
			for(i = 0; i < SMALL_BLOCK_SIZE; i++)
		     	for(j = 0; j < SMALL_BLOCK_SIZE; j++) 
			{
				mean += spe_buffer[i * p.width + j + counter * SMALL_BLOCK_SIZE];
			}
			mean /= SMALL_BLOCK_SIZE * SMALL_BLOCK_SIZE;
		

			/* Compute stdev and bitplane */ 
			for(i = 0; i < SMALL_BLOCK_SIZE; i++)
			for(j = 0; j < SMALL_BLOCK_SIZE; j++)
			{
				block[counter].bitplane[i * SMALL_BLOCK_SIZE + j]  = spe_buffer[i * p.width + j + counter * SMALL_BLOCK_SIZE] > mean ? 1 : 0; 
				stdev += (spe_buffer[i * p.width + j + counter * SMALL_BLOCK_SIZE] - mean) * (spe_buffer[i * p.width + j + counter * SMALL_BLOCK_SIZE] - mean); 	
				q += block[counter].bitplane[i * SMALL_BLOCK_SIZE + j]; 
			}
			stdev /= (SMALL_BLOCK_SIZE * SMALL_BLOCK_SIZE);
			stdev = sqrt(stdev);

				
			/* Compute A and B */
			if(q == 0) 
			{
				a = b = mean;
			}
			else {
				f1 = sqrt(q / (SMALL_BLOCK_SIZE * SMALL_BLOCK_SIZE - q));
				f2 = sqrt((SMALL_BLOCK_SIZE * SMALL_BLOCK_SIZE - q) / q);
				a  = (mean - stdev * f1);
				b  = (mean + stdev * f2);
			}

			
			/* Repair conversion issues */
			if( a < 0 ) 
				a = 0;
			if( b > 255 )
				b = 255;
			
			/* Store A and B on local SPE stack */
				block[counter].a = a;
				block[counter].b = b; 						
		
		}	
		

		mfc_put((void*)block, block_and_size.block + p.spu_id * nr_tinny_blocks + offset, nr_tinny_blocks * sizeof(struct block), tag_id, 0, 0);
                waitag(tag_id);	
	
	
		/* Send back to PPU main memory entire block * width processed */
		offset += p.spu_nr * nr_tinny_blocks;			
		addr_counter += p.spu_nr * block_size;						
	} 
	


	/* Acum facem de decompresie 
	 * in block_and_size am
	 * adresa lui out_img*/

	 		        	
 
	return 0;
}


