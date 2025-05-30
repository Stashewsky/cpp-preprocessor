#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

const static regex include_library(R"(\s*#\s*include\s*<([^>]*)>\s*)");
const static regex include_header(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}
bool IncludeSearch(const path& include_file, const vector<path>& include_directories, int& line_num, const path& in_file, const path& out_file);
// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){
    ifstream input;
    input.open(in_file, ios::in);
    if(!input.is_open()){
        return false;
    }

    ofstream output;
    output.open(out_file, ios::out|ios::app);
    if(!output.is_open()){
        return false;
    }

    smatch library;
    smatch header;
    string s;
    int count_lines = 0;

    while(getline(input, s)){
        count_lines++;
        if(regex_match(s,library,include_library)){
            const path include_file = string(library[1]);
            bool is_found = IncludeSearch(include_file, include_directories, count_lines, in_file, out_file);
            if(!is_found){
                return false;
            }
        }else if(regex_match(s, header, include_header)){
            const path include_file = string(header[1]);
            const path full = in_file.parent_path() / include_file;
            ifstream get_file;
            get_file.open(full);
            if(get_file.is_open()){
                Preprocess(full, out_file, include_directories);
                get_file.close();
            }else{
                bool is_found = IncludeSearch(include_file, include_directories, count_lines, in_file, out_file);
                if(!is_found){
                    return false;
                }
            }
        }else{
            output << s << endl;
        }
    }

    return true;
}

bool IncludeSearch(const path& include_file, const vector<path>& include_directories, int& line_num, const path& in_file, const path& out_file){
    bool is_find = false;
    for(const auto& curr_file : include_directories){
        if(exists(path (curr_file / include_file))){
            is_find = true;
            Preprocess(curr_file / include_file, out_file, include_directories);
            break;
        }
    }
    if(!is_find){
        cout << "unknown include file "s << include_file.string()
             << " at file "s << in_file.string()
             << " at line "s << line_num << endl;
    }
    return is_find;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                        {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}