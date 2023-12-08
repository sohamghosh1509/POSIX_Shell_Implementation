#include <iostream>
#include <string>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <ctime>
#include <pwd.h>
#include <grp.h>

using namespace std;

void longFormat(struct stat file_stat)
{
    
    cout << ((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    cout << ((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    cout << ((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    cout << ((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    cout << ((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    cout << ((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    cout << ((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    cout << ((file_stat.st_mode & S_IROTH) ? "r" : "-");
    cout << ((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    cout << ((file_stat.st_mode & S_IXOTH) ? "x" : "-");
    
    cout << " ";

    cout << file_stat.st_nlink << " ";
    struct passwd* user = getpwuid(file_stat.st_uid);
    struct group* grp = getgrgid(file_stat.st_gid);
    cout << user->pw_name << " ";
    cout << grp->gr_name  << " ";
    cout << file_stat.st_size << " ";
    struct tm *time = localtime(&file_stat.st_mtime);
    cout << asctime(time) << " ";
}
void listFiles(bool hidden, bool long_format, string path)
{
    DIR* dir;
    struct dirent* entry;
    struct stat path_stat;
    stat(path.c_str(), &path_stat);
    if(S_ISDIR(path_stat.st_mode))
    {
        dir = opendir(path.c_str());
        if(dir == nullptr)
        {
            perror("Error in opening the directory");
            return;
        }
        while((entry = readdir(dir)))
        {
            if (entry->d_name[0] == '.' && !hidden)
                continue;
            if (long_format)
            {
                string file = entry->d_name;
                struct stat file_stat;
                if(stat(file.c_str(), &file_stat)==0)
                {
                    longFormat(file_stat);
                }
            }
            cout << entry->d_name << endl;
        }
    }
    else
    {
        if(long_format)
            longFormat(path_stat);
        cout << path << endl;
    }
}

void processInfo(int pid)
{
    string pid_str = to_string(pid);
    string exec_path = "/proc/" + pid_str + "/exe";
    string status_path = "/proc/" + pid_str + "/status";
    ifstream status_file(status_path, ios::in);
    string line;
    while(getline(status_file, line))
    {
        if(line.find("State:") == 0)
        {
            cout << "Process Status-- " << line.substr(7) << " " << endl;
        }
        if(line.find("VmSize:") != string::npos)
        {
            stringstream str(line);
            string field, value, unit;
            str >> field >> value >> unit;
            cout << "memory-- " << value << " {Virtual Memory}" << endl;
        }
    }  
    char exec_actualpath[100];
    ssize_t exec_size = readlink(exec_path.c_str(), exec_actualpath, sizeof(exec_actualpath) - 1);
    if(exec_size == -1)
    {
        perror("Error in finding executable path of file");
    }
    else
    {
        int i = 0;
        cout << "Executable Path-- ";
        while(i < exec_size)
        {
            cout << exec_actualpath[i];
            i++;
        }
        cout << endl;
    }
}
bool searchFileDir(string &item, string &cur_dir)
{
    DIR* dir;
    struct dirent* entry;
    dir = opendir(cur_dir.c_str());
    if(dir == nullptr)
    {
        perror("Error in opening the directory");
        return false;
    }
    while(entry = readdir(dir))
    {
        string file_dir = entry->d_name;
        if(file_dir == item)
        {
            closedir(dir);
            return true;
        }
        if(file_dir != "." && file_dir != "..")
        {
            struct stat file_stat;
            stat(file_dir.c_str(), &file_stat);
            if(S_ISDIR(file_stat.st_mode))
            {
                bool res = searchFileDir(item, file_dir);
                if(res)
                    return res;
            }
        }
    }
    closedir(dir);
    return false;
}
int executeCommand(string command, bool background)
{
    pid_t child_pid = fork();
    if(child_pid < 0)
    {
        perror("Foreground/Background process failed");
    }
    else if(child_pid == 0)
    {
        if(background)
            setsid();
        vector<char*> arg_list;
        char *token;
        char *com = strdup(command.c_str());
        token = strtok(com," ");
        while (token != nullptr)
        {
            arg_list.push_back(token);
            token = strtok(nullptr, " ");
        }
        arg_list.push_back(nullptr);
        execvp(arg_list[0],arg_list.data());
    }
    else
    {
        if(!background)
        {
            int status;
            waitpid(child_pid,&status,0);
        }
        else
            return child_pid;
    }
}
int main() 
{
    struct utsname unameData;
    uname(&unameData);
    char *system = unameData.sysname;

    char curr_path[100];
    char home_path[100];
    getcwd(home_path,sizeof(home_path));
    char *user = getenv("USER");
    string path = "~";

    while (true) {

        cout << user << "@" << system << ":~>";
        string input;
        getline(cin, input);

        if (input == "exit") {
                break;
        }
        else if(input.back() == '&')
        {
            input.pop_back();
            string command = input;
            cout << executeCommand(command,true) << endl;
        }
        else if(input == "pwd") {
            getcwd(curr_path,sizeof(curr_path));
            cout << curr_path << endl;
        }
        else if(input.substr(0,4) == "echo")
        {
            string s = input.substr(5);
            cout << s << endl;
        }
        else if(input.substr(0,2) == "cd")
        {
            stringstream str(input);
            string token;
            vector<string> tokens;
            while(getline(str, token, ' '))
            {
                tokens.push_back(token);
            }
            if(tokens.size() == 1)
            {
                chdir(home_path);
            }
            else if(tokens.size() == 2)
            {
                path = tokens[1];
                if(path == "~")
                    path = home_path;
                chdir(path.c_str());
            }
            else
                cout << "Invalid use of cd" << endl;
        }
        else if(input == "ls") {
            listFiles(false,false,".");
        }
        else if(input.substr(0,2) == "ls")
        {
            stringstream str(input);
            string token;
            vector<string> tokens;
            while(getline(str, token, ' '))
            {
                tokens.push_back(token);
            }
            if(tokens.size() == 2)
            {
                if(input == "ls -l")
                    listFiles(false,true,".");
                else if(input == "ls -a")
                    listFiles(true,false,".");
                else if(input == "ls -al" || input == "ls -la")
                    listFiles(true,true,".");
                else
                {
                    string path = tokens[1];
                    cout << path << endl;
                    if(path == "~")
                        path = home_path;
                    listFiles(false,false,path);
                }
            }
            else if(tokens.size() == 3)
            {
                if(input == "ls -a -l")
                    listFiles(true,true,".");
                else if(tokens[1] == "-l")
                {
                    string path = tokens[2];
                    if(path == "~")
                        path = home_path;
                    listFiles(false,true,path);
                }
                else if(tokens[1] == "-a")
                {
                    string path = tokens[2];
                    if(path == "~")
                        path = home_path;
                    listFiles(true,false,path);
                }
                else if(tokens[1] == "-al" || tokens[1] == "-la")
                {
                    string path = tokens[2];
                    if(path == "~")
                        path = home_path;
                    listFiles(true,true,path);
                }
            }                 
            else if(tokens[1] == "-a" && tokens[2] == "-l")
            {
                string path = tokens[3];
                if(path == "~")
                    path = home_path;
                listFiles(true,true,path);
            }
        }
        else if(input == "pinfo")
        {
            int pid = getpid();
            cout << "pid -- " << pid << endl;
            processInfo(pid);
        }
        else if(input.substr(0,5) == "pinfo")
        {
            int pid = stoi(input.substr(6));
            cout << "pid -- " << pid << endl;
            processInfo(pid);
        }
        else if(input.substr(0,6) == "search")
        {
            string item = input.substr(7);
            string cur_dir = ".";
            cout << boolalpha << searchFileDir(item,cur_dir) << endl;
        }
        else if(!input.empty())
        {
            string command = input;
            executeCommand(command,false);
        }
    }
}