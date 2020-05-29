#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/istreamwrapper.h"

// Authors: 
// 1. Usama Khan "16L-4312"
// 2. Aimen Ijaz "17L-4171"
// 3. Ahmed Ejaz "17L-4111"

using namespace std;
using namespace rapidjson;

static const char *kTypeNames[] = {"Null", "False", "True", "Object", "Array", "String", "Number"};

struct DataBlock
{
    string name;
    string data;
    int next;
    bool used;
};

class FileSystem
{
private:
    string virtualDrive;
    int blockSize;
    vector<DataBlock> dataBlocks; // The FAT Table.
    int freeBlocks = 1000;
    vector<string> directoryStack;
    Document doc;

public:
    FileSystem()
    {
        if (!CheckVirtualDrive())
            CreateVirtualDrive();
        dataBlocks.resize(1000);
        DataBlock temp;
        temp.name = temp.data = "NULL";
        temp.used = temp.next = 0;
        for (int i = 0; i < 1000; i++)
            dataBlocks[i] = temp;
        ReadVirtualDrive();
    }
    ~FileSystem()
    {
        SaveVirtualDrive();
    }
    string GetType(Value &v)
    {
        return kTypeNames[v.GetType()];
    }
    void SaveVirtualDrive()
    {
        string rm2 = "rm " + virtualDrive + ".json";
        system(rm2.c_str());
        ofstream ofs(virtualDrive + ".json");
        OStreamWrapper osw(ofs);
        Writer<OStreamWrapper> writer(osw);
        doc.Accept(writer);
    }
    bool CheckVirtualDrive()
    {
        system("ls > t.txt"); // Creates a t.txt file to check whether a .vd file exists.

        int fd = open("t.txt", O_RDONLY);
        char *f = new char[65535];
        read(fd, f, 65535);
        string files(f);
        delete[] f;

        string fileName;
        bool flag = false;
        for (int i = 0; i < files.size() && !flag; i++)
        {
            if (files[i] == 0 || files[i] == '\n' || files[i] == EOF)
            {
                if (fileName[fileName.size() - 1] == 'd' && fileName[fileName.size() - 2] == 'v')
                {
                    flag = true;
                    virtualDrive = fileName;
                }
                fileName.clear();
            }
            else
            {
                fileName.push_back(files[i]);
            }
        }
        system("rm t.txt"); // Delete the temporary "t.txt" file.
        return flag;
    }
    void CreateVirtualDrive()
    {
        cout << "No virtual drive was found." << endl
             << "Write the new virtual hard drive's name: ";
        string vdname;
        getline(cin, vdname);
        vdname += ".vd";
        ofstream foutv(vdname);
        ofstream fout(vdname + ".json");
        virtualDrive = vdname;

        cout << "Block size: ";
        int size;
        cin >> size;
        string vdconf(vdname);
        vdconf += ".conf";
        ofstream foutc(vdconf);
        foutc << "BLOCK=" << size << endl;
        blockSize = size;

        //cout << "Root Directory (just name the folder): ";
        string root("root");
        //cin.ignore();
        //getline(cin, root);
        foutc << "ROOT=" << root;
        //directoryStack.push_back(root);

        fout << "{ \"" << root << "\": {} }";
        fout.close();
    }
    void ReadVirtualDrive()
    {
        ifstream ifs(virtualDrive + ".json");
        IStreamWrapper isw(ifs);
        doc.ParseStream(isw);

        string confFile = virtualDrive + ".conf";
        ifstream fin(confFile);
        string text;
        getline(fin, text);
        blockSize = stoi(text.substr(6));
        // getline(fin, text);
        //directoryStack.push_back("root");
        Value &v = doc;
        Value a(kObjectType);
        if (v.HasMember("new2"))
            v.AddMember("newobj", a, doc.GetAllocator());

        string rm2 = "rm " + virtualDrive + ".json";
        system(rm2.c_str());
        ofstream ofs(virtualDrive + ".json");
        OStreamWrapper osw(ofs);
        Writer<OStreamWrapper> writer(osw);
        doc.Accept(writer);
    }
    void PrintVirtualDrive()
    {
        for (Value::ConstMemberIterator itr = doc.MemberBegin();
             itr != doc.MemberEnd(); ++itr)
        {
            printf("%s: (type) %s\n",
                   itr->name.GetString(), kTypeNames[itr->value.GetType()]);
        }
    }
    int GetFreeFileBlock()
    {
        for (int i = 0; i < dataBlocks.size(); i++)
        {
            if (dataBlocks[i].used == false)
                return i;
        }
    }
    void Terminal()
    {
        directoryStack.push_back("root");
        Value &currentDirectory = doc;
        bool exit = false;
        while (!exit)
        {
            string inputCommand;
            for (int i = 0; i < directoryStack.size(); i++)
                cout << directoryStack[i] << "/";
            cout << "$ ";
            getline(cin, inputCommand);
            if (inputCommand == "exit")
                exit = true;
            else
            {
                string sub;
                for (int i = 0; inputCommand[i] != ' ' && inputCommand[i] != 0; i++)
                    sub.push_back(inputCommand[i]);
                if (sub == "help")
                {
                    cout << "ls: Lists the current directory contents." << endl
                         << "rm: Removes a specified directory." << endl
                         << "mkdir: Creates a new directory." << endl
                         << "cd: Changes directory." << endl
                         << "import: Imports a file from host system into virtual drive at the specified path." << endl
                         << "exit: Exits the terminal." << endl;
                }
                else if (sub == "cd")
                {
                    if (inputCommand.size() <= 3)
                    {
                        cout << "USAGE: cd [PATH]" << endl;
                        continue;
                    }
                    string sub2 = inputCommand.substr(3);
                    if (sub2 == ".")
                    {
                        continue;
                    }
                    else if (sub2 == "..")
                    {
                        if (directoryStack.size() == 1)
                            continue;
                        else
                        {
                            directoryStack.pop_back();
                            continue;
                        }
                    }
                    string path;
                    for (int i = 0; i <= sub2.size(); i++)
                    {
                        if (i == sub2.size() || sub2[i] == '/')
                        {
                            Value::MemberIterator itr = doc.FindMember("root");
                            for (int i = 1; i < directoryStack.size(); i++)
                            {
                                itr = itr->value.FindMember(directoryStack[i].c_str());
                            }
                            if (itr->value.HasMember(path.c_str()))
                            {
                                itr = itr->value.FindMember(path.c_str());
                                if (itr->value.GetType() == 3)
                                    directoryStack.push_back(path);
                                else
                                {
                                    cout << "Invalid path." << endl;
                                    break;
                                }
                            }
                            else
                            {
                                cout << "Invalid path." << endl;
                                break;
                            }
                            path.clear();
                        }
                        else
                            path.push_back(sub2[i]);
                    }
                }
                else if (sub == "mkdir")
                {
                    if (inputCommand.size() <= 6)
                    {
                        cout << "USAGE: mkdir [DIR NAME]" << endl;
                        continue;
                    }
                    string sub2 = inputCommand.substr(6);

                    Value::MemberIterator itr = doc.FindMember("root");
                    for (int i = 1; i < directoryStack.size(); i++)
                    {
                        itr = itr->value.FindMember(directoryStack[i].c_str());
                    }
                    if (itr->value.HasMember(sub2.c_str()))
                    {
                        cout << "'" << sub2 << "' directory already exists." << endl;
                    }
                    else
                    {
                        Value temp(kObjectType);
                        Value temp2;
                        char name[sub2.size()];
                        strcpy(name, sub2.c_str());
                        temp2.SetString(name, sub2.size(), doc.GetAllocator());
                        itr->value.AddMember(temp2, temp, doc.GetAllocator());
                    }
                }
                else if (sub == "rm")
                {
                    if (inputCommand.size() <= 3)
                    {
                        cout << "USAGE: rm [PATH]" << endl;
                        continue;
                    }
                    string sub2 = inputCommand.substr(3);
                    if (sub2 == "." || sub2 == "..")
                    {
                        continue;
                    }
                    string path;
                    string pathlast;
                    Value::MemberIterator itr = doc.FindMember("root");
                    bool remove = true;
                    for (int i = 0; i <= sub2.size(); i++)
                    {
                        if (i == sub2.size() || sub2[i] == '/')
                        {
                            for (int i = 1; i < directoryStack.size(); i++)
                            {
                                itr = itr->value.FindMember(directoryStack[i].c_str());
                            }
                            if (itr->value.HasMember(path.c_str()))
                            {
                                if (sub2[i] == '/')
                                    itr = itr->value.FindMember(path.c_str());
                            }
                            else
                            {
                                cout << "Invalid path." << endl;
                                remove = false;
                                break;
                            }
                            pathlast = path;
                            path.clear();
                        }
                        else
                            path.push_back(sub2[i]);
                    }
                    if (itr->value.GetType() == 3 && remove)
                    {
                        if (itr->value.HasMember(pathlast.c_str()))
                            itr->value.RemoveMember(itr->value.FindMember(pathlast.c_str()));
                        else
                            cout << "Invalid path." << endl;
                    }
                    else if (remove)
                        cout << "Invalid path." << endl;
                }
                else if (sub == "ls")
                {
                    Value::MemberIterator itr = doc.FindMember("root");
                    for (int i = 1; i < directoryStack.size(); i++)
                    {
                        itr = itr->value.FindMember(directoryStack[i].c_str());
                    }

                    for (Value::ConstMemberIterator itr2 = itr->value.MemberBegin();
                         itr2 != itr->value.MemberEnd(); ++itr2)
                    {
                        cout << "--> " << itr2->name.GetString();
                        if (itr2->value.GetType() == 6)
                            cout << " - (File)" << endl;
                        else if (itr2->value.GetType() == 3)
                            cout << " - (Folder)" << endl;
                        else
                            cout << ": (type) " << kTypeNames[itr2->value.GetType()] << endl;
                    }
                }
                else if (sub == "import")
                {
                    if (inputCommand.size() <= 7)
                    {
                        cout << "USAGE: import [PATH OF FILE] [DESTINATION PATH]" << endl;
                        continue;
                    }
                    string sub2 = inputCommand.substr(7);
                    string filePath, destinationPath;
                    if (sub2.find(' ') != string::npos)
                    {
                        filePath = sub2.substr(0, sub2.find(' '));
                        destinationPath = sub2.substr(sub2.find(' ') + 1);
                        ifstream fin(filePath);
                        if (!fin)
                        {
                            cout << "Invalid path of file." << endl;
                            continue;
                        }

                        string path, pathlast;
                        Value::MemberIterator itr = doc.FindMember("root");
                        bool remove = true;
                        for (int i = 0; i <= destinationPath.size(); i++)
                        {
                            if (i == destinationPath.size() || destinationPath[i] == '/')
                            {
                                for (int i = 1; i < directoryStack.size(); i++)
                                {
                                    itr = itr->value.FindMember(directoryStack[i].c_str());
                                }
                                if (destinationPath[i] == '/')
                                {
                                    if (itr->value.HasMember(path.c_str()))
                                    {
                                        itr = itr->value.FindMember(path.c_str());
                                    }
                                }
                                pathlast = path;
                                path.clear();
                            }
                            else
                                path.push_back(destinationPath[i]);
                        }
                        if (itr->value.GetType() == 3 && remove)
                        {
                            if (itr->value.HasMember(pathlast.c_str()))
                                cout << "File already exists." << endl;
                            else
                            {
                                //cout << "Current Directory: " << itr->name.GetString() << endl;
                                //cout << "Path Last: " << pathlast << endl;
                                //cout << "File Path: " << filePath << endl;

                                int j = 0;
                                int fd = open(filePath.c_str(), O_RDONLY);
                                if (fd < 0)
                                {
                                    cout << "File invalid." << endl;
                                }
                                else
                                {
                                    char *importFileBuffer = new char[65535];
                                    int readBytes = read(fd, importFileBuffer, 65535);
                                    string importFileData(importFileBuffer);
                                    delete[] importFileBuffer;
                                    //cout << "File Bytes: " << readBytes << endl;
                                    //cout << "File Data: " << importFileData << endl;
                                    //cout << "Free Blocks & Size: " << freeBlocks << " " << blockSize << endl;
                                    if (readBytes > (freeBlocks * blockSize))
                                    {
                                        cout << "ERROR: Not enough storage space to store this file." << endl;
                                    }
                                    else
                                    {
                                        int fileBlockStart = GetFreeFileBlock();
                                        int fileBlock = fileBlockStart;
                                        string fileBlockData;
                                        int remainingBytes = readBytes;
                                        for (int i = 0; i < importFileData.size(); i++)
                                        {
                                            fileBlockData.push_back(importFileData[i]);
                                            if (i % blockSize == 0)
                                            {
                                                dataBlocks[fileBlock].used = true;
                                                freeBlocks--;
                                                dataBlocks[fileBlock].data = fileBlockData;
                                                fileBlockData.clear();
                                                remainingBytes -= blockSize;
                                                if (remainingBytes <= 0)
                                                {
                                                    dataBlocks[fileBlock].next = -1;
                                                }
                                                else
                                                {
                                                    dataBlocks[fileBlock].next = GetFreeFileBlock();
                                                    fileBlock = dataBlocks[fileBlock].next;
                                                }
                                            }
                                        }
                                        Value temp;
                                        string fileName, fileNameRight;
                                        for (int i = filePath.size() - 1; i >= 0; i--)
                                        {
                                            if (filePath[i] == '/')
                                                break;
                                            else
                                                fileName.push_back(filePath[i]);
                                        }
                                        for (int i = fileName.size() - 1; i >= 0; i--)
                                            fileNameRight.push_back(fileName[i]);
                                        //cout << fileNameRight << endl;
                                        char name[fileNameRight.size()];
                                        strcpy(name, fileNameRight.c_str());
                                        temp.SetString(name, fileNameRight.size(), doc.GetAllocator());
                                        itr->value.AddMember(temp, fileBlockStart, doc.GetAllocator());
                                        //cout << freeBlocks << endl;
                                    }

                                    //while (true)
                                    //{
                                    //    break;
                                    /*
                                        How to import file?
                                        The code provided just below the while loop inserts the file and its starting block into JSON array.
                                        Write a logic that calculates the bytes a file has (the number of characters in the entire file),
                                        and when you have the no. of bytes, see if you actually have that much of free space.

                                        Free space can be calculated by (freeBlocks * blockSize). If the product is smaller than no. of bytes,
                                        we can not save that file.

                                        When you have the no. of bytes and enough free space, loop through the FAT table and store the file's characters
                                        in such a way that when one block fills, the rest of the characters are moved to the next free block (use the
                                        GetFreeFileBlock() function to get a free block.)

                                        You have stored a file in a FAT table in one of your labs, it's kind of the same thing but now each block can
                                        have multiple characters depending on its size. For example, if block size is 32, then save 32 characters in each
                                        block and then move on to the next till the file is completely saved. Make sure you're storing the "next" block's index in
                                        the DataBlock's "next" property.

                                        When you're done with all that, just store the file in the JSON array using the code provided below.
                                    */
                                    //}
                                }
                            }
                        }
                        else if (remove)
                            cout << "Invalid destination path 2." << endl;
                    }
                    else
                    {
                        cout << "USAGE: import [PATH OF FILE] [DESTINATION PATH]" << endl;
                        continue;
                    }
                    //cout << sub2.substr() << endl;
                }
                else
                {
                    cout << "'" << inputCommand << "' is not a valid command." << endl;
                }
            }
            SaveVirtualDrive();
        }
    }
};

int main()
{
    FileSystem fs;
    fs.Terminal();
}