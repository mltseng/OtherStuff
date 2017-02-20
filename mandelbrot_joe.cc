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

int convertY(int height, int linearPlace){
	return (linearPlace / height);
}

int convertX(int width, int linearPlace){
	return (linearPlace % width);
}

int convertLinear(int x, int y, int width){
	return (y*width)+(x);
}


int mandelbrot(double x, double y) {
  int maxit = 511;
  double cx = x;
  double cy = y;
  double newx, newy;

  int it = 0;
  for (it = 0; it < maxit && (x*x + y*y) < 4; ++it) {
    newx = x*x - y*y + cx;
    newy = 2*x*y + cy;
    x = newx;
    y = newy;
  }
  return it;
}

int main (int argc, char* argv[])
{
  /* Lucky you, you get to write MPI code */
	double minX = -2.1;
	double maxX = 0.7;
	double minY = -1.25;
	double maxY = 1.25;
	
	int i,j;

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
	
	int *img_main;
	int *img_local;
	int P;
	MPI_Comm_size(MPI_COMM_WORLD, &P);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	int pRange = height/P;
	int startPlace = (rank*pRange);
	img_local = (int *)malloc(sizeof(int)*pRange);	
	
	if(rank==0){
		img_main = (int *)malloc(sizeof(int)*width*height);
	}
	
	int localX,localY;
	//iterate over the range of pixels for the processor
	for(i=startPlace;i<pRange;i++){
		//turn the linear value into X and Y coordinates on the picture
		localX = convertX(height,i);
		localY = convertY(width,y);
		//convert the X and Y coordinates into the mandel value for that pixel
		x = minX + (localX * jt);
		y = minY + (localY * it);
		//store the mandelbrot value into a local buffer to send off to root later
		*(img_local+i) = mandelbrot(x,y);
	}
	
	/******Send the local image buffer back to the root main image buffer******/
	
	//First, gather the recvcount buffer so we know how large to make each 
	//For now we're going to not worry about the last few pixels that might left out due to uneven 
	//division of labor in Joe's parallism and assume a constant recvcount 
	
	//Second, calculate the displacement from the startPlace of each processor 
	//If we ignore the uneven division of labor, displacement should also be linearly constant for all processors
	int *recvcount,*displacement;
	if(rank==0){
		for(i=0;i<P;i++){
			*(recvcount+i)    = pRange;
			*(displacement+i) = startPlace;
		}
	}
	//Start the Gather now that we know the displacement and recvcount
	MPI_Gatherv(img_local,pRange,MPI_INT,img_main,recvcount,displacement,MPI_INT,0,MPI_COMM_WORLD);
	
	if(rank==0){
		gil::rgb8_image_t img(height, width);
		auto img_view = gil::view(img);
		
		for(i=0;i<height;i++){
			for(j=0;j<width;j++){
				img_view(j,i) = img_main(convertLinear(i,j,width));
			}
		}
		
		gil::png_write_view("mandelbrot_joe.png", const_view(img));
	}
	
	MPI_Finalize();
}

/* eof */
