/**
 *  \file mandelbrot_ms.cc
 *
 *  \brief Implement your parallel mandelbrot set in this file.
 */

#include <iostream>
#include <cstdlib>
#include <mpi.h>

#include "render.hh"

using namespace std;

int mandelbrot(double x, double y){
	int maxit = 511;
	double cx = x;
	double cy = y;
	double newx, newy;

	int it = 0;
	for(it=0;it < maxit && (x*x + y*y) < 4;it++){
		newx = x*x - y*y + cx;
		newy = 2*x*y + cy;
		x = newx;
		y = newy;
	}
	return it;
}

void subset_to_main(int *subset, int *main, int width){
	int n = 0;
	int row = subset[width];
	int i;

	for(n=0;n < width;n++){
		i = row*width + n;
		*(main + i) = subset[n];
	}
}

int
main (int argc, char* argv[])
{
  /* Lucky you, you get to write MPI code */
	double minX = -2.1;
	double maxX = .7;
	double minY = -1.25;
	double maxY = 1.25;
	const int W_TAG = 0;
	const int S_TAG = 1;

	int height, width;
	if(argc == 3){
		height = atoi(argv[1]);
		width = atoi(argv[2]);
		assert(height > 0 && width > 0);
	}	
	else{
		fprintf(stderr, "usage: %s <height> <width>\n", argv[0]);
		fprintf(stderr, "where <height> and <width> are the dimensions of the image.\n");
		return -1;
	}

	MPI_Init(NULL, NULL);
	double it = (maxY - minY)/height;
	double jt = (maxX - minX)/width;
	double x, y;
	
	int img_subset[width+1];
	int *img_main;

	int P;
	MPI_Comm_size(MPI_COMM_WORLD, &P);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	MPI_Status status;
	int i = 0;
	int j = 0;
	int k = 0;
	int row = 0;
	int work_row = 0;	
	
	if(rank == 0){

		img_main = (int *)malloc(sizeof(int)*width*height);
		for(i=1;i<P;i++){
			MPI_Send(&row, 1, MPI_INT, i, W_TAG, MPI_COMM_WORLD);
			row++;
//			printf("row =%d\n", row);
		}
		for(i = row;i < height;i++){
			MPI_Recv(&img_subset, width+1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	//		printf("size = %d, width = %d\n", sizeof(img_subset)/sizeof(int), width+1);
			subset_to_main(img_subset, img_main, width);
	//		printf("one.two\n");
			MPI_Send(&row, 1, MPI_INT, status.MPI_SOURCE, W_TAG, MPI_COMM_WORLD);	
			row++;
//			printf("row=%d\n", row);
		}	
		
		for(i=1;i<P;i++){
			MPI_Recv(&img_subset, width+1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			subset_to_main(img_subset, img_main, width);
		}
		for(i=1;i<P;i++){
			MPI_Send(0, 0, MPI_INT, i, S_TAG, MPI_COMM_WORLD);
		}
	}
	else{
//		img_subset = (int *)malloc(sizeof(int)*(width+1));
		while(1){
			MPI_Recv(&work_row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//			printf("work row = %d, rank = %d, status = %d\n",  work_row, rank, status.MPI_TAG);
			if(status.MPI_TAG == S_TAG){
				break;
			}	
			k = 0;
			y = minY + work_row*it;
			x = minX;
//			printf("minx = %f\n", minX);
			for(j=0;j<width;j++){
				img_subset[k] = mandelbrot(x, y);
//				printf("row = %d, y = %f, x = %f\n", work_row, y, x);
//				printf("madnel = %d\n", img_subset[k]);	
				x += jt;
				k++;
			}
			
			img_subset[width] = work_row;
			MPI_Send(&img_subset, width+1, MPI_INT, 0, 0, MPI_COMM_WORLD);	
		}
	}

	if(rank == 0){
		gil::rgb8_image_t img(width, height);
		auto img_view = gil::view(img);

/*		for(i=0;i<width*height;i++){
			printf("img = %d\n", img_main[i]);
		}*/

		k = 0;
		for(i=0;i<height;i++){
			for(j=0;j<width;j++){
				img_view(j, i) = render(img_main[k]/512.0);
				k++;
			}
		}
		gil::png_write_view("mandelbrot_ms.png", const_view(img));
	}	
	MPI_Finalize();
	return 0;
}

/* eof */
