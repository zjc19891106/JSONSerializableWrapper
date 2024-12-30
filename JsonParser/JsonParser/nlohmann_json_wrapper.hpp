//
//  nlohmann_json_wrapper.hpp
//  JsonParser
//
//  Created by 朱继超 on 12/30/24.
//

#ifndef nlohmann_json_wrapper_hpp
#define nlohmann_json_wrapper_hpp

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <type_traits>
#include "json.hpp"


namespace nlohmann {
// 使用SFINAE替代concept检查
template<typename T>
class is_serializable {
    template<typename U>
    static auto check_to_json(U*) ->
        decltype(std::declval<U>().toJson(), std::true_type{});
    
    template<typename>
    static std::false_type check_to_json(...);

    template<typename U>
    static auto check_from_json(U*) ->
        decltype(U::fromJsonStatic(std::string()), std::true_type{});
    
    template<typename>
    static std::false_type check_from_json(...);

public:
    static constexpr bool value =
        std::is_same<decltype(check_to_json<T>(nullptr)), std::true_type>::value &&
        std::is_same<decltype(check_from_json<T>(nullptr)), std::true_type>::value;
};

// 首先添加枚举序列化器
template<typename EnumType>
class EnumSerializer {
    static_assert(std::is_enum<EnumType>::value, "Type must be an enum");

public:
    using UnderlyingType = typename std::underlying_type<EnumType>::type;
    using MapType = std::unordered_map<std::string, EnumType>;
    using ReverseMapType = std::unordered_map<UnderlyingType, std::string>;
    
    static EnumSerializer& instance() {
        static EnumSerializer serializer;
        return serializer;
    }

    void registerValue(const std::string& name, EnumType value) {
        stringToEnum[name] = value;
        enumToString[static_cast<UnderlyingType>(value)] = name;
    }

    static void serialize(json& obj, const std::string& key, const EnumType& value) {
        auto& inst = instance();
        auto it = inst.enumToString.find(static_cast<UnderlyingType>(value));
        if (it != inst.enumToString.end()) {
            obj[key] = it->second;
        }
    }

    static void deserialize(const json& obj, const std::string& key, EnumType& value) {
        auto& inst = instance();
        if (obj.contains(key)) {
            const auto& jsonValue = obj[key];
            if (jsonValue.is_string()) {
                std::string strValue = jsonValue.get<std::string>();
                auto it = inst.stringToEnum.find(strValue);
                if (it != inst.stringToEnum.end()) {
                    value = it->second;
                }
            } else if (jsonValue.is_number()) {
                value = static_cast<EnumType>(jsonValue.get<int>());
            }
        }
    }

private:
    MapType stringToEnum;
    ReverseMapType enumToString;
};


// 基础序列化类
template<typename Derived>
class JSONSerializable {
protected:
    // 存储属性描述器
    std::vector<std::function<void(json&)>> serializers;
    std::vector<std::function<void(const json&)>> deserializers;

    // 注册属性 - 基本类型
    template<typename T, typename Enable = void>
    class PropertySerializer {
    public:
        static void serialize(json& obj, const std::string& key, const T& value);
        static void deserialize(const json& obj, const std::string& key, T& value);
    };

    // 特化 std::string
    template<>
    class PropertySerializer<std::string> {
    public:
        static void serialize(json& obj, const std::string& key, const std::string& value) {
            obj[key] = value;
        }

        static void deserialize(const json& obj, const std::string& key, std::string& value) {
            if (obj.contains(key) && obj[key].is_string()) {
                value = obj[key].get<std::string>();
            }
        }
    };

    // 特化 int
    template<>
    class PropertySerializer<int> {
    public:
        static void serialize(json& obj, const std::string& key, const int& value) {
            obj[key] = value;
        }

        static void deserialize(const json& obj, const std::string& key, int& value) {
            if (obj.contains(key) && obj[key].is_number_integer()) {
                value = obj[key].get<int>();
            }
        }
    };

    // 特化 double
    template<>
    class PropertySerializer<double> {
    public:
        static void serialize(json& obj, const std::string& key, const double& value) {
            obj[key] = value;
        }

        static void deserialize(const json& obj, const std::string& key, double& value) {
            if (obj.contains(key) && obj[key].is_number()) {
                value = obj[key].get<double>();
            }
        }
    };

    // 特化 bool
    template<>
    class PropertySerializer<bool> {
    public:
        static void serialize(json& obj, const std::string& key, const bool& value) {
            obj[key] = value;
        }

        static void deserialize(const json& obj, const std::string& key, bool& value) {
            if (obj.contains(key) && obj[key].is_boolean()) {
                value = obj[key].get<bool>();
            }
        }
    };

    //特化unsigned int
    template<>
    class PropertySerializer<unsigned int> {
    public:
        static void serialize(json& obj, const std::string& key, const unsigned int& value) {
            obj[key] = value;
        }

        static void deserialize(const json& obj, const std::string& key, unsigned int& value) {
            if (obj.contains(key) && obj[key].is_number_unsigned()) {
                value = obj[key].get<unsigned int>();
            }
        }
    };

    //特化uint64_t
    template<>
    class PropertySerializer<uint64_t> {
    public:
        static void serialize(json& obj, const std::string& key, const uint64_t& value) {
            obj[key] = value;
        }

        static void deserialize(const json& obj, const std::string& key, uint64_t& value) {
            if (obj.contains(key) && obj[key].is_number_unsigned()) {
                value = obj[key].get<uint64_t>();
            }
        }
    };

    //特化JSONObject
    template<>
    class PropertySerializer<JSONSerializable> {
    public:
        static void serialize(json& obj, const std::string& key, const JSONSerializable& value) {
            obj[key] = json::parse(value.toJson());
        }

        static void deserialize(const json& obj, const std::string& key, JSONSerializable& value) {
            if (obj.contains(key) && obj[key].is_object()) {
                value = JSONSerializable::fromJsonStatic(obj[key].dump());
            }
        }
    };

    //特化vector<JSONObject>
    template<>
    class PropertySerializer<std::vector<JSONSerializable>> {
    public:
        static void serialize(json& obj, const std::string& key, const std::vector<JSONSerializable>& value) {
            json arr = json::array();
            for (const auto& item : value) {
                arr.push_back(json::parse(item.toJson()));
            }
            obj[key] = arr;
        }

        static void deserialize(const json& obj, const std::string& key, std::vector<JSONSerializable>& value) {
            if (obj.contains(key) && obj[key].is_array()) {
                value.clear();
                for (const auto& item : obj[key]) {
                    value.push_back(JSONSerializable::fromJsonStatic(item.dump()));
                }
            }
        }
    };

    // 添加枚举特化
    template<typename T>
    class PropertySerializer<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    public:
        static void serialize(json& obj, const std::string& key, const T& value) {
            EnumSerializer<T>::serialize(obj, key, value);
        }
        
        static void deserialize(const json& obj, const std::string& key, T& value) {
            EnumSerializer<T>::deserialize(obj, key, value);
        }
    };

    // 添加注册枚举的便捷方法
    template<typename EnumType>
    void registerEnum(const std::string& key, EnumType& value) {
        static_assert(std::is_enum<EnumType>::value, "Type must be an enum");
        registerProperty(key, value);
    }

    // 在 JSONSerializable 类中使用
    template<typename T>
    void registerProperty(const std::string& key, T& value) {
        using Serializer = PropertySerializer<typename std::remove_reference<T>::type>;
        
        serializers.push_back([key, &value](json& obj) {
            Serializer::serialize(obj, key, value);
        });

        deserializers.push_back([key, &value](const json& obj) {
            Serializer::deserialize(obj, key, value);
        });
    }

    // 注册嵌套对象
    template<typename T>
    void registerNestedObject(const std::string& key, T& value) {
        static_assert(std::is_base_of<JSONSerializable<T>, T>::value,
            "Nested object must inherit from JSONSerializable");

        serializers.push_back([key, &value](json& obj) {
            obj[key] = json::parse(value.toJson());
        });

        deserializers.push_back([key, &value](const json& obj) {
            if (obj.contains(key) && obj[key].is_object()) {
                value = T::fromJsonStatic(obj[key].dump());
            }
        });
    }
    
    //注册嵌套对象数组
    template<typename T>
    void registerNestedArray(const std::string& key, std::vector<T>& value) {
        static_assert(std::is_base_of<JSONSerializable<T>, T>::value,
            "Nested object must inherit from JSONSerializable");

        serializers.push_back([key, &value](json& obj) {
            json arr = json::array();
            for (const auto& item : value) {
                arr.push_back(json::parse(item.toJson()));
            }
            obj[key] = arr;
        });

        deserializers.push_back([key, &value](const json& obj) {
            if (obj.contains(key) && obj[key].is_array()) {
                value.clear();
                for (const auto& item : obj[key]) {
                    value.push_back(T::fromJsonStatic(item.dump()));
                }
            }
        });
    }

public:
    virtual ~JSONSerializable() = default;

    // 序列化接口
    virtual std::string toJson() const {
        json obj;
        for (const auto& serializer : serializers) {
            serializer(obj);
        }
        return obj.dump();
    }

    // 反序列化接口
    virtual bool fromJson(const std::string& jsonStr) {
        json obj = json::parse(jsonStr);
        for (const auto& deserializer : deserializers) {
            deserializer(obj);
        }
        return true;
    }

    // 静态工厂方法
    static Derived fromJsonStatic(const std::string& jsonStr) {
        Derived obj;
        obj.fromJson(jsonStr);
        return obj;
    }
};

}



#endif /* nlohmann_json_wrapper_hpp */
