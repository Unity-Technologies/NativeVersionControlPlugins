#pragma once

// Incompatibilities
#ifdef WIN32
#undef isnan
static inline bool isnan(double x) { return x != x; }
static inline bool isinf(double x) { return !isnan(x) && isnan(x - x); }
#endif

#include <vector>
#include <string>
#include <map>

// Custom types
class JSONValue;
typedef std::vector<JSONValue*> JSONArray;
typedef std::map<std::string, JSONValue*> JSONObject;

class JSON;

enum JSONType { JSONType_Null, JSONType_String, JSONType_Bool, JSONType_Number, JSONType_Array, JSONType_Object };

class JSONValue
{
	friend class JSON;
	
public:
    JSONValue();
    JSONValue(const char *m_char_value);
    JSONValue(const std::string &m_string_value);
    JSONValue(bool m_bool_value);
    JSONValue(double m_number_value);
    JSONValue(const JSONArray &m_array_value);
    JSONValue(const JSONObject &m_object_value);
    ~JSONValue();
    
    bool IsNull() const;
    bool IsString() const;
    bool IsBool() const;
    bool IsNumber() const;
    bool IsArray() const;
    bool IsObject() const;
    
    const std::string &AsString() const;
    bool AsBool() const;
    double AsNumber() const;
    const JSONArray &AsArray() const;
    const JSONObject &AsObject() const;
    
    std::size_t CountChildren() const;
    bool HasChild(std::size_t index) const;
    JSONValue *Child(std::size_t index);
    bool HasChild(const char* name) const;
    JSONValue *Child(const char* name);
    
    std::string Stringify(bool pretty=false, int indent=0) const;
    
    inline operator const std::string&() { return AsString(); }
    inline operator bool() { return AsBool(); }
    inline operator double() { return AsNumber(); }
    inline operator int() { return (int)AsNumber(); }
    inline operator const JSONArray&() { return AsArray(); }
    inline operator const JSONObject&() { return AsObject(); }
    
protected:
    static JSONValue *Parse(const char **data);
    
private:
    static std::string StringifyString(const std::string &str);
	
    JSONType type;
    std::string string_value;
    bool bool_value;
    double number_value;
    JSONArray array_value;
    JSONObject object_value;
};

class JSON
{
	friend class JSONValue;
	
public:
    static JSONValue* Parse(const char *data);
    static std::string Stringify(const JSONValue *value, bool pretty=false, int indent=0);
protected:
    static bool SkipWhitespace(const char **data);
    static bool ExtractString(const char **data, std::string &str);
    static double ParseInt(const char **data);
    static double ParseDecimal(const char **data);
private:
    JSON();
};
