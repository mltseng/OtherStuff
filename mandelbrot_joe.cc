/**
 *  \file mandelbrot_joe.cc
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
		newx = x*x - y*y +cx;
		newy = 2*x*y + cy;
		x = newx;
		y = newy;
	}
	return it;
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
		fprintf(stderr, "where <height> and <width> are the dimensions of the image.\n");
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

	int *recvcounts = NULL;
	int *displs = NULL;
	int N = height/P;
	int rcount = 0;
	int subset_h = N;
	int i = 0;
	int j = 0;
	int k = 0;
	int z = rank*N;
	int end = rank*N + (N - 1);
	
	if(rank == 0){
		img_main = (int *)malloc(sizeof(int)*width*height);
		recvcounts = (int *)malloc(sizeof(int)*P);
	}
	//Leftover blocks are put into the last processor for processing; Leftover blocks appear when the height and processor size do not divide neatly
	//So the end point of the last processor is changed to the end of the whole array	
	if(rank == (P - 1) && height % P != 0){
		end = height-1;
		subset_h = height - rank*N;
	}
	img_subset = (int *)malloc(sizeof(int)*width*subset_h);
		
	for(i = z;i <= end;i++){
		y = minY + i*it; //Allows for y to start at the proper value for the block
		x = minX;
		for(j = 0;j<width;j++){
			img_subset[k] = mandelbrot(x, y);
			x += jt;
			k++;
		}
	}
	rcount = k;

	MPI_Gather(&rcount, 1, MPI_INT, recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if(rank == 0){
		displs =(int *)malloc(P*sizeof(int));
		displs[0] = 0;

		for(i=1;i<P;i++){
			displs[i] = displs[i-1] + recvcounts[i-1];
		}
	}


	MPI_Gatherv(img_subset, rcount, MPI_INT, img_main, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

	if(rank == 0){
		gil::rgb8_image_t img(width, height);
		auto img_view = gil::view(img);
		
		k = 0;
		for(i = 0;i < height;i++){
			for(j = 0;j < width;j++){
				img_view(j, i) = render(img_main[k]/512.0);
				k++;
			}
		}
		gil::png_write_view("mandelbrot_joe.png", const_view(img));
	//	free(img_main);
	//	free(displs);
	//	free(recvcounts);
	}

//	free(img_subset);	
	MPI_Finalize();
	return 0;	
}

/* eof */
