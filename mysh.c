#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

typedef struct aliasEntry{
    char* alias;
    char* command;
} aliasEntry;

int tableSize = 5;

int main(int argc, char *argv[]) {
    if(argc > 2){
        write(2, "Usage: mysh [batch-file]\n", strlen("Usage: mysh [batch-file]\n"));
        exit(1);
    }

    int batch = 0;
    FILE *batchFile;
    if(argc == 2){
        batch = 1;
        batchFile = fopen(argv[1], "r");
        if (batchFile == NULL){
            write(2, "Error: Cannot open file ", strlen("Error: Cannot open file "));
            write(2, argv[1], strlen(argv[1]));
            write(2, ".\n", strlen(".\n"));
            exit(1);
        }
    }


    aliasEntry* aliasTable = malloc(tableSize*sizeof(aliasEntry));
    for(int i = 0; i < tableSize; i++){
        aliasTable[i].alias = malloc(512); 
        aliasTable[i].command = malloc(512); 
        strcpy(aliasTable[i].alias, "alias");
        strcpy(aliasTable[i].command, "");
    }

    while(1){
        
        if(batch == 0){
            write(1, "mysh> ", strlen("mysh> "));
        }

        //Get input
        char rawInput[512];
        if(batch == 1){
            if(fgets(rawInput, sizeof(rawInput), batchFile) == NULL){
                for(int i = 0; i < tableSize; i++){
                    free(aliasTable[i].alias);
                    free(aliasTable[i].command);
                }
                free(aliasTable);
                fclose(batchFile);
                break;
            }
        }
        else{
            if(fgets(rawInput, sizeof(rawInput), stdin) == NULL){
                for(int i = 0; i < tableSize; i++){
                    free(aliasTable[i].alias);
                    free(aliasTable[i].command);
                }
                free(aliasTable);
                break;
            }
        }

        if(batch == 1){
            write(1, rawInput, strlen(rawInput));
        }
        if(strlen(rawInput) == 1){
            continue;
        }

        //trim input
        int startofInput = -1;
        while(isspace(rawInput[++startofInput]));
        int endofInput = strlen(rawInput); 
        while(isspace(rawInput[--endofInput])){
            if(endofInput == 0)break;
        }
        if(isspace(rawInput[endofInput])){
            continue;
        }
        char input[512];
        strncpy(input, rawInput + startofInput, endofInput + 1 - startofInput);
        input[endofInput + 1 - startofInput] = '\0';

        if(strlen(input) >= 512){
            write(2, "Command line too long\n", strlen("Command line too long\n"));
            continue;
        }

        if(strcmp(input, "exit") == 0){
            for(int i = 0; i < tableSize; i++){
                free(aliasTable[i].alias);
                free(aliasTable[i].command);
            }
            free(aliasTable);
            if(batch ==1){
                fclose(batchFile);
            }
            break;
        }
        
        if(strncmp(input, "alias", 5) == 0){
            //check if only 'alias' was entered
            if(strlen(input) <= 6){
                for(int i = 0; i < tableSize; i++){
                    if(strcmp(aliasTable[i].alias, "alias") != 0){
                        write(1, aliasTable[i].alias, strlen(aliasTable[i].alias));
                        write(1, " ", strlen(" "));
                        write(1, aliasTable[i].command, strlen(aliasTable[i].command));
                        write(1, "\n", strlen("\n"));
                    }
                }
                continue;
            }

            //find alias name
            int startofAlias = 5;
            while(isspace(input[++startofAlias]));
            int endofAlias = startofAlias;
            while(!isspace(input[++endofAlias]));
            
            char alias[endofAlias - startofAlias];
            strncpy(alias, input + startofAlias, endofAlias - startofAlias);
            alias[endofAlias - startofAlias] = '\0';

            if(strcmp(alias, "alias") == 0 || strcmp(alias, "exit") == 0 || strcmp(alias, "unalias") == 0 ){
                write(2, "alias: Too dangerous to alias that.\n", strlen("alias: Too dangerous to alias that.\n"));
                continue;
            }

            //check if only 1 argument is given
            if(endofAlias >= strlen(input) - 1){
                for(int j = 0; j < tableSize; j++){ 
                    if(strcmp(aliasTable[j].alias, alias) == 0){
                        write(1, aliasTable[j].alias, strlen(aliasTable[j].alias));
                        write(1, " ", strlen(" "));
                        write(1, aliasTable[j].command, strlen(aliasTable[j].command));
                        write(1, "\n", strlen("\n"));
                        break;
                    }
                }
                continue;
            }

            //two arguments given
            int startofCommand = endofAlias;
            while(isspace(input[++startofCommand]));

            char command[strlen(input) - startofCommand];
            strncpy(command, input + startofCommand, strlen(input) - startofCommand);
            command[strlen(input) - startofCommand] = '\0';

            int found = -1;
            for(int j = 0; j < tableSize; j++){
                if(strcmp(aliasTable[j].alias, alias) == 0){
                    found = j;
                    break;
                }
            }
            if(found == -1){
                for(int j = 0; j < tableSize; j++){
                    if(strcmp(aliasTable[j].alias, "alias") == 0){
                        found = j;
                        break;
                    }
                }
            }
            //grow array
            if(found == -1){
                aliasTable = realloc(aliasTable, tableSize*2*sizeof(aliasEntry));
                for(int i = 0; i < tableSize; i++){ 
                    aliasTable[i + tableSize].alias = malloc(512); 
                    aliasTable[i + tableSize].command = malloc(512); 
                    strcpy(aliasTable[i + tableSize].alias, "alias");
                    strcpy(aliasTable[i + tableSize].command, "");
                }
                found = tableSize;
                tableSize = tableSize * 2;
            }
            strcpy(aliasTable[found].alias, alias);
            strcpy(aliasTable[found].command, command);
            aliasTable[found].command[strlen(command)] = '\0';
            
            continue;
        }

        if(strncmp(input, "unalias", 7) == 0){
            if(strlen(input) <= 8){
                write(2, "unalias: Incorrect number of arguments.\n", strlen("unalias: Incorrect number of arguments.\n"));
                continue;
            }

            int startofAlias = 7;
            while(isspace(input[++startofAlias]));
            int endofAlias = startofAlias;
            while(!isspace(input[++endofAlias]));
            
            char alias[endofAlias - startofAlias];
            strncpy(alias, input + startofAlias, endofAlias - startofAlias);
            alias[endofAlias - startofAlias] = '\0';

            if(!(endofAlias >= strlen(input) - 1)){
                write(2, "unalias: Incorrect number of arguments.\n", strlen("unalias: Incorrect number of arguments.\n"));
                continue;
            }

            int found = -1;
            for(int j = 0; j < tableSize; j++){
                if(strcmp(aliasTable[j].alias, alias) == 0){
                    found = j;
                    break;
                }
            }
            if(found != -1){
                strcpy(aliasTable[found].alias, "alias");
                strcpy(aliasTable[found].command, "");
            }
            continue;
        }

        //process command
        if(input[0] == '>'){
            write(2, "Redirection misformatted.\n", strlen("Redirection misformatted.\n"));
            continue;
        }
        int endofCmd = -1;
        while(!isspace(input[++endofCmd]));
        char cmd[endofCmd];
        strncpy(cmd, input, endofCmd);
        cmd[endofCmd] = '\0';

        for(int i = 0; i < tableSize; i++){
            if(strcmp(cmd, aliasTable[i].alias) == 0){
                strcpy(input, aliasTable[i].command);  
                input[strlen(aliasTable[i].command)] = '\0';
                endofCmd = -1;
                while(!isspace(input[++endofCmd]));
                strncpy(cmd, input, endofCmd);
                cmd[endofCmd] = '\0';
                break;
            }
        }

        int foundArgs = 0;
        int foundFile = 0;
        int startofArgs = endofCmd;
        int endofArgs = startofArgs;
        int startofFile = endofArgs;

        if(endofCmd < strlen(input) - 1){
            while(isspace(input[++startofArgs]));
            if(input[startofArgs] == '>'){
                foundFile = 1;
                startofFile = startofArgs;
            }
            else{
                foundArgs = 1;
                endofArgs = startofArgs;
                while(++endofArgs < strlen(input) && input[endofArgs] != '>');
                if(input[endofArgs] == '>'){
                    foundFile = 1;
                    startofFile = endofArgs;
                }
            }
        }
        if(foundArgs == 0) {
            startofArgs = 0;
            endofArgs = 1;
        }
        if(foundFile == 0){
            startofFile = 0;
        }
        else if(startofFile >= strlen(input) - 1){
            write(2, "Redirection misformatted.\n", strlen("Redirection misformatted.\n"));
            continue;
        }
        else{
            while(isspace(input[++startofFile]));
            if(input[startofFile] == '>'){
                write(2, "Redirection misformatted.\n", strlen("Redirection misformatted.\n"));
                continue;
            }
            int whitespaceCheck = startofFile;
            for(; whitespaceCheck < strlen(input); whitespaceCheck++){
                if(isspace(input[whitespaceCheck])){
                    write(2, "Redirection misformatted.\n", strlen("Redirection misformatted.\n"));
                    break;
                }
            }
            if(whitespaceCheck < strlen(input)){
                continue;
            }
        }

        char cmdArgs[endofArgs - startofArgs]; 
        char cmdFile[strlen(input) - startofFile];

        if(foundArgs != 0) {
            strncpy(cmdArgs, input + startofArgs, endofArgs - startofArgs);
            cmdArgs[endofArgs - startofArgs] = '\0';    
        }
        if(foundFile != 0){
            strncpy(cmdFile, input + startofFile, strlen(input) - startofFile);
            cmdFile[strlen(input) - startofFile] = '\0'; 
        }

        char * paths;
        char tempPath2[endofArgs - startofArgs];
        strcpy(tempPath2,cmdArgs);
        paths = strtok(tempPath2, " ");
        int count = 0;
        while(paths != NULL){
            count++;
            paths = strtok(NULL, " ");
        }

        char *argstoExec[count + 2];
        argstoExec[0] = cmd;
        strcpy(tempPath2, cmdArgs);
        paths = strtok(tempPath2, " ");
        if(foundArgs == 1){
            argstoExec[1] = paths;  
            for(int i = 2; i < count + 1; i++){
                argstoExec[i] = strtok(NULL, " ");
            }
            argstoExec[count + 1] = NULL;
        }
        else{
            argstoExec[1] = NULL;
        }

        for(int i = 1; argstoExec[i] != NULL; i++){
            int startofArg = -1;
            while(isspace(argstoExec[i][++startofArg]));
            int endofArg = strlen(argstoExec[i]); 
            while(isspace(argstoExec[i][--endofArg])){
                if(endofInput == 0)break;
            }
            strncpy(argstoExec[i], argstoExec[i] + startofArg, endofArg - startofArg);
            argstoExec[i][endofArg + 1 - startofArg] = '\0';
        }

        //Call exec
        int pid = fork();
        if(pid != 0){
            int status;
            waitpid(-1, &status, 0);
        }
        else{
            FILE *redirectFile;
            if(foundFile == 1){
                fclose(stdout);
                redirectFile = fopen(cmdFile, "w");
                if (redirectFile == NULL){
                    _exit(1);
                }
            }
            if(execv(cmd, argstoExec) == -1){

                write(2, cmd, strlen(cmd));
                write(2, ": Command not found.\n", strlen(": Command not found.\n"));
                if(foundFile == 1){
                    fclose(redirectFile);
                }
                _exit(1);
            }
        }
    }
}