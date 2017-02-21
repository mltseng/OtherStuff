/**
 *  \file mandelbrot_susie.cc
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
	for(it = 0;it < maxit && (x*x + y*y) < 4;it++){
		newx = x*x - y*y + cx;
		newy = 2*x*y + cy;
		x = newx;
		y = newy;
	}
	return it;
}

int determine_subset_num(int rank, int P, int height){
	int i = 0;
	int t;
	while((rank + i*P) < height){
		t = rank + i*P;
		i++;
	}	
	
	return i;
}

int
main (int argc, char* argv[])
{
  /* Lucky you, you get to write MPI code */
	double minX = -2.1;
	double maxX = .7;
	double minY = -1.25;
	double maxY = 1.25;
	
	int height, width;
	if(argc == 3){
		height = atoi(argv[1]);
		width = atoi(argv[2]);
		assert(height > 0 && width > 0);
	}
	else{
		fprintf(stderr, "usage: %s <height> <width>\n", argv[0]);
		fprintf(stderr, "where <height> and <width> are the dimenstions fo the image.\n");
		return -1;
	}
	
	MPI_Init(NULL, NULL);
	double it = (maxY - minY)/height;
	double jt = (maxX - minX)/width;
	double x, y;
	
	int *img_subset;
	int *img_main;

	int P;
	MPI_Comm_size(MPI_COMM_WORLD, &P);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	int j = 0;
	int i = 0;
	int k = 0;
	int z = 0;
	int n = 0;
	int *recvcounts = NULL;
	int *displs = NULL;
	int *total_subsets = NULL;
	int f = 0;
	int subset_num = determine_subset_num(rank, P, height);
	img_subset = (int *)malloc(sizeof(int)*width*subset_num);	
	int rcount = subset_num*width;
	

	if(rank == 0){
		img_main = (int *)malloc(sizeof(int)*width*height);
		recvcounts = (int *)malloc(P*sizeof(int));
		total_subsets = (int *)malloc(P*sizeof(int));
	}

	MPI_Gather(&subset_num, 1, MPI_INT, total_subsets, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Gather(&rcount, 1, MPI_INT, recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);

	y = minY;
	for(n=0;n<subset_num;n++){
		j = rank + n*P;
		y = minY + j*it;
		x = minX;
		for(i=0;i<width;i++){
			img_subset[k] = mandelbrot(x, y);
			x += jt;
			k++;
		}

	}

	if(rank == 0){
		displs = (int *)malloc(P*sizeof(int));
		displs[0] = 0;

		for(i=1;i<P;i++){
			displs[i] = displs[i-1] + recvcounts[i-1];
		}	
	}

	MPI_Gatherv(img_subset, rcount, MPI_INT, img_main, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
	
	if(rank == 0){
		gil::rgb8_image_t img(width, height);
		auto img_view = gil::view(img);

		i = 0;
		n = 0;
		for(k=0;k<P;k++){
			for(j=0;j<total_subsets[k];j++){
				z = k + n*P;
				for(f=0;f<width;f++){
					img_view(f, z) = render(img_main[i]/512.0);
					i++;
				}
				n++;
			}
			n = 0;
		}
		gil::png_write_view("mandelbrot_susie.png", const_view(img));
	}
	MPI_Finalize();
	return 0;
}

/* eof */
