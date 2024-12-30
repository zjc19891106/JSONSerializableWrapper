//
//  rapid_json_wrapper.hpp
//  JsonParser
//
//  Created by 朱继超 on 12/30/24.
//

#ifndef rapid_json_wrapper_hpp
#define rapid_json_wrapper_hpp

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include "rapidjson/document.h"
#include <type_traits>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


namespace rapidjson {

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

    static void serialize(rapidjson::Value& obj,
                        const std::string& key,
                        const EnumType& value,
                        rapidjson::Document::AllocatorType& alloc) {
        auto& inst = instance();
        auto it = inst.enumToString.find(static_cast<UnderlyingType>(value));
        if (it != inst.enumToString.end()) {
            rapidjson::Value k(key.c_str(), alloc);
            rapidjson::Value v(it->second.c_str(), alloc);
            obj.AddMember(k, v, alloc);
        }
    }

    static void deserialize(const rapidjson::Value& obj,
                          const std::string& key,
                          EnumType& value) {
        auto& inst = instance();
        if (!obj.HasMember(key.c_str())) return;
        
        const auto& jsonValue = obj[key.c_str()];
        if (jsonValue.IsString()) {
            std::string strValue = jsonValue.GetString();
            auto it = inst.stringToEnum.find(strValue);
            if (it != inst.stringToEnum.end()) {
                value = it->second;
            }
        } else if (jsonValue.IsNumber()) {
            value = static_cast<EnumType>(jsonValue.GetInt());
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
    std::vector<std::function<void(rapidjson::Value&, rapidjson::Document::AllocatorType&)>> serializers;
    std::vector<std::function<void(const rapidjson::Value&)>> deserializers;

    // 注册属性 - 基本类型
    template<typename T, typename Enable = void>
    class PropertySerializer {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const T& value,
                              rapidjson::Document::AllocatorType& alloc);
        static void deserialize(const rapidjson::Value& obj, const std::string& key, T& value);
    };

    // 特化 std::string
    template<>
    class PropertySerializer<std::string> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const std::string& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value k(key.c_str(), alloc);
            rapidjson::Value v(value.c_str(), alloc);
            obj.AddMember(k, v, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, std::string& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsString()) {
                value = obj[key.c_str()].GetString();
            }
        }
    };

    // 特化 int
    template<>
    class PropertySerializer<int> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const int& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, value, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, int& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsInt()) {
                value = obj[key.c_str()].GetInt();
            }
        }
    };
    

    // 特化 double
    template<>
    class PropertySerializer<double> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const double& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, value, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, double& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsDouble()) {
                value = obj[key.c_str()].GetDouble();
            }
        }
    };

    // 特化 bool
    template<>
    class PropertySerializer<bool> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const bool& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, value, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, bool& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsBool()) {
                value = obj[key.c_str()].GetBool();
            }
        }
    };
    
    //特化Unt
    template<>
    class PropertySerializer<unsigned int> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const unsigned int& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, value, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, unsigned int& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsUint()) {
                value = obj[key.c_str()].GetUint();
            }
        }
    };
    
    //特化UInt64
    template<>
    class PropertySerializer<uint64_t> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const uint64_t& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, value, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, uint64_t& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsUint64()) {
                value = obj[key.c_str()].GetUint64();
            }
        }
    };
    
    //特化JSONObject
    template<>
    class PropertySerializer<JSONSerializable> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const JSONSerializable& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Document subDoc;
            subDoc.Parse(value.toJson().c_str());
            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, subDoc, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, JSONSerializable& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsObject()) {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                obj[key.c_str()].Accept(writer);
                value = JSONSerializable::fromJsonStatic(buffer.GetString());
            }
        }
    };
        
    //特化vector<JSONObject>
    template<>
    class PropertySerializer<std::vector<JSONSerializable>> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const std::vector<JSONSerializable>& value,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value arr(rapidjson::kArrayType);
            for (const auto& item : value) {
                rapidjson::Document subDoc;
                subDoc.Parse(item.toJson().c_str());
                arr.PushBack(subDoc, alloc);
            }

            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, arr, alloc);
        }

        static void deserialize(const rapidjson::Value& obj, const std::string& key, std::vector<JSONSerializable>& value) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsArray()) {
                value.clear();
                for (int i = 0; i < obj.Size(); i++) {
                    const rapidjson::Value& item = obj[key.c_str()][i];
                    if (item.IsObject()) {
                        rapidjson::StringBuffer buffer;
                        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                        item.Accept(writer);
                        value.push_back(JSONSerializable::fromJsonStatic(buffer.GetString()));
                    }
                }
            }
        }
    };
    
    // 添加枚举特化
    template<typename T>
    class PropertySerializer<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    public:
        static void serialize(rapidjson::Value& obj, const std::string& key, const T& value,
                              rapidjson::Document::AllocatorType& alloc) {
            EnumSerializer<T>::serialize(obj, key, value, alloc);
        }
        
        static void deserialize(const rapidjson::Value& obj, const std::string& key, T& value) {
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
        
        serializers.push_back([key, &value](rapidjson::Value& obj,
            rapidjson::Document::AllocatorType& alloc) {
            Serializer::serialize(obj, key, value, alloc);
        });

        deserializers.push_back([key, &value](const rapidjson::Value& obj) {
            Serializer::deserialize(obj, key, value);
        });
    }


    // 注册嵌套对象
    template<typename T>
    void registerNestedObject(const std::string& key, T& value) {
        static_assert(std::is_base_of<JSONSerializable<T>, T>::value,
            "Nested object must inherit from JSONSerializable");

        serializers.push_back([key, &value](rapidjson::Value& obj,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Document subDoc;
            subDoc.Parse(value.toJson().c_str());
            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, subDoc, alloc);
        });

        deserializers.push_back([key, &value](const rapidjson::Value& obj) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsObject()) {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                obj[key.c_str()].Accept(writer);
                value = T::fromJsonStatic(buffer.GetString());
            }
        });
    }
    
    //注册嵌套对象数组
    template<typename T>
    void registerNestedArray(const std::string& key, std::vector<T>& value) {
        static_assert(std::is_base_of<JSONSerializable<T>, T>::value,
            "Nested object must inherit from JSONSerializable");

        serializers.push_back([key, &value](rapidjson::Value& obj,
            rapidjson::Document::AllocatorType& alloc) {
            rapidjson::Value arr(rapidjson::kArrayType);
            for (const auto& item : value) {
                rapidjson::Document subDoc;
                subDoc.Parse(item.toJson().c_str());
                arr.PushBack(subDoc, alloc);
            }

            rapidjson::Value k(key.c_str(), alloc);
            obj.AddMember(k, arr, alloc);
        });

        deserializers.push_back([key, &value](const rapidjson::Value& obj) {
            if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsArray()) {
                value.clear();
                for (int i = 0; i < obj[key.c_str()].Size(); i++) {
                    const rapidjson::Value& item = obj[key.c_str()][i];
                    if (item.IsObject()) {
                        rapidjson::StringBuffer buffer;
                        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                        item.Accept(writer);
                        value.push_back(T::fromJsonStatic(buffer.GetString()));
                    }
                }
            }
        });
    }

public:
    virtual ~JSONSerializable() = default;

    // 序列化接口
    virtual std::string toJson() const {
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        for (const auto& serializer : serializers) {
            serializer(doc, alloc);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }

    // 反序列化接口
    virtual bool fromJson(const std::string& jsonStr) {
        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());

        if (doc.HasParseError()) {
            return false;
        }

        for (const auto& deserializer : deserializers) {
            deserializer(doc);
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

#endif /* rapid_json_wrapper_hpp */
