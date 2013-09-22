#ifndef _JSONVALUE_H_
#define _JSONVALUE_H_

#include <vector>
#include <string>

#include "JSON.h"

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

		std::string Stringify() const;

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

#endif
