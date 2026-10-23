#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"

void parse(std::vector<Token> &tokens)
{
    Command currentCommand;
    SimpleCommand *currentSimpleCommand = new SimpleCommand();
    bool errorOccurred = false;

    for (int i = 0; i < tokens.size(); i++)
    {
        switch (tokens[i].type)
        {
        case TOKEN_COMMAND:
            currentSimpleCommand->insertArgument(strdup(tokens[i].value.c_str()));
            break;
        case TOKEN_ARGUMENT:
            currentSimpleCommand->insertArgument(strdup(tokens[i].value.c_str()));
            break;
        case TOKEN_REDIRECT:
            if (i + 1 < tokens.size() && tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._outFile = strdup((tokens[i + 1]).value.c_str());
                i++;
            }
            else
            {
                errorOccurred = true;
                printf("Error: No output file specified for redirection.\n");
            }
            break;
        case TOKEN_APPEND:
            if (i + 1 < tokens.size() && tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._append = 1;
                currentCommand._outFile = strdup(tokens[i + 1].value.c_str());
                i++;
            }
            else
            {
                errorOccurred = true;
                printf("Error: No output file specified for append.\n");
            }
            break;
        case TOKEN_INPUT:
            if (i + 1 < tokens.size() && tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._inputFile = strdup(tokens[i + 1].value.c_str());
                i++;
            }
            else
            {
                errorOccurred = true;
                printf("Error: No input file specified for redirection.\n");
            }
            break;
        case TOKEN_ERROR:
            if (i + 1 < tokens.size() && tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._errFile = strdup(tokens[i + 1].value.c_str());
                i++;
            }
            else
            {
                errorOccurred = true;
                printf("Error: No error file specified for redirection.\n");
            }
            break;
        case TOKEN_PIPE:
            if (currentSimpleCommand->_numberOfArguments > 0)
            {
                currentCommand.insertSimpleCommand(currentSimpleCommand);
                currentSimpleCommand = new SimpleCommand();
            }
            else
            {
                errorOccurred = true;
                printf("Error: No command specified before pipe.\n");
            }
            break;
        case TOKEN_BACKGROUND:
            currentCommand._background = 1;
            break;
        case TOKEN_REDIRECT_AND_ERROR:
            if (i + 1 < tokens.size() && tokens[i + 1].type == TOKEN_ARGUMENT)
            {
                currentCommand._out_error = 1;
                currentCommand._append = 1;
                currentCommand._errFile = strdup(tokens[i + 1].value.c_str());
                currentCommand._outFile = strdup(tokens[i + 1].value.c_str());
                i++;
            }
            else
            {
                errorOccurred = true;
                printf("Error: No error file specified for redirect and error.\n");
            }
            break;
        case TOKEN_EOF:
            if (currentSimpleCommand->_numberOfArguments == 0 && currentCommand._numberOfSimpleCommands > 0)
            {
                errorOccurred = true;
                printf("Error: No command specified after pipe.\n");
            }
            if (currentSimpleCommand->_numberOfArguments > 0)
            {
                currentCommand.insertSimpleCommand(currentSimpleCommand);
            }
            else
            {
                free(currentSimpleCommand->_arguments);
                delete (currentSimpleCommand);
            }
            if (currentCommand._numberOfSimpleCommands == 0)
            {
                errorOccurred = true;
            }
            if (!errorOccurred)
            {
                currentCommand.execute();
            }
            else
            {
                currentCommand.clear();
            }
            break;
        default:
            break;
        }
    }
    free(currentCommand._simpleCommands);
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _numberOfSimpleCommands = 0;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _out_error = 0;
    _append = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        delete (_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _out_error = 0;
    _append = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
        printf("\n");
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default", _background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    print();
    pid_t pid;
    int fd;
    std::vector<pid_t> childPids;
    int pipefd[2];
    int prevRead=-1;
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        if (i < _numberOfSimpleCommands - 1)
        {
            if (pipe(pipefd) == -1)
            {
                perror("Pipe failed");
                exit(EXIT_FAILURE);
            }
        }
        pid = fork();
        if (pid == 0)
        {
            if(prevRead != -1)
            {
                if (dup2(prevRead, 0) == -1)
                {
                    perror("Error duplicating file descriptor");
                    _exit(EXIT_FAILURE);
                }
                close(prevRead);
            }
            if (i < _numberOfSimpleCommands - 1)
            {
                close(pipefd[0]);
                if (dup2(pipefd[1], 1) == -1)
                {
                    perror("Error duplicating file descriptor");
                    _exit(EXIT_FAILURE);
                }
                close(pipefd[1]);
            }
            if (_out_error && i == _numberOfSimpleCommands - 1)
            {
                fd = open(_errFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd == -1)
                {
                    perror("Error opening error file");
                    _exit(EXIT_FAILURE);
                }
                if (dup2(fd, 1) == -1)
                {
                    perror("Error duplicating file descriptor");
                    _exit(EXIT_FAILURE);
                }
                if (dup2(fd, 2) == -1)
                {
                    perror("Error duplicating file descriptor");
                    _exit(EXIT_FAILURE);
                }
                close(fd);
            }
            else
            {
                if (_outFile && i == _numberOfSimpleCommands - 1)
                {
                    fd = open(_outFile, _append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1)
                    {
                        perror("Error opening output file");
                        _exit(EXIT_FAILURE);
                    }
                    if (dup2(fd, 1) == -1)
                    {
                        perror("Error duplicating file descriptor");
                        _exit(EXIT_FAILURE);
                    }
                    close(fd);
                }
                if (_errFile && i == _numberOfSimpleCommands - 1)
                {
                    fd = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1)
                    {
                        perror("Error opening error file");
                        _exit(EXIT_FAILURE);
                    }
                    if (dup2(fd, 2) == -1)
                    {
                        perror("Error duplicating file descriptor");
                        _exit(EXIT_FAILURE);
                    }
                    close(fd);
                }
            }
            if (_inputFile && i==0)
            {
                fd = open(_inputFile, O_RDONLY);
                if (fd == -1)
                {
                    perror("Error opening input file");
                    _exit(EXIT_FAILURE);
                }
                if (dup2(fd, 0) == -1)
                {
                    perror("Error duplicating file descriptor");
                    _exit(EXIT_FAILURE);
                }
                close(fd);
            }
            if (execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments) == -1)
            {
                perror("Error executing command");
                _exit(EXIT_FAILURE);
            }
        }
        else if (pid > 0)
        {
            childPids.push_back(pid);
            if(prevRead != -1)
            {
                close(prevRead);
            }
            if (i < _numberOfSimpleCommands - 1)
            {
                close(pipefd[1]);
                prevRead = pipefd[0];
            }
            if(i==_numberOfSimpleCommands-1)
            {
                prevRead=-1;
            }
        }
        else
        {
            perror("Fork failed");
        }
    }
    if (!_background)
    {
        for (int i = 0; i < childPids.size(); i++)
        {
            waitpid(childPids[i], NULL, 0);
        }
    }
    clear();
}

void Command::prompt()
{
    std::string input;
    while (true)
    {
        printf("myshell>");
        fflush(stdout);
        if (!std::getline(std::cin, input))
        {
            break;
        }
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    Command::_currentCommand.prompt();
    return 0;
}