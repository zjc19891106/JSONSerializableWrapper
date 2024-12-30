//
//  main.m
//  JsonParser
//
//  Created by 朱继超 on 12/26/24.
//

#import <Foundation/Foundation.h>
//If you want to use nlohmann_json, you can use the following code
#include "nlohmann_json_wrapper.hpp"
//If you want to use rapidjson, you can use the following code.
//#include "rapid_json_wrapper.hpp"


// Test enum
enum class MyEnum {
    Value1,
    Value2,
    Value3
};


// User class for test
class User : public nlohmann::JSONSerializable<User> {//If you want to use rapidjson, you can use rapidjson::JSONSerializable<User>
private:
    std::string name;
    int age;
    std::string email;
    MyEnum enumValue;

public:
    User() {
        // Register properties
        registerProperty("name", name);
        registerProperty("age", age);
        registerProperty("email", email);
        // Register enum property
        registerEnum("enumValue", enumValue);
        // Register enum values for EnumSerializer,if you want to use rapidjson, you can use rapidjson::EnumSerializer<MyEnum>::instance()
        auto& serializer = nlohmann::EnumSerializer<MyEnum>::instance();
        serializer.registerValue("Value1", MyEnum::Value1);
        serializer.registerValue("Value2", MyEnum::Value2);
        serializer.registerValue("Value3", MyEnum::Value3);
    }

    void setName(const std::string& n) { name = n; }
    void setAge(int a) { age = a; }
    void setEmail(const std::string& e) { email = e; }
    void setEnumValue(MyEnum e) { enumValue = e; }

    std::string getName() const { return name; }
    int getAge() const { return age; }
    std::string getEmail() const { return email; }
    MyEnum getEnumValue() const { return enumValue; }
};

class Address : public nlohmann::JSONSerializable<Address> {//If you want to use rapidjson, you can use rapidjson::JSONSerializable<Address>
public:
    std::string street;
    std::string city;
    std::string zipCode;
    
    Address(const std::string& s = "", const std::string& c = "", const std::string& z = "")
        : street(s), city(c), zipCode(z) {
        registerProperty("street", street);
        registerProperty("city", city);
        registerProperty("zipCode", zipCode);
    }
};

// 个人类
class Person : public nlohmann::JSONSerializable<Person> {//If you want to use rapidjson, you can use rapidjson::JSONSerializable<Person>
public:
    std::string name;
    int age;
    Address homeAddress;                // nest object
    Address workAddress;                // nest object
    std::vector<Address> pastAddresses; // nest objects
    
    Person(const std::string& n = "", int a = 0)
        : name(n), age(a) {
        registerProperty("name", name);
        registerProperty("age", age);
        registerNestedObject("homeAddress", homeAddress);
        registerNestedObject("workAddress", workAddress);
        registerNestedArray("pastAddresses", pastAddresses);
    }
};

void printJson(const std::string& label, const std::string& json) {
    std::cout << "\n=== " << label << " ===\n";
    std::cout << json << std::endl;
}

void testSerialization() {
    // Validate class serializable
    static_assert(nlohmann::is_serializable<User>::value, "User must be serializable");//If you want to use rapidjson, you can use rapidjson::is_serializable<User>::value
    static_assert(nlohmann::is_serializable<Address>::value, "Address must be serializable");//If you want to use rapidjson, you can use rapidjson::is_serializable<Address>::value
    
    // create user & encode
    User user;
    user.setName("John Doe");
    user.setAge(30);
    user.setEmail("john@example.com");
    user.setEnumValue(MyEnum::Value2);
    
    std::string jsonStr = user.toJson();
    std::cout << "Serialized JSON: " << jsonStr << std::endl;
    
    // decode
    User newUser;
    newUser.fromJson(jsonStr);
    std::cout << "Deserialized name: " << newUser.getName() << std::endl;
    std::cout << "Deserialized age: " << newUser.getAge() << std::endl;
    std::cout << "Deserialized email: " << newUser.getEmail() << std::endl;
    std::cout << "Deserialized enum: " << static_cast<int>(newUser.getEnumValue()) << std::endl;
    
    // create test data
    Person person("John Doe", 30);
    
    person.homeAddress = Address(
                                 "123 Home Street",
                                 "Hometown",
                                 "12345"
                                 );
    

    person.workAddress = Address(
                                 "456 Office Road",
                                 "Worktown",
                                 "67890"
                                 );
    
   
    person.pastAddresses = {
        Address("789 Old Lane", "OldCity", "11111"),
        Address("321 Previous Ave", "PastTown", "22222"),
        Address("654 Former St", "FormerCity", "33333")
    };
    
    // encode
    std::string jsonStrPerson = person.toJson();
    printJson("Original JSON", jsonStr);
    
    // Create a new object and deserialize it from JSON.
    Person deserializedPerson;
    deserializedPerson.fromJson(jsonStrPerson);
    std::cout << "\n=== " << "Person property:" << "===\n" << "name:" << deserializedPerson.name << "age:" << deserializedPerson.age << "home_address:" << "street:" << deserializedPerson.homeAddress.street << "city:" << deserializedPerson.homeAddress.city << "city:" << deserializedPerson.homeAddress.zipCode << "nest_array:" << " street:" << deserializedPerson.pastAddresses[0].street << " city:" << deserializedPerson.pastAddresses[0].city << " zipCode:" << deserializedPerson.pastAddresses[0].zipCode << " ===\n" << std::endl;
    // Serialize it again to verify data integrity.
    std::string reserializedJson = deserializedPerson.toJson();
    printJson("Reserialized JSON", reserializedJson);
    
    // Validate data
    std::cout << "\n=== Verification ===\n";
    std::cout << "Name: " << deserializedPerson.name << std::endl;
    std::cout << "Age: " << deserializedPerson.age << std::endl;
    
    std::cout << "\nHome Address:" << std::endl;
    std::cout << "Street: " << deserializedPerson.homeAddress.street << std::endl;
    std::cout << "City: " << deserializedPerson.homeAddress.city << std::endl;
    std::cout << "ZIP: " << deserializedPerson.homeAddress.zipCode << std::endl;
    
    std::cout << "\nWork Address:" << std::endl;
    std::cout << "Street: " << deserializedPerson.workAddress.street << std::endl;
    std::cout << "City: " << deserializedPerson.workAddress.city << std::endl;
    std::cout << "ZIP: " << deserializedPerson.workAddress.zipCode << std::endl;
    
    std::cout << "\nPast Addresses:" << std::endl;
    for (size_t i = 0; i < deserializedPerson.pastAddresses.size(); ++i) {
        const auto& addr = deserializedPerson.pastAddresses[i];
        std::cout << "\nAddress " << (i + 1) << ":" << std::endl;
        std::cout << "Street: " << addr.street << std::endl;
        std::cout << "City: " << addr.city << std::endl;
        std::cout << "ZIP: " << addr.zipCode << std::endl;
    }
    
    // test for partial update
    std::string partialUpdate = R"({
            "name": "John Smith",
            "homeAddress": {
                "street": "999 New Home St",
                "city": "NewCity"
            }
        })";
    
    deserializedPerson.fromJson(partialUpdate);
    std::string updatedJson = deserializedPerson.toJson();
    printJson("Partially Updated JSON", updatedJson);
}

int main() {
    try {
        testSerialization();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
