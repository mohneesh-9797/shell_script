#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<dirent.h>
#define TRUE 1
#define MAX_LETTER_SUPPORTED 1000
#define MAX_COMMAND_SUPPORTED 100

int Find(char *** list, char * file_name, char * directory, int * location);
int to_find(int argc, char * argv[])
{

    char ** found_list = (char**)malloc(sizeof(char*) * 30);
    if (!found_list) return 0;

    int counter_location = 0;

    char * ini_dir = (char*)malloc(sizeof(char) * 500);
    if (!ini_dir) {
        free (found_list);
        return 0;
    }

    if (argv[2]){
            strcpy(ini_dir, argv[2]);
    }else{
             strcpy(ini_dir,".");
    }
    Find(&found_list, argv[1], ini_dir, &counter_location);


    if (counter_location > 0) {
        while (--counter_location >= 0) {
            write (STDOUT_FILENO,"\n", sizeof("\n"));
            write (STDOUT_FILENO,found_list[counter_location],strlen(found_list[counter_location]));
            free (found_list[counter_location]);
        }
    }
    else {
        write(STDOUT_FILENO,"Failed to find the file!\n",sizeof("Failed to find the file!\ni"));
    }

    free (ini_dir);
    free (found_list);
    return 0;
}
int Find(char *** list, char * file_name, char * directory, int * location)
{
    DIR *opened_dir;
    struct dirent *directory_structure;

    char * temp_dir = (char*)malloc(sizeof(char) * 300);
    if (!temp_dir) return -1;

    strcat(directory, "/");
    opened_dir = opendir(directory);
    if (opened_dir == NULL) {
                free (temp_dir);
                return -1;
        }

    while (1) {
        directory_structure = readdir(opened_dir);
        if (!directory_structure) {
            closedir(opened_dir);
            free (temp_dir);
            return 0;
        }
        if (!strcmp(directory_structure->d_name, "..") ||
            !strcmp(directory_structure->d_name, ".")) continue;

        else if (!strcmp(directory_structure->d_name, file_name)) {
            (*list)[*location] = (char*)malloc(sizeof(char) * 300);
            sprintf((*list)[*location], "%s%s", directory, directory_structure->d_name);
            (*location)++;
        }
        else if (directory_structure->d_type == DT_DIR) {
            sprintf(temp_dir, "%s%s", directory, directory_structure->d_name);
            if (Find(list, file_name, temp_dir, location) == -1) {
                closedir(opened_dir);
                free (temp_dir);
                return -1;
            }
        }
    }

    return 0;
}



int Input(char* str)
{
        char* buf;

        buf = readline("% ");
        if (strlen(buf) != 0) {
                add_history(buf);
                strcpy(str, buf);
                return 0;
        } else {
                return 1;
        }
}
void presentworkingdirectory()
{
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
         write(STDOUT_FILENO,cwd,sizeof("cwd"));
}

void execArgs(char** parsed)
{
        pid_t pid = fork();

        if (pid == -1) {
                return;
                exit(0);
        }
        else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command..");
        }
        exit(0);
    }
        else {
                wait(NULL);
                return;
        }
}
void execArgsPiped(char** parsed, char** parsedpipe)
{
        int pipefd[2];
        pid_t p1, p2;

        if (pipe(pipefd) < 0) {
                return;
        }
        p1 = fork();
        if (p1 < 0) {
                return;
        }

        if (p1 == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);

                if (execvp(parsed[0], parsed) < 0) {
                        exit(0);
                }
        } else {
                p2 = fork();

                if (p2 < 0) {
                        return;
                }
                if (p2 == 0) {
                        close(pipefd[1]);
                        dup2(pipefd[0], STDIN_FILENO);
                        close(pipefd[0]);
                        if (execvp(parsedpipe[0], parsedpipe) < 0) {
                                exit(0);
                        }
                } else {
                        wait(NULL);
                        wait(NULL);
                }
        }
}


int Command_Handle(char** parsed)
{
        int No_Of_Supported_Commands = 5, i, switchOwnArg = 0;
        char* List_Of_Supported_Commands[No_Of_Supported_Commands];
        char* username;

        List_Of_Supported_Commands[0] = "exit";
        List_Of_Supported_Commands[1] = "cd";
        List_Of_Supported_Commands[2] = "ls";
        List_Of_Supported_Commands[3] = "pwd";
        List_Of_Supported_Commands[4] = "ff";


        for (i = 0; i < No_Of_Supported_Commands; i++) {
                if (strcmp(parsed[0], List_Of_Supported_Commands[i]) == 0) {
                        switchOwnArg = i + 1;
                        break;
                }
        }

        switch (switchOwnArg) {
        case 1:

                write(STDOUT_FILENO,"Goodbye",sizeof("Goodbye")-1);
                exit(0);
        case 2:
                chdir(parsed[1]);
                return 1;
        case 3:
                system("ls -l");
                return 1;
        case 4:
                presentworkingdirectory();
                return 1;
        case 5:

                to_find(3,parsed);
                write(STDOUT_FILENO,"\n",sizeof("\n"));
                return 1;
        default:

                break;
        }

        return 0;
}

int parsePipe(char* str, char** strpiped)
{
        int i;
        for (i = 0; i < 2; i++) {
                strpiped[i] = strsep(&str, "|");
                if (strpiped[i] == NULL)
                        break;
        }

        if (strpiped[1] == NULL)
                return 0;
        else {
                return 1;
        }
}

void parseSpace(char* str, char** parsed)
{
        int i;

        for (i = 0; i < MAX_COMMAND_SUPPORTED; i++) {
                parsed[i] = strsep(&str, " ");

                if (parsed[i] == NULL)
                        break;
                if (strlen(parsed[i]) == 0)
                        i--;
        }
}

int processString(char* str, char** parsed, char** parsedpipe)
{

        char* strpiped[2];
        int piped = 0;

        piped = parsePipe(str, strpiped);

        if (piped) {
                parseSpace(strpiped[0], parsed);
                parseSpace(strpiped[1], parsedpipe);

        } else {

                parseSpace(str, parsed);
        }

        if (Command_Handle(parsed))
                return 0;
        else
                return 1 + piped;
}
int getWords(char *base, char target[100][200])
{
        int n=0,i,j=0;

        for(i=0;TRUE;i++)
        {
                if(base[i]!='/'){
                        target[n][j++]=base[i];
                }
                else{
                        target[n][j++]='\0';
                        n++;
                        j=0;
                }
                if(base[i]=='\0')
                    break;
        }
        return n;
}

int showworkingdir()
{
   char pwd[4096];
   getcwd(pwd, sizeof(pwd));
   if(strlen(pwd)>16)
   {
        char arr[100][200];

        int n=getWords(pwd,arr);
        int i;
        for(i=0;i<=2;i++){
                write(STDOUT_FILENO,arr[i],sizeof(arr[i]));
                if (i==2)
                {
                        write(STDOUT_FILENO,"%",sizeof("%"));
                }

                write(STDOUT_FILENO,"/",sizeof("/"));
        }
        write(STDOUT_FILENO,".../",sizeof(".../"));
        write(STDOUT_FILENO,arr[n],sizeof(arr[n]));
        return 0;
   }
   else
   {
        write(STDOUT_FILENO,pwd,strlen(pwd));
        return 0;
   }


}


int main()
{
          char inputString[MAX_LETTER_SUPPORTED], *parsedArgs[MAX_COMMAND_SUPPORTED];
        char* parsedArgsPiped[MAX_COMMAND_SUPPORTED];
        int execFlag = 0;


        while (1) {

                showworkingdir();
                if (Input(inputString))
                        continue;

                execFlag = processString(inputString,parsedArgs, parsedArgsPiped);

                if (execFlag == 1)
                        execArgs(parsedArgs);

                if (execFlag == 2)
                        execArgsPiped(parsedArgs, parsedArgsPiped);
        }
        return 0;


}


