#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <functional>
#include <chrono>
#include <sstream>
#include <tuple>

const std::string DELIM = "|--|";
const int PAGE_SIZE = 10;

using IDValState = std::map<int, std::pair<std::string, bool>>;

std::filesystem::path exeDir;
std::filesystem::path filePath;

int longestEntry(const IDValState& myNotes){
    int maxLength = 0;
    for (const auto& [id, noteDesc] : myNotes) {
        int length = noteDesc.first.length();
        if (length > maxLength) {
            maxLength = length;
        }
    }
    return maxLength;
}

int lastId(const IDValState& myNotes){
    int maxId = 0;
    for(const auto& [id,_] : myNotes){
        maxId = std::max(maxId,id);
    }
    return maxId;
}

template<typename Func>
void withTimer(Func f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "[Elapsed: " << elapsed.count() << " ms]\n";
}

bool getIntInput(int& out) {
    std::cin >> out;
    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return false;
    }
    return true;
}

std::pair<IDValState, bool> viewData(int page = 0){
    std::cout << "\033[2J\033[H";
    std::ifstream file(filePath);
    IDValState myNotes;
    bool hasNextPage = false;
    if (!file.is_open()){
        std::cout << "Error reading file." << std::endl;
        myNotes[0] = {"EMPTY",false};
        return {myNotes, false};
    }
    std::string lines;
    int lineNum = 0;
    int startLine = page * PAGE_SIZE;
    int endLine = startLine + PAGE_SIZE;
    while (std::getline(file,lines))
    {
        if (lineNum < startLine) { lineNum++; continue; }
        if (lineNum >= endLine + 1) break;
        size_t pos1 = lines.find(DELIM);
        size_t pos2 = lines.rfind(DELIM);
        if (pos1 == std::string::npos || pos2 == std::string::npos || pos1 == pos2) {
            std::cout << "Malformed line\n";
            lineNum++;
            continue;
        }
        std::string IDstr = lines.substr(0, pos1);
        std::string noteStr = lines.substr(pos1 + DELIM.length(), pos2 - (pos1 + DELIM.length()));
        std::string stateStr = lines.substr(pos2 + DELIM.length());
        try {
            int ID = std::stoi(IDstr);
            bool state = (stateStr == "true");
            if (lineNum < endLine) {
                myNotes[ID] = {noteStr,state};
            } else {
                hasNextPage = true;
            }
        } catch(...) {
            std::cout << "Malformed line\n";
        }
        lineNum++;
    }
    file.close();

    if (myNotes.empty()){
        std::cout << "\033[2J\033[H";
        std::cout << "Nothing to display.\n";
        myNotes[0] = {"EMPTY",false};
        return {myNotes, false};
    } else {
        int notesColumn = longestEntry(myNotes) + 2;
        std::cout << std::string(notesColumn-6,'-') << "Tasks" << std::string(notesColumn-6,'-') << "\n";
        for(const auto& [id,noteDesc]: myNotes){
            std::cout << (noteDesc.second ? "\033[1;32m" : "\033[1;31m") << id << "\033[0m " << noteDesc.first << "\n";
        }
        std::cout << "\nPage: " << (page+1) << "\n";
        return {myNotes, hasNextPage};
    }
}

void writeData(IDValState& myNotes){
    std::cout << "\033[3J\033[2J\033[H";
    int maxId = lastId(myNotes);
    std::ofstream file(filePath, std::ios::app);
    std::string noteStr;
    if(!file.is_open()){
        std::cout << "Error opening file." << std::endl;
        return;
    }
    if (maxId == 0){myNotes.erase(maxId);} maxId++;
    std::cin.ignore();
    std::cout << "Write your task/note:(\\n for a newline)\n";
    std::getline(std::cin, noteStr);
    myNotes[maxId] = {noteStr,false};
    file << maxId << DELIM << noteStr << DELIM << "false" << "\n";
    file.close();
}

void editData(IDValState& myNotes){
    std::cout << "\033[3J\033[2J\033[H";
    int maxId = lastId(myNotes);
    int choice;
    std::string noteStr;
    std::cout << "Select a note by its ID\n";
    if (!getIntInput(choice)){
        std::cout << "Try again. Numbers only. \n";
    } else if(myNotes.find(choice) == myNotes.end()){
        std::cout << "No such ID.";
        return;
    } else{
        int noteId = choice;
        std::cout << "Edit note/task (1)\nToggle State completed/incomplted (2)\n";
        if (!getIntInput(choice))
        {
            std::cout << "Try again. Numbers only. \n";
        } else if (choice == 1)
        {
            std::cin.ignore();
            std::cout << "Write your task/note:(\\n for a newline)\n";
            std::getline(std::cin, noteStr);
            if (noteStr.empty()){
                std::cout << "Empty note. Nothing to edit.\n";
                return;
            }
            myNotes[noteId].first = noteStr;
            std::cout << "Note updated.\n";
        } else if (choice == 2)
        {
            myNotes[noteId].second = !myNotes[noteId].second;
        }
        else{
            std::cout << "Invalid choice. Try again.\n";
            return;
        }

        std::ifstream inFile(filePath);
        std::vector<std::string> allLines;
        std::string line;
        while (std::getline(inFile, line)) {
            allLines.push_back(line);
        }
        inFile.close();

        for (auto& l : allLines) {
            size_t pos1 = l.find(DELIM);
            if (pos1 == std::string::npos) continue;
            int id = std::stoi(l.substr(0, pos1));
            if (myNotes.find(id) != myNotes.end()) {
                l = std::to_string(id) + DELIM + myNotes[id].first + DELIM + (myNotes[id].second ? "true" : "false");
            }
        }
        std::ofstream outFile(filePath);
        for (const auto& l : allLines) {
            outFile << l << "\n";
        }
        outFile.close();
    }
}

void deleteData(IDValState &myNotes){
    int maxId = lastId(myNotes);
    int choice;
    std::cout << "\n" <<"Enter ID to delete.\n";
    std::cin >> choice;
    if (std::cin.fail()){
        std::cout << "\033[3J\033[2J\033[H";
        std::cin.clear();
        std::cin.ignore(10000,'\n');
        std::cout << "Try again. Numbers only.\n";
        return;
    } else if(myNotes.find(choice) == myNotes.end()){
        std::cout << "No such ID.";
        return;
    } else{
        std::ifstream inFile(filePath);
        std::vector<std::string> allLines;
        std::string line;
        while (std::getline(inFile, line)) {
            size_t pos1 = line.find(DELIM);
            if (pos1 == std::string::npos) continue;
            int id = std::stoi(line.substr(0, pos1));
            if (id != choice) {
                allLines.push_back(line);
            }
        }
        inFile.close();
        std::ofstream outFile(filePath);
        for (const auto& l : allLines) {
            outFile << l << "\n";
        }
        outFile.close();
        myNotes.erase(choice);
        std::cout << "Entry deleted\n";
    }
}

int main(int argc, char* argv[]){
    exeDir = std::filesystem::canonical(std::filesystem::path(argv[0])).parent_path();
    filePath = exeDir / "notes" / "notes.csv";
    std::filesystem::create_directories(exeDir / "notes");

    int choice, page = 0;
    bool hasNextPage = false;
    IDValState myNotes;
    std::tie(myNotes, hasNextPage) = viewData(page);

    std::unordered_map<int, std::function<void()>> actions = {
        {1, [&]() { std::tie(myNotes, hasNextPage) = viewData(page); }},
        {2, [&]() { writeData(myNotes); std::tie(myNotes, hasNextPage) = viewData(page); }},
        {3, [&]() { editData(myNotes); std::tie(myNotes, hasNextPage) = viewData(page); }},
        {4, [&]() { deleteData(myNotes); std::tie(myNotes, hasNextPage) = viewData(page); }},
        {5, [&]() { page++; std::tie(myNotes, hasNextPage) = viewData(page); }},
        {6, [&]() { if(page>0) page--; std::tie(myNotes, hasNextPage) = viewData(page); }},
        {7, [](){ exit(0); }}
    };

    while (true) {
        std::cout << "\nUpdate Screen(1)\nNew Note(2)\nEdit Data(3)\nDelete Data(4)\n";
        if (hasNextPage) std::cout << "Next Page(5)\n";
        if (page > 0) std::cout << "Prev Page(6)\n";
        std::cout << "Exit(7)\n" << std::endl;

        if (!getIntInput(choice)){
            std::cout << "Try again. Numbers only.\n";
            continue;
        }
        auto it = actions.find(choice);
        if(it != actions.end()){
            it -> second();
        } else{
            std::cout << "Invalid Choice.\n";
        }
    }
    return 0;
}
