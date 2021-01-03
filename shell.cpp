#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <wait.h>
#include <string>
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <iomanip>
#include <regex>

using namespace std;

// tagging our parts
enum tagType
{
    command,
    paras,
    redirect,
    file,
    pipeline
};

struct token
{
    tagType type;
    string text;
    token(string str)
    {
        type = tagType::paras;
        text = str;
    }
};

// contains each operation
struct process
{
    vector<string> textInput;
    vector<string> files;
    vector<string> commands;
    vector<string> parameters;
    vector<string> redirectors;

    process(vector<token> tokens)
    {
        for (auto t : tokens)
        {

            switch (t.type)
            {
            case tagType::command:
                textInput.push_back(t.text);
                commands.push_back(t.text);
                break;

            case tagType::paras:
                textInput.push_back(t.text);
                parameters.push_back(t.text);
                break;

            case tagType::file:
                files.push_back(t.text);
                break;

            case tagType::redirect:
                redirectors.push_back(t.text);
                break;
            }
        }
    }
};

// get each operation that will be executed
vector<process> createProcesses(vector<vector<string>> parts)
{

    vector<process> result;

    regex redirect("[<>]");
    regex pipe("^(\\|)$");
    regex space(" ");

    // turn into tokens into processes
    for (int i = 0; i < parts.size(); i++)
    {
        tagType lastTag = tagType::pipeline;
        vector<token> currStream;

        for (int j = 0; j < parts[i].size(); j++)
        {
            token currToken(parts[i][j]);

            if (regex_search(parts[i][j], redirect) && !regex_search(parts[i][j], space))
                currToken.type = tagType::redirect;

            else if (regex_search(parts[i][j], pipe) && !regex_search(parts[i][j], space))
                currToken.type = tagType::pipeline;

            else if (lastTag == tagType::pipeline)
                currToken.type = tagType::command;

            else if (lastTag == tagType::redirect)
                currToken.type = tagType::file;

            lastTag = currToken.type;

            //cout << currToken.text << " " << currToken.type << endl;
            currStream.push_back(currToken);
        }
        process myProcess(currStream);
        result.push_back(myProcess);
    }
    return result;
}

string trimWhitespace(string &str, string &argument)
{
    if (str.size() == 0)
        return "";

    int begin = 0;
    int end = str.size() - 1;

    while (begin < end && isspace(str[begin]))
        begin++;
    while (begin < end && isspace(str[end]))
        end--;

    //both argument + parameters
    string resultTemp(str.begin() + begin, str.begin() + end + 1);

    string recursive = "";
    string parameter = "";

    // rip argument off, send it off
    // first instance
    if (argument.size() == 0)
    {
        for (char c : resultTemp)
        {
            if (c == ' ')
                break;

            argument += c;
        }
        parameter = string(resultTemp.begin() + argument.size(), resultTemp.end());
        recursive = trimWhitespace(parameter, argument);

        return argument + ' ' + recursive;
    }
    // second instance (when recursion happens)
    else
    {
        return resultTemp;
    }
}

vector<vector<string>> splitSecond(vector<string> parts)
{
    vector<vector<string>> result;
    int lastIndex = 0;
    for (int i = 0; i < parts.size(); i++)
    {
        if (parts[i] == "|" || i == parts.size() - 1)
        {
            vector<string> currLine;

            for (int j = lastIndex; j <= i; j++)
            {
                if (parts[j] != "|")
                {
                    currLine.push_back(parts[j]);
                }
            }

            lastIndex = i;
            i++;

            result.push_back(currLine);
        }
    }
    return result;
}

// step 1: clean the string to be more suitable for parsing
// step 2: use the quoted method to split our string, but not split ones inside quotes
vector<string> splitFirst(string str)
{
    string cleanString = "";
    string lastPart = "";

    // last minute changes to grading calls for last minute thinking
    // clean the string
    for (int i = 0; i < str.size(); i++)
    {
        // remove single quotes and the fake quotes used in the handout
        if (str[i] == '\'' || str[i] == '‘' || str[i] == '“' || str[i] == '”')
        {
            cleanString += '\"';
        }
        // add space before and after a director
        else if (str[i] == '>' || str[i] == '<')
        {
            cleanString += " ";
            cleanString += str[i];
            cleanString += " ";
        }
        // add space before and after a pipe
        else if (str[i] == '|')
        {
            cleanString += " ";
            cleanString += str[i];
            cleanString += " ";
        }
        // whatever
        else
        {
            cleanString += str[i];
        }
    }

    vector<string> result;

    istringstream iss(cleanString);
    string s;

    while (iss >> quoted(s))
    {
        result.push_back(s);
    }
    return result;
}

char **vecToChar(vector<string> &parts)
{
    char **result = new char *[parts.size() + 1]; // NULL terminator
    for (int i = 0; i < parts.size(); i++)
    {
        result[i] = (char *)parts[i].c_str();
    }
    result[parts.size()] = NULL;
    return result;
}

string getDir()
{
    char buff[FILENAME_MAX]; //create string buffer to hold path
    getcwd(buff, FILENAME_MAX);
    string current_working_dir(buff);
    return current_working_dir;
}

using namespace std;
int main()
{
    vector<int> bgs;

    // username
    char *user = getenv("USER");
    string prevCdPath = "";
    int backup = dup(0);
    int backout = dup(1);

    while (true)
    {
        dup2(backup, 0);
        dup2(backout, 1);

        //check for zombie processes, and remove them
        for (int i = 0; i < bgs.size(); i++)
        {
            if (waitpid(bgs[i], 0, WNOHANG) == bgs[i])
            {
                bgs.erase(bgs.begin() + i);
                i--;
            }
        }

        // time
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        char s[64];
        assert(strftime(s, sizeof(s), "%c", tm));

        cout << "User: " << user << '\n'
             << "Current time: " << s << endl;
        // PROMPT
        cout << "My Shell$ "; //print a prompt
        string inputline;
        getline(cin, inputline); //get a line from standard input

        if (inputline == string("exit"))
        {
            cout << "Bye!! End of shell" << endl;
            break;
        }

        // BACKGROUND PROCESS
        bool backgroundProcess = false;

        // trim whitespace
        string command = "";
        inputline = trimWhitespace(inputline, command);

        if (inputline[inputline.size() - 1] == '&')
        {
            backgroundProcess = true;
            cout << "Background process began!" << endl;
            inputline = inputline.substr(0, inputline.size() - 1);
        }

        // split input by spaces
        vector<string> parts = splitFirst(inputline);
        // split input by pipes
        vector<vector<string>> seperatedCommands = splitSecond(parts);

        // our processes to execute
        vector<process> commands = createProcesses(seperatedCommands);

        for (int i = 0; i < commands.size(); i++)
        {
            int fd[2];
            pipe(fd);

            // change directory (cd), DONE & TESTED
            if (commands[i].textInput[0] == "cd")
            {
                if (prevCdPath.size() != 0 && commands[i].parameters[0] == "-")
                {
                    string tempPath = getDir();
                    int path = chdir(prevCdPath.c_str());
                    prevCdPath = tempPath;
                }
                else if (prevCdPath.size() == 0 && commands[i].parameters[0] == "-")
                {
                    cout << "No previous directory!" << endl;
                }
                else
                {
                    prevCdPath = getDir();
                    int path = chdir(commands[i].parameters[0].c_str());
                }
            }

            int pid = fork();
            if (pid == 0)
            { //child process
                // preparing the input command for execution

                if (i < commands.size() - 1)
                {
                    dup2(fd[1], 1);
                }

                // redirector
                if (!commands[i].files.empty())
                {
                    for (int j = 0; j < commands[i].redirectors.size(); j++)
                    {
                        if (commands[i].redirectors[j] == ">")
                        {
                            int f = open(commands[i].files[j].c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                            dup2(f, 1);
                        }
                        else
                        {
                            // check if file actually exists
                            if (access(commands[i].files[j].c_str(), F_OK) != -1)
                            {
                                int f = open(commands[i].files[j].c_str(), O_RDONLY | S_IRUSR);
                                dup2(f, 0);
                            }
                            else
                            {
                                cout << "Cannot find desired file." << endl;
                            }
                        }
                    }
                }

                char **args = vecToChar(commands[i].textInput);
                execvp(args[0], args);
            }

            else
            {
                if (i == commands.size() - 1 && !backgroundProcess)
                {
                    waitpid(pid, 0, 0); //parent waits for child process
                }
                else
                {
                    bgs.push_back(pid);
                }
                dup2(fd[0], 0);
                close(fd[1]);
            }
        }
    }
}