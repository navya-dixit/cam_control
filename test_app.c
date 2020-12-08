/*User application to control camera using dip switch*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/poll.h>
#define DS1 23
#define DS2 24
#define DS3 25
#define DS4 26
int dp_fd, cam_fd;
int st1,st2,st3,st4;
/*Polling function structure*/
struct pollfd fds[1];

unsigned int mask = 0;
int status = 0;

/*Opening dip switch driver*/ 
int dp_sw()
{
	int fd;
	system("mknod /dev/dip_sw c 244 0");
	fd = open("/dev/dip_sw", O_RDWR, 0);
	if(fd<0)
	{
		printf("cannot open device\r\n");
		return -1;
	}
	return fd;
}
/*Opening the camera driver*/
int cam_dr()
{
	int fd;
	fd = open("/dev/video1", O_RDWR,0);
	if(fd<0)
	{
		printf("cannot open device\r\n");
		return -1;
	}
	return fd;
}
/*Camera Operations*/
void cam_function()
{
	fds[0].fd = dp_fd;
	fds[0].events = POLLIN;
	printf("\nGetting the status of the switch\n");
	printf("\n**************************\n");
        /*To get the dip switch status*/
	st1 = ioctl(dp_fd, DS1, 0);
        st2 = ioctl(dp_fd, DS2, 0);
        st3 = ioctl(dp_fd, DS3, 0);
        st4 = ioctl(dp_fd, DS4, 0);
	printf("\n**********STATUS**********\n");
        printf("st2 = %d \nst3 = %d \nst4 = %d \n",st2,st3,st4);
	printf("\n**************************\n");
	if (st1==0 || st1==1)
	{
		/*camera off*/
		if (st2==0 && st3==0 && st4==0)
		{	
			printf("Camera is off\n");
			printf("\n**************************\n");
			POLL : mask = poll(fds,1);
			if(fds[0].revents & POLLIN)
			{
				if (status == 1)
				{
					status = 0;
					system("pkill -9 -f './mxc_v4l2_overlay.out'");
				}
				cam_function();
			}		
			else
				goto POLL;
			

		}	
		/*camera on or preview*/
		else if (st2==0 && st3==0 && st4==1)
		{
			printf("\nCamera Overlay\n");
			printf("\n**************************\n");
			status = 1;
			system("./mxc_v4l2_overlay.out -ow 800 -oh 480 -di /dev/video1 &");		
			POLL1 : mask = poll(fds,1);
			if(fds[0].revents & POLLIN)
			{
				if (status == 1)
				{
					status = 0;
					system("pkill -9 -f './mxc_v4l2_overlay.out'");
				}
				cam_function();
			}		
			else
				goto POLL1;	
		}
	
		/*capture the video*/
		else if ( st2==0 && st3==1 && st4==0 ) 
		{
			printf("\n Capturing the video\n");
			printf("\n**************************\n");
			system("./mxc_v4l2_capture.out -iw 640 -ih 480 -ow 800 -oh 480 -d /dev/video1 -c 300 test.yuv");
			POLL2 : mask = poll(fds,1);
			if(fds[0].revents & POLLIN)
			{
				if (status == 1)
				{
					status = 0;
					system("pkill -9 -f './mxc_v4l2_overlay.out'");
				}
				cam_function();
			}
			else
				goto POLL2;
		}
		/*play the video*/
		else if ( st2==0 && st3==1 && st4==1 ) 
		{	
			printf("\nPlaying the video\n");	
			printf("\n**************************\n");
			system("./mxc_v4l2_output.out -iw 800 -ih 480 test.yuv");
			POLL3 : mask = poll(fds,1);
			if(fds[0].revents & POLLIN)
			{
				if (status == 1)
				{
					status = 0;
					system("pkill -9 -f './mxc_v4l2_overlay.out'");
				}
				cam_function();
			}
			else
				goto POLL3;
		}
		/*Capture the image*/
		else if ( st2==1 && st3==1 && st4==1 ) 
		{
			printf("\nCapturing the image\n");
			printf("\n**************************\n");
			system("./mxc_v4l2_capture.out -iw 640 -ih 480 -ow 800 -oh 480 -d /dev/video1 -c 1 test1.yuv");
			POLL4 : mask = poll(fds,1);
			if(fds[0].revents & POLLIN)
			{
				if (status == 1)
				{
					status = 0;
					system("pkill -9 -f './mxc_v4l2_overlay.out'");
				}
				cam_function();
			}
			else
				goto POLL4;
		}
		/*Display the image*/
		else if ( st2==1 && st3==1 && st4==0 )
		{ 
			printf("\nDisplaying the image\n");
			printf("\n**************************\n");
			system("./mxc_v4l2_output.out -iw 800 -ih 480 -l 100 test1.yuv");
			POLL5 : mask = poll(fds,1);
			if(fds[0].revents & POLLIN)
			{
				if (status == 1)
				{
					status = 0;
					system("pkill -9 -f './mxc_v4l2_overlay.out'");
				}
				cam_function();
			}
			else
				goto POLL5;
		}
		else
		{
			printf("\nGive the valid switch combination\n");
			printf("D2\t|D3\t|D4\t|Camera Function\n");
			printf("--------|-------|-------|---------------------\n");
			printf("0\t|0\t|0\t|Camera is OFF\n");
			printf("0\t|0\t|1\t|Camera Preview (ON)\n");
			printf("0\t|1\t|0\t|Video Capture\n");
			printf("0\t|1\t|1\t|Video Playback (Last captured video)\n");
			printf("1\t|1\t|1\t|Image Capture\n");
			printf("1\t|1\t|0\t|Displays last captured image\n");		
			printf("\n-----------------------------------------------\n");
			goto POLL;
		}
	}	
}

int main()
{
	/*printf("\n----------Dip switch driver----------\n");*/
	/* Call the function to open the dip switch driver*/
	dp_fd = dp_sw();
	if(dp_fd < 0)
		return -1;
		
	/*printf("\n----------Camera driver-----------\n");*/
        /*Call the function to open the camera driver */
        cam_fd = cam_dr();
        if(cam_fd < 0)
                return -1;

	cam_function();

	return 0;
}

 
