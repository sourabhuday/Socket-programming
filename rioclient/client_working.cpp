#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <iostream>
extern "C"{
#include "csapp.h"
}
using namespace cv;
using namespace std;



int main(int argc, char *argv[])

{   int sockfd = 0;
       
    struct sockaddr_in serv_addr;

    /* Create a socket first */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
    /* Initialize sockaddr_in data structure */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); // port
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Attempt a connection */
    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
   
 while(1)
 {
 //Get Picture Size
  printf("Getting Picture Size\n");
  //Send height and width
    Mat image;
    image = imread(argv[1], CV_LOAD_IMAGE_COLOR); 
     Mat tempInput = image;
    int i = (int)(argv[2][0] - '0');

    unsigned int functions = i;
    int height = image.rows;
    int  width = image.cols;
   
 //   functions= 1;//argv[2];  
  
    printf("Sending height");
    printf("Sending width");
    printf("Sending choice");
    Rio_writen(sockfd, &height, sizeof(height));
    Rio_writen(sockfd, &width, sizeof(width));
    Rio_writen(sockfd, &functions, sizeof(functions)); 

    image = (image.reshape(0,1)); // to make it continuous
    int  imgSize = image.total()*image.elemSize();
    printf("imgSize = %d", imgSize);
    Rio_writen(sockfd, image.data, imgSize);

   



  int bytes=0;
   Mat  img = Mat::zeros( height,width, CV_8UC1);
   int  grySize = img.total()*img.elemSize();
   printf("grySize = %d", grySize);
   uchar sockData[grySize];
   printf("imgsize = %d", grySize);

 //Receive data here

     for (int i = 0; i < grySize; i += bytes) 
     {
       if ((bytes = Rio_readn(sockfd, sockData +i, grySize  - i)) == -1) 
       {
          printf("Recieve failed");	
       }

     }

 // Assign pixel value to img

    //cout << "trying to " <<endl;

    //flush(cout);
    int count = 0;
    int ptr=0;
    for (int i = 0;  i < img.rows; i++) 
    {
        for (int j = 0; j < img.cols; j++) 
        {                                     
          img.at<uchar>(i,j) = (uchar)sockData[ptr+ 0];
          ptr=ptr+1;

        }
     }

 namedWindow( "Input", WINDOW_AUTOSIZE );
 imshow( "Input", tempInput );
 //waitKey(0);

 namedWindow( "Output", WINDOW_AUTOSIZE );
 imshow( "Output", img );

 waitKey(0);

 //  imwrite( "back2.png", img );

close(sockfd);         
 }
close(sockfd);

}
