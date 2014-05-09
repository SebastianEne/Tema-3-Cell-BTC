#ifndef __HEADER_H
#define __HEADER_H


struct block_and_size {
	struct block *block;
	int dim;
	struct img *out_img;
} __attribute__ ((aligned(16)));


struct block {
	// data for a block from the compressed img
	unsigned char a, b;
	unsigned char bitplane[8 * 8];
	//unsigned char padding[14];
} __attribute__ ((aligned(16)));


typedef struct {
	short int *pixel_data;	// 4 bytes
	int width;		// 4 bytes
	short int spu_id;	// 2 bytes
	short int spu_nr; 	// 2 bytes
	struct block_and_size *block_and_size;		// 4 bytes	
} __attribute__ ((aligned(16))) pointers_t;


struct c_img {
	// compressed image
	int width, height;
	struct block* blocks;
} __attribute__ ((aligned(16)));


struct bits {
	unsigned bit0 : 1;
	unsigned bit1 : 1;
	unsigned bit2 : 1;
	unsigned bit3 : 1;
	unsigned bit4 : 1;
	unsigned bit5 : 1;
	unsigned bit6 : 1;
	unsigned bit7 : 1;
} __attribute__ ((aligned(16)));


/* TODO: adaugati define de waitag, vezi exemple DMA */
#define waitag(t) mfc_write_tag_mask(1<<t); mfc_read_tag_status_all();
#define BUFFER_SIZE_SPE  128000

#endif 
