#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>

using json = nlohmann::json;

struct User
{
    int id;
    std::string name;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(User, id, name);
};


int main(int argc, char const *argv[])
{

    std::ifstream f("a.json");

    json obj = json::parse(f);

    double pi = obj["pi"];
    bool is_happy = obj["happy"];

    std::cout << pi << " " << is_happy << std::endl;


    std::ifstream f1("user.json");

    json obj1 = json::parse(f1);
    
    User u = obj1.get<User>();
    std::cout << u.id << std::endl;
    std::cout << u.name << std::endl;

    User u1{102, "hhh"};
    auto u1_json_str = json(u1).dump();
    std::cout << u1_json_str << std::endl;
    
    return 0;
}
