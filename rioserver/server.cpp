extern "C"{
#include "imag_proc.h"
#include "csapp.h"
}
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
/*Function prototypes*/
Mat receive(int height, int width, int fd);
void echo(Mat back, int fd);
void converttogray(Mat img , Mat& out);
void gaussianblur(Mat ip_gry , Mat& out);
void sobelfunc(Mat input , Mat& sobel);
void converttohsv(Mat imgh ,  Mat &hsv_image);
int add_delay(int delay);

queue *fifo;
queue *q;
int main (int argc, char **argv)
{

	
        if (argc != 2) 
       {
	 fprintf(stderr, "usage: %s <port>\n", argv[0]);
	 exit(1);
        }
        
        
        
         
	//queue *fifo;   
	pthread_t main, worker[4];
	int port, i;
	struct sched_param my_param;
	
        fifo = queueInit ();
	if (fifo ==  NULL) 
        {
		fprintf (stderr, "main: Queue Init failed.\n");
		exit (1);
	}
	/*Initialize thread attributes*/
        pthread_attr_t hp_attr;  
	pthread_attr_t lp_attr;
	int min_priority, policy;
	
        my_param.sched_priority = sched_get_priority_min(SCHED_FIFO); // Linux allows static priority range 1 to 99 for SCHED_FIFO and SCHED_RR 
	min_priority = my_param.sched_priority;                      //struct sched_param{int sched_priority;}
	pthread_setschedparam(pthread_self(), SCHED_RR, &my_param);  // sets the scheduling policy and parameters of the thread
	pthread_getschedparam (pthread_self(), &policy, &my_param);  //returns the scheduling policy and parametrs of the thread in the buffer my_param

 
        port = atoi(argv[1]);                                      //converts character to integer
	listenfd = Open_listenfd(port);                            // ready to accept connection requestss

      /* SCHEDULING POLICY AND PRIORITY FOR OTHER THREADS */
      pthread_attr_init(&lp_attr);                                 // initializes thread attributes pointed to by lp_attr with default
      pthread_attr_init(&hp_attr);

      pthread_attr_setinheritsched(&lp_attr, PTHREAD_EXPLICIT_SCHED);  // inherit scheduling attributes from the calling thread
      pthread_attr_setinheritsched(&hp_attr, PTHREAD_EXPLICIT_SCHED);

      pthread_attr_setschedpolicy(&lp_attr, SCHED_FIFO);            
      pthread_attr_setschedpolicy(&hp_attr, SCHED_FIFO); 

      my_param.sched_priority = min_priority + 1;
      pthread_attr_setschedparam(&lp_attr, &my_param);                //sets the priority in my_param to attr - lp_attr
      
      my_param.sched_priority = min_priority + 3;
      pthread_attr_setschedparam(&hp_attr, &my_param);
      

      /*creating main thread calling the producer routine*/
      pthread_create (&main, &hp_attr, producer, fifo);
      /*creating worker thread calling consumer routine*/ 
        
        for(i=0;i<4;i++)	
        pthread_create (&worker[i],&lp_attr, consumer, fifo);
       
       //wait for producer to complete and move on
       pthread_join (main, NULL);
	for(i=0;i<4;i++)
       //wait for all the consumer threads to terminate
        pthread_join (worker[i], NULL);
	queueDelete (fifo);
	
        
	pthread_attr_destroy(&lp_attr);   // thread attribute is no longer required
	pthread_attr_destroy(&hp_attr);
	
return 0;
}

queue *queueInit (void)
{
//	queue *q;

	q = (queue *)malloc (sizeof (queue));
	if (q == NULL) return (NULL);

	q->empty = 1;
	q->full = 0;
	q->head = 0;
	q->tail = 0;
	q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (q->mut, NULL);
	q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notFull, NULL);
	q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (q->notEmpty, NULL);
	
	return (q);
}

void queueDelete (queue *q)
{
	pthread_mutex_destroy (q->mut);
        free (q->mut);	
	pthread_cond_destroy (q->notFull);
	free (q->notFull);
	pthread_cond_destroy (q->notEmpty);
	free (q->notEmpty);
	free (q);
}

void millisleep(int milliseconds)
{
      usleep(milliseconds * 1000);
}

void queueAdd (queue *q, int in)
{
	q->buf[q->tail] = in;
	q->tail++;
	if (q->tail == QUEUESIZE)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = 1;
	q->empty = 0;

	return;
}

void queueDel (queue *q, int *out)
{
	*out = q->buf[q->head];

	q->head++;
	if (q->head == QUEUESIZE)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = 1;
	q->full = 0;

	return;
}




void *producer (void *q)
{
	pthread_t thread_id = pthread_self();   //obtain the ID of the calling thread
//        queue *fifo;
	int *fd;
        socklen_t clientlen;
    	struct sockaddr_in clientaddr;    //initialize to contain the size of this structure	
	
        struct sched_param param;        
	int priority, policy; 
       int ret;

	fifo = (queue *)q;
	
        ret = pthread_getschedparam (thread_id, &policy, &param);  // returns the scheduling policy and parameters of the thread 
                                                                   // ret is the priority value that is set by the most recent setschedparm() 
        priority = param.sched_priority;	
        printf("Manager thread: %d \n", priority);
	printf("Reached producer\n");
        int i;

	for(i=0;i<LOOP;i++) 
               {
	
		pthread_mutex_lock (fifo->mut);
		while (fifo->full) {
			printf ("producer: queue FULL.\n");
			pthread_cond_wait (fifo->notFull, fifo->mut);
		}
 
        clientlen = sizeof(clientaddr); //contains the size of the structure clientaddr
	fd=(int *)Malloc(sizeof(int));   //malloc to dynamically allocate memory for the individual connection descriptors
	*fd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //returns the file descriptor here 

        queueAdd (fifo, *fd);
    //    if(fifo->full==fifo->buf[QUEUESIZE])
     //   { 
      //     printf("Queue full");

        //}
		pthread_mutex_unlock (fifo->mut);
		pthread_cond_signal (fifo->notEmpty);
	//	millisleep (100);      // 
	}
	return (NULL);
}


void *consumer (void *q)
{ 

//Mat gray_image = Mat::zeros( 500,500, CV_64F);
 //cvtColor( img, gray_image, COLOR_BGR2GRAY );

 //imwrite( "Gray_Image.png", gray_image );
        //printf("Consumer thread")
        int workfd;
//	queue *fifo;
	pthread_t thread_id = pthread_self();
	struct sched_param param;
	int priority, policy, ret;
        
	fifo = (queue *)q;
	ret = pthread_getschedparam (thread_id, &policy, &param);
	priority = param.sched_priority;   // fetching priority to make sure we have assigned a right value
	printf("Worker threads: %d \n", priority);

  while(1)
  {  //add_delay(5);
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) 
     {
	printf ("consumer: queue EMPTY.\n");
	pthread_cond_wait (fifo->notEmpty, fifo->mut);
     }
    millisleep(10000);
    //usleep(100000000);
    //add_delay(5);
     queueDel (fifo, &workfd);
     pthread_mutex_unlock (fifo->mut);
     pthread_cond_signal (fifo->notFull);
     //printf("Reached here");
     printf ("consumer: recieved %d.\n", workfd);


        


        int height, width;
        int bytes=0;
        int functions;   
              
        //Rio_readn height and width of the image
        Rio_readn(workfd, &height, sizeof(int));
        Rio_readn(workfd, &width, sizeof(int));
        Rio_readn(workfd, &functions, sizeof(int));

        printf("Reading Picture Size, width= %d, height = %d , choice = %d \n" , width , height , functions);
        printf(" Choice = %d  \n" , functions );
     Mat img, outImg, guss ;   
     img=receive(height,width, workfd);
     //proc=converttogray(img);
     //blur=gaussianblur(proc);
     //echo(blur, workfd);
    //onverttogray(img, outImg);
       //add_delay(5);

	void (*fun[4])(Mat , Mat&);    // array of function pointers
	
	fun[0] = &converttogray;            
	fun[1] = &gaussianblur;
	fun[2] = &sobelfunc;
	fun[3] = &converttohsv;

	fun[functions - 1](img, outImg);

/*

    switch(functions)
      {
      case 1:
       converttogray(img, outImg);
      // echo(proc, workfd);
      break;
      
      case 2:
      //converttogray(img, guss);
      gaussianblur(img, outImg);
      // echo(blur, workfd);
      break;
      case 3:
     sobelfunc(img, outImg); 
      }
*/

     // printf("does it break here??");
       echo(outImg, workfd);
     //imwrite( "Gray_Image.png", gray_image );
   
     
  // echo(outImg, workfd);
   }//while(1) ends here
//close(workfd);
}//consumer function ends here



Mat receive(int height, int width, int fd)
{
 
   Mat  img = Mat::zeros( height,width, CV_8UC3);
   int  imgSize = img.total()*img.elemSize();
   uchar sockData[imgSize];
   printf("imagesize = %d\n", imgSize);
   int bytes = 0;
 //Receive data here

     for (int i = 0; i < imgSize; i += bytes) 
     {
       if ((bytes = Rio_readn(fd, sockData +i, imgSize  - i)) == -1) 
       {
          
          printf("Recieve failed");	
       }
       
     }

    // Assign pixel value to img

     
     int ptr=0;
     for (int i = 0;  i < img.rows; i++) 
     {
        for (int j = 0; j < img.cols; j++) 
        {                                     
          img.at<cv::Vec3b>(i,j) = cv::Vec3b(sockData[ptr+ 0],sockData[ptr+1],sockData[ptr+2]);
          ptr=ptr+3;
          
        }
     }

return img; 

}

void echo(Mat back, int fd)
{
     back = (back.reshape(0,1)); // to make it continuous

     int  ImgSize = back.total()*back.elemSize();
	//flush(std::cout);
     Rio_writen(fd, back.data, ImgSize);
}

/*Convert to grayscale*/
void converttogray(Mat img ,  Mat &gray_image)
{     
   millisleep(7000); 
     cvtColor( img, gray_image, COLOR_BGR2GRAY );
    // add_delay(5);
   // millisleep(30000);
   
}

/*Blur the image*/ 
void gaussianblur(Mat ip_gry ,  Mat &func)
{    
Mat gray;
converttogray(ip_gry , gray);
     GaussianBlur(gray , func, Size( 7, 7 ), 5 );
  //  add_delay(5);
   
}

/*Edge detection - Convert to sobel*/
void sobelfunc(Mat input , Mat& sobel)
{
Mat gus;
gaussianblur(input , gus);
Sobel( gus , sobel,  CV_8U , 0, 1, 3, 1, 0, BORDER_DEFAULT );
//add_delay(5);
}

/* Convert to hsv*/
void converttohsv(Mat imgh ,  Mat &hsv_image)
{     
    
     cvtColor( imgh, hsv_image, COLOR_BGR2HSV );
	Mat temp[3];
	split(hsv_image , temp);
	hsv_image = temp[1];
  //      add_delay(5);
   
}


int add_delay(int delay)
{


  struct timeval start, end;
  time_t start_time, cur_time;

  gettimeofday(&start, NULL);
  
  time(&start_time);
  do
  {
	time(&cur_time);
  }
  while((cur_time - start_time) < delay);


  gettimeofday(&end, NULL);

  printf("%ld\n", ((end.tv_sec * 1000000 + end.tv_usec)
		  - (start.tv_sec * 1000000 + start.tv_usec)));

 
  return 0;
}
