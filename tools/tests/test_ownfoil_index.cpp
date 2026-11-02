#include "utils/ownfoil_index.hpp"
#include <cassert>
#include <iostream>
#include <cstring>
#include <fstream>
#include <iterator>

int main(int argc, char** argv) {
    using namespace sphaira::ownfoil::index;
    std::vector<File> files;
    std::string error;
    const char* json = R"({"files":[
      {"url":"/files/base.nsp#Example%20Game%20[0100123456780000][v0].nsp","size":15032385536},
      {"url":"/files/update.nsz#Example Game [0100123456780800][v262144].nsz","size":"21474836480"},
      {"url":"/files/dlc.nsp#Example DLC [0100123456781001][v65536].nsp","size":1024},
      {"url":"https://cdn.invalid/file.nsp#A file without ID.nsp","size":-1},
      {"url":"/ignored.txt","size":10},
      null,42,{"name":"missing URL"}
    ],"titledb":{"0100123456780000":{"name":"Catalogue display name","publisher":"Example publisher","iconUrl":"/art/icon.png"}}})";
    assert(Parse(json, std::strlen(json), files, error));
    assert(files.size() == 4);
    assert(files[0].name == "Catalogue display name");
    assert(files[0].publisher == "Example publisher");
    assert(files[0].url == "/files/base.nsp");
    assert(files[0].size == 15032385536LL && files[1].size == 21474836480LL);
    assert(files[0].kind == Kind::Base && files[0].title_id == "0100123456780000");
    assert(files[1].kind == Kind::Update && files[1].title_id == files[0].id);
    assert(files[1].version == 262144 && files[1].extension == "nsz");
    assert(files[2].kind == Kind::Dlc && files[2].title_id == files[0].id && files[2].version == 65536);
    assert(files[3].kind == Kind::Unknown && files[3].name == "A file without ID" && files[3].size == 0);
    assert(Id("[0100abcdef120000]") == "0100ABCDEF120000");
    assert(Id("A00123456789000000").empty());
    assert(Id("abcdefgh12345678").empty());
    assert(Id("abcdef1234567890").empty());
    assert(CleanName("  A [DLC] [0100123456781001][v2].nsp ") == "A [DLC] .nsp");
    assert(Decode("one%20two%00three%0D%0A") == "one twothree");
    assert(Extension("GAME.NSZ?token=example") == "nsz");
    for (const auto invalid : {"{", "[]", "{\"files\":{}}", "encrypted data"}) assert(!Parse(invalid, std::strlen(invalid), files, error));
    assert(Parse("{\"files\":[]}", 12, files, error) && files.empty());
    const auto raw = R"({"files":["/Example%20[0100123456780000][v0].xci"]})";
    assert(Parse(raw, std::strlen(raw), files, error));
    assert(files.size() == 1 && files[0].name == "Example" && files[0].extension == "xci");
    const auto html = R"(<html><table><a href="Example%20[0100123456780000][v0].nsp"> </a><a href="Folder &amp; Extra/"> </a><a href="../"> </a></table></html>)";
    assert(ParseHtml(html, std::strlen(html), files, error));
    assert(files.size() == 2 && files[0].name == "Example" && files[1].directory && files[1].name == "Folder & Extra");
    assert(!ParseHtml("<a href='00 - ERROR'> </a>", 25, files, error));
    assert(!ParseHtml("<html>Login</html>", 18, files, error));
    assert(ParseHtml("<table></table>", 15, files, error) && files.empty());
    std::cout << "Ownfoil JSON/HTML parser fixtures passed\n";
    if (argc == 2) {
        std::ifstream stream(argv[1], std::ios::binary);
        assert(stream.good());
        const std::string data{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        assert(ParseHtml(data.data(), data.size(), files, error));
        const auto folders = std::count_if(files.begin(), files.end(), [](const auto& f) { return f.directory; });
        const auto ids = std::count_if(files.begin(), files.end(), [](const auto& f) { return !f.id.empty(); });
        std::cout << "Private fixture: entries=" << files.size() << " directories=" << folders << " recognized IDs=" << ids << '\n';
    }
}
