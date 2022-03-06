#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>

short int HandleArgs(int, char**, char**, char**, char**, char**, short*, short*);
void PrintArgs(char**, char**, char**, char**, short*, short*);
int createChildOne(char**, char**, char**, int*);
int createChildTwo(char**, char**, short*, int*);

int main(int argc, char* argv[]) {
    char* inputPath = NULL;
    char* outputPath = NULL;
    char* programOnePath = NULL;
    char* programTwoPath = NULL;
    short append, devmode = 0;  //bool
    int c1, c2 = 0;
    int pipeFDs[2];

    const int READ_SIDE = 0;
    const int WRITE_SIDE = 1;


    if (HandleArgs(argc, argv, &inputPath, &outputPath, &programOnePath, &programTwoPath, 
    &append, &devmode) == -1) {
        return -1;
    }
    if (devmode == 1)
        PrintArgs(&inputPath, &outputPath, &programOnePath, &programTwoPath, &append, &devmode);

    if (programTwoPath != NULL)
        pipe(pipeFDs); //0 is rs, 1 is ws

    c1 = createChildOne(&inputPath, &programOnePath, &programTwoPath, pipeFDs);

    //printf("Created Child One\n");

    if (programTwoPath != NULL) {
        c2 = createChildTwo(&outputPath, &programTwoPath, &append, pipeFDs);
    }

    close(pipeFDs[READ_SIDE]);
    close(pipeFDs[WRITE_SIDE]);

    wait(&c1);
        
    if(programTwoPath != NULL) {
        wait(&c2);
    }

    return 0;
}

short int HandleArgs(int argc, char** argv, char** input, char** output, char** programOne, 
char** programTwo, short* append, short* devmode) {
    short int c;
    char cwd[256];
    int retval;

    while ((c = getopt(argc, argv, "d:i:o:a:1:2:pv")) != -1) {
        switch (c) {
            default:
            //Changes Working Directory
            case 'v':
                //Dev Mode prints out perror() in stdout
                (*devmode) = 1;
                close(2);
                dup(1);
                break;
            case 'd':
                if(chdir(optarg) == 0) {
                    chdir(optarg);
                }
                else {
                    perror("-d ");
                }
                break;
            //Changes input path
            case 'i':
                *input = strdup(optarg);
                if (open(*input, O_RDONLY) < 0)
                    perror("-i ");
                break;
            //Changes output path
            case 'o':
                *output = strdup(optarg);
                break;
            //Open a file in append mode
            case 'a':
                *output = strdup(optarg);
                (*append) = 1;
                break;
            case '1':
                *programOne = strdup(optarg);
                break;
            case '2':
                *programTwo = strdup(optarg);
                break;
            case 'p':
                printf("Current working Directory: %s\n", getcwd(cwd, sizeof(cwd)));
                break;
            
        }
    }
    if ((*programOne) == NULL) {
        perror("Missing required command line option -1");
        return -1; //False
    }

    return 0; //True
}

void PrintArgs(char** input, char** output, char** programOnePath, char** programTwoPath, 
short* append, short* dev) {
    if((*input) != NULL) {
        printf("The given input path given was: %s\n",  (*input));
    } if((*output) != NULL) {
        printf("The given output path given was %s\n", (*output));
    }
    if ((*programOnePath) != NULL) {
        printf("The given program is %s\n", (*programOnePath));
    }
    if ((*programTwoPath) != NULL) {
        printf("The given 2nd progam is %s\n", (*programTwoPath));
    }
    printf("Append bool is %d\n", *append);
    printf("Dev Mode bool is %d\n*", *dev);

    printf("\n\n\n\n\n");
}

int createChildOne(char** input, char** programOnePath, char** programTwoPath, int* pipe) {
    const short int READ_SIDE = 0;
    const short int WRITE_SIDE = 1;
    const short int STDIN = 0;
    const short int STDOUT = 1;
    int dupTwoCap;
    int pid = 10;
    pid = fork();

    if (pid == 0) {
        //Child

        //Set up pipe
        if (*programTwoPath != NULL) {
            close(pipe[READ_SIDE]);
            dupTwoCap = dup2(pipe[WRITE_SIDE], STDOUT);
            assert(dupTwoCap >= 0);
            close(pipe[WRITE_SIDE]);
        }

        if(*input != NULL) {
            int fd = open(*input, O_RDONLY);
            dupTwoCap = dup2(fd, 0);
            assert(dupTwoCap >= 0);
        }

        char* progArgs[] = {*programOnePath, NULL};
        if (execvp(progArgs[0], progArgs) == -1) {
            perror("Error Thrown by Child One Execvp");
            exit(0);
        }

    } else if(pid != 0) {
        //Parent Code
        printf("Child 1: %d\n", pid);
        return pid;
    }
    return -1; //Shouldn't Return 
}

int createChildTwo(char** output, char** programTwoPath, short* append, int* pipe) {
    const int READ_SIDE = 0;
    const int WRITE_SIDE = 1;
    const int STDIN = 0;
    const int STDOUT = 1;
    int dupTwoCap;
    short int pid = fork();
    if (pid == 0) {
        //Child

        //Set up pipe
        close(pipe[WRITE_SIDE]);
        dupTwoCap = dup2(pipe[READ_SIDE], STDIN);
        assert(dupTwoCap >= 0);
        close(pipe[READ_SIDE]);
        
        if (*output != NULL) {
            if (*append) {
                int fd = open(*output, O_WRONLY | O_APPEND | O_CREAT, 0777);
                dupTwoCap = dup2(fd, STDOUT);
                assert(dupTwoCap >= 0);
            }
            else {
                int fd = open(*output, O_WRONLY | O_CREAT, 0777);
                dupTwoCap = dup2(fd, STDOUT);
                assert(dupTwoCap >= 0);
            }
        }
        char* progArgs[] = {*(programTwoPath), NULL};
        if (execvp(progArgs[0], progArgs) == -1) {
            perror("Error Thrown by Child Two Execvp");
            exit(0);
        }

    } else if(pid != 0) {
        printf("Child 2: %d\n", pid);
        return pid;
    }
    return -1; //Shouldn't Return
}