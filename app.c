#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd,error;
    unsigned char command[1];

    /*判断输入的命令是否合法*/
    if(argc != 2)
    {
        printf(" command error ! \n");
        return -1;
    }

    /*打开文件*/
    fd = open("/dev/Embedfire_LED", O_RDWR);
    if(fd < 0) {
		printf("open file : %s failed !\n", argv[0]);
		return -1;
	}

    /* 将受到的命令值转化为数字 char -> int */
    command[0] = atoi(argv[1]);  

    /*写入命令*/
    error = write(fd,command,sizeof(command));
    if(error < 0) {
        printf("write file error! \n");
        close(fd);
    }

    /*关闭文件*/
    error = close(fd);
    if(error < 0) {
        printf("close file error! \n");
    }
    
    return 0;
}