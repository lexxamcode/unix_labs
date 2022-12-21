//
// Created by oleksiy on 20.12.22.
//
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <csignal>

using namespace std;

const string passwd = "/etc/passwd";
const string shadow = "/etc/shadow";
const string gshadow = "/etc/gshadow";

struct user
{
    int uid;
    string name;
    string home_directory;
    string password_hash;
    map<string, bool> groups;

    user()
    {
        uid = -1;
        name = "";
        home_directory = "";
        password_hash = "";

    }
};

void parse_line_passwd(const string& line, user& parsed_user)
{
    istringstream is(line);
    string token;

    getline(is, token, ':'); // name
    getline(is, token, ':'); // x

    if (getline(is, token, ':')) // uid
        parsed_user.uid = stoi(token);

    getline(is, token, ':'); // gid
    getline(is, token, ':'); //info

    if (getline(is, token, ':')) // catalogue
        parsed_user.home_directory = token;
}

void parse_line_gshadow(const string& line, map<string, set<string>>& admins, map<string, set<string>>& members)
{
    istringstream is(line);
    string token;

    string group_name;

    map<string, bool> groups;

    if(getline(is, token, ':')) // group name
        group_name = token;
    getline(is, token, ':'); // group password

    if (getline(is, token, ':')) // admins
    {
        istringstream js(token);
        string name;
        while(getline(js, name, ','))
            if (name.length() >= 1)
            {
                admins[group_name].insert(name);
            }
    }

    if (getline(is, token, ':'))
    {
        istringstream js(token);
        string name;
        while (getline(js, name, ','))
            if (name.length() >= 1)
            {
                members[group_name].insert(name);
            }
    }
}

void parse_line_shadow(const string& line, user& parsed_user)
{
    istringstream is(line);
    string token;

    if(getline(is, token, ':')) // name
        parsed_user.name = token;
    if (getline(is, token, ':')) // password
        parsed_user.password_hash = token;
}

void read_passwd(vector<user>& users)
{
    ifstream file;
    file.open(passwd);

    string line;
    size_t i = 0;
    while(getline(file, line))
    {
        parse_line_passwd(line, users[i]);
        i++;
    }

    file.close();
}

void read_shadow(vector<user>& users)
{
    ifstream file;
    file.open(shadow);

    string line;
    while(getline(file, line))
    {
        user temp;
        parse_line_shadow(line, temp); //   create user object
        users.push_back(temp);            //   and initialize its
                                          //   name and password
    }

    file.close();
}

void read_gshadow(map<string, set<string>>& admins, map<string, set<string>>& members)
{
    ifstream file;
    file.open(gshadow);

    setuid(getuid()); // drop root rights

    string line;
    size_t i = 0;
    while(getline(file, line))
    {
        parse_line_gshadow(line, admins, members);
        i++;
    }

    file.close();
}

int main()
{
    system("sudo su");

    map<string, set<string>> all_admins;
    map<string, set<string>> all_members;

    vector<user> users;
    read_shadow(users);
    read_gshadow(all_admins, all_members);
    read_passwd(users);

    for (size_t i = 0; i < users.size(); i++)
    {
        cout << endl;
        cout << "Name: " << users[i].name << endl;
        cout << "UID: " << users[i].uid << endl;
        cout << "Home directory: " << users[i].home_directory << endl;
        cout << "Password: " << users[i].password_hash << endl;
        cout << "Groups:" << endl;
        for (auto& it: all_members)
        {
            if (it.second.count(users[i].name))
            {
                users[i].groups[it.first] = 0;
            }
        }
        for (auto& it: all_admins)
        {
            if (it.second.count(users[i].name))
            {
                users[i].groups[it.first] = 1;
            }
        }

        for (auto& it: users[i].groups)
        {
            if (it.second)
                cout << "\t" << it.first << " - admin" << endl;
            else
                cout << "\t" << it.first << " - member" << endl;
        }
        cout << endl;
    }

    return 0;
}