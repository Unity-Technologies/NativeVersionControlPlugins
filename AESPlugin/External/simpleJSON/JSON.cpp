#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <sstream>
#include <math.h>
#include "JSON.h"

// String functions
char ToLower(char v)
{
	if (v >= 'A' && v <= 'Z')
		return static_cast<char>(v | 0x20);
	else
		return v;
}

char ToUpper(char v)
{
	if (v >= 'a' && v <= 'z')
		return static_cast<char>(v & 0xdf);
	else
		return v;
}

int StrNICmp(const char * str1, const char * str2, size_t n)
{
	const char * p1 = (char *) str1;
	const char * p2 = (char *) str2;
	char c1, c2;
	size_t charsLeft=n;
	
	if( n <= 0 )
		return 0;
	
	while ((c1 = ToLower (*p1)) == (c2 = ToLower (*p2)))
	{
		++p1; ++p2; --charsLeft;
		if (!charsLeft || !c1)
			return 0;
	}
    
	return(c1 - c2);
}

// Macros to free an array/object
#define FREE_ARRAY(x) { JSONArray::iterator iter; for (iter = x.begin(); iter != x.end(); iter++) { delete *iter; } }
#define FREE_OBJECT(x) { JSONObject::iterator iter; for (iter = x.begin(); iter != x.end(); iter++) { delete (*iter).second; } }

/**
 * Blocks off the public constructor
 *
 * @access private
 *
 */
JSON::JSON()
{
}

/**
 * Parses a complete JSON encoded string (UNICODE input version)
 *
 * @access public
 *
 * @param char* data The JSON text
 *
 * @return JSONValue* Returns a JSON Value representing the root, or NULL on error
 */
JSONValue *JSON::Parse(const char *data)
{
	// Skip any preceding whitespace, end of data = no JSON = fail
	if (!SkipWhitespace(&data))
		return NULL;
    
	// We need the start of a value here now...
	JSONValue *value = JSONValue::Parse(&data);
	if (value == NULL)
		return NULL;
	
	// Can be white space now and should be at the end of the string then...
	if (SkipWhitespace(&data))
	{
		delete value;
		return NULL;
	}
	
	// We're now at the end of the string
	return value;
}

/**
 * Turns the passed in JSONValue into a JSON encode string
 *
 * @access public
 *
 * @param JSONValue* value The root value
 *
 * @return std::string Returns a JSON encoded string representation of the given value
 */
std::string JSON::Stringify(const JSONValue *value, bool pretty, int indent)
{
	if (value != NULL)
		return value->Stringify(pretty, indent);
	else
		return "";
}

/**
 * Skips over any whitespace characters (space, tab, \r or \n) defined by the JSON spec
 *
 * @access protected
 *
 * @param char** data Pointer to a char* that contains the JSON text
 *
 * @return bool Returns true if there is more data, or false if the end of the text was reached
 */
bool JSON::SkipWhitespace(const char **data)
{
	while (**data != 0 && (**data == ' ' || **data == '\t' || **data == '\r' || **data == '\n'))
		(*data)++;
	
	return **data != 0;
}

/**
 * Extracts a JSON String as defined by the spec - "<some chars>"
 * Any escaped characters are swapped out for their unescaped values
 *
 * @access protected
 *
 * @param char** data Pointer to a char* that contains the JSON text
 * @param std::string& str Reference to a std::string to receive the extracted string
 *
 * @return bool Returns true on success, false on failure
 */
bool JSON::ExtractString(const char **data, std::string &str)
{
	str = "";
	
	while (**data != 0)
	{
		// Save the char so we can change it if need be
		char next_char = **data;
		
		// Escaping something?
		if (next_char == '\\')
		{
			// Move over the escape char
			(*data)++;
			
			// Deal with the escaped char
			switch (**data)
			{
				case '"': next_char = '"'; break;
				case '\\': next_char = '\\'; break;
				case '/': next_char = '/'; break;
				case 'b': next_char = '\b'; break;
				case 'f': next_char = '\f'; break;
				case 'n': next_char = '\n'; break;
				case 'r': next_char = '\r'; break;
				case 't': next_char = '\t'; break;
                    
                    // By the spec, only the above cases are allowed
				default:
					return false;
			}
		}
		
		// End of the string?
		else if (next_char == '"')
		{
			(*data)++;
			str.reserve(); // Remove unused capacity
			return true;
		}
		
		// Disallowed char?
		else if (next_char < ' ' && next_char != '\t')
		{
			// SPEC Violation: Allow tabs due to real world cases
			return false;
		}
		
		// Add the next char
		str += next_char;
		
		// Move on
		(*data)++;
	}
	
	// If we're here, the string ended incorrectly
	return false;
}

/**
 * Parses some text as though it is an integer
 *
 * @access protected
 *
 * @param char** data Pointer to a char* that contains the JSON text
 *
 * @return double Returns the double value of the number found
 */
double JSON::ParseInt(const char **data)
{
	double integer = 0;
	while (**data != 0 && **data >= '0' && **data <= '9')
		integer = integer * 10 + (*(*data)++ - '0');
	
	return integer;
}

/**
 * Parses some text as though it is a decimal
 *
 * @access protected
 *
 * @param char** data Pointer to a char* that contains the JSON text
 *
 * @return double Returns the double value of the decimal found
 */
double JSON::ParseDecimal(const char **data)
{
	double decimal = 0.0;
    double factor = 0.1;
	while (**data != 0 && **data >= '0' && **data <= '9')
    {
        int digit = (*(*data)++ - '0');
		decimal = decimal + digit * factor;
        factor *= 0.1;
    }
	return decimal;
}

/**
 * Parses a JSON encoded value to a JSONValue object
 *
 * @access protected
 *
 * @param char** data Pointer to a char* that contains the data
 *
 * @return JSONValue* Returns a pointer to a JSONValue object on success, NULL on error
 */
JSONValue *JSONValue::Parse(const char **data)
{
	// Is it a string?
	if (**data == '"')
	{
		std::string str;
		if (!JSON::ExtractString(&(++(*data)), str))
			return NULL;
		else
			return new JSONValue(str);
	}
	
	// Is it a boolean?
	else if (StrNICmp(*data, "true", 4) == 0)
	{
		(*data) += 4;
		return new JSONValue(true);
	}
	else if (StrNICmp(*data, "false", 5) == 0)
	{
		(*data) += 5;
		return new JSONValue(false);
	}
	
	// Is it a null?
	else if (StrNICmp(*data, "null", 4) == 0)
	{
		(*data) += 4;
		return new JSONValue;
	}
	
	// Is it a number?
	else if (**data == '-' || (**data >= '0' && **data <= '9'))
	{
		// Negative?
		bool neg = **data == '-';
		if (neg) (*data)++;
        
		double number = 0.0;
        
		// Parse the whole part of the number - only if it wasn't 0
		if (**data == '0')
			(*data)++;
		else if (**data >= '1' && **data <= '9')
			number = JSON::ParseInt(data);
		else
			return NULL;
		
		// Could be a decimal now...
		if (**data == '.')
		{
			(*data)++;
            
			// Not get any digits?
			if (!(**data >= '0' && **data <= '9'))
				return NULL;
			
			// Find the decimal and sort the decimal place out
			// Use ParseDecimal as ParseInt won't work with decimals less than 0.1
			// thanks to Javier Abadia for the report & fix
			double decimal = JSON::ParseDecimal(data);
			
			// Save the number
			number += decimal;
		}
        
		// Could be an exponent now...
		if (**data == 'E' || **data == 'e')
		{
			(*data)++;
            
			// Check signage of expo
			bool neg_expo = false;
			if (**data == '-' || **data == '+')
			{
				neg_expo = **data == '-';
				(*data)++;
			}
			
			// Not get any digits?
			if (!(**data >= '0' && **data <= '9'))
				return NULL;
            
			// Sort the expo out
			double expo = JSON::ParseInt(data);
			for (double i = 0.0; i < expo; i++)
				number = neg_expo ? (number / 10.0) : (number * 10.0);
		}
        
		// Was it neg?
		if (neg) number *= -1;
        
		return new JSONValue(number);
	}
    
	// An object?
	else if (**data == '{')
	{
		JSONObject object;
		
		(*data)++;
        
		while (**data != 0)
		{
			// Whitespace at the start?
			if (!JSON::SkipWhitespace(data))
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			// Special case - empty object
			if (object.size() == 0 && **data == '}')
			{
				(*data)++;
				return new JSONValue(object);
			}
			
			// We want a string now...
			std::string name;
			if (!JSON::ExtractString(&(++(*data)), name))
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			// More whitespace?
			if (!JSON::SkipWhitespace(data))
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			// Need a : now
			if (*((*data)++) != ':')
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			// More whitespace?
			if (!JSON::SkipWhitespace(data))
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			// The value is here
			JSONValue *value = Parse(data);
			if (value == NULL)
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			// Add the name:value
			if (object.find(name) != object.end())
				delete object[name];
			object[name] = value;
			
			// More whitespace?
			if (!JSON::SkipWhitespace(data))
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			// End of object?
			if (**data == '}')
			{
				(*data)++;
				return new JSONValue(object);
			}
			
			// Want a , now
			if (**data != ',')
			{
				FREE_OBJECT(object);
				return NULL;
			}
			
			(*data)++;
		}
		
		// Only here if we ran out of data
		FREE_OBJECT(object);
		return NULL;
	}
	
	// An array?
	else if (**data == '[')
	{
		JSONArray array;
		
		(*data)++;
		
		while (**data != 0)
		{
			// Whitespace at the start?
			if (!JSON::SkipWhitespace(data))
			{
				FREE_ARRAY(array);
				return NULL;
			}
			
			// Special case - empty array
			if (array.size() == 0 && **data == ']')
			{
				(*data)++;
				return new JSONValue(array);
			}
			
			// Get the value
			JSONValue *value = Parse(data);
			if (value == NULL)
			{
				FREE_ARRAY(array);
				return NULL;
			}
			
			// Add the value
			array.push_back(value);
			
			// More whitespace?
			if (!JSON::SkipWhitespace(data))
			{
				FREE_ARRAY(array);
				return NULL;
			}
			
			// End of array?
			if (**data == ']')
			{
				(*data)++;
				return new JSONValue(array);
			}
			
			// Want a , now
			if (**data != ',')
			{
				FREE_ARRAY(array);
				return NULL;
			}
			
			(*data)++;
		}
		
		// Only here if we ran out of data
		FREE_ARRAY(array);
		return NULL;
	}
	
	// Ran out of possibilites, it's bad!
	else
	{
		return NULL;
	}
}

/**
 * Basic constructor for creating a JSON Value of type NULL
 *
 * @access public
 */
JSONValue::JSONValue(/*NULL*/)
{
	type = JSONType_Null;
}

/**
 * Basic constructor for creating a JSON Value of type String
 *
 * @access public
 *
 * @param char* m_char_value The string to use as the value
 */
JSONValue::JSONValue(const char *m_char_value)
{
	type = JSONType_String;
	string_value = std::string(m_char_value);
}

/**
 * Basic constructor for creating a JSON Value of type String
 *
 * @access public
 *
 * @param std::string m_string_value The string to use as the value
 */
JSONValue::JSONValue(const std::string &m_string_value)
{
	type = JSONType_String;
	string_value = m_string_value;
}

/**
 * Basic constructor for creating a JSON Value of type Bool
 *
 * @access public
 *
 * @param bool m_bool_value The bool to use as the value
 */
JSONValue::JSONValue(bool m_bool_value)
{
	type = JSONType_Bool;
	bool_value = m_bool_value;
}

/**
 * Basic constructor for creating a JSON Value of type Number
 *
 * @access public
 *
 * @param double m_number_value The number to use as the value
 */
JSONValue::JSONValue(double m_number_value)
{
	type = JSONType_Number;
	number_value = m_number_value;
}

/**
 * Basic constructor for creating a JSON Value of type Array
 *
 * @access public
 *
 * @param JSONArray m_array_value The JSONArray to use as the value
 */
JSONValue::JSONValue(const JSONArray &m_array_value)
{
	type = JSONType_Array;
	array_value = m_array_value;
}

/**
 * Basic constructor for creating a JSON Value of type Object
 *
 * @access public
 *
 * @param JSONObject m_object_value The JSONObject to use as the value
 */
JSONValue::JSONValue(const JSONObject &m_object_value)
{
	type = JSONType_Object;
	object_value = m_object_value;
}

/**
 * The destructor for the JSON Value object
 * Handles deleting the objects in the array or the object value
 *
 * @access public
 */
JSONValue::~JSONValue()
{
	if (type == JSONType_Array)
	{
		JSONArray::iterator iter;
		for (iter = array_value.begin(); iter != array_value.end(); iter++)
			delete *iter;
	}
	else if (type == JSONType_Object)
	{
		JSONObject::iterator iter;
		for (iter = object_value.begin(); iter != object_value.end(); iter++)
		{
			delete (*iter).second;
		}
	}
}

/**
 * Checks if the value is a NULL
 *
 * @access public
 *
 * @return bool Returns true if it is a NULL value, false otherwise
 */
bool JSONValue::IsNull() const
{
	return type == JSONType_Null;
}

/**
 * Checks if the value is a String
 *
 * @access public
 *
 * @return bool Returns true if it is a String value, false otherwise
 */
bool JSONValue::IsString() const
{
	return type == JSONType_String;
}

/**
 * Checks if the value is a Bool
 *
 * @access public
 *
 * @return bool Returns true if it is a Bool value, false otherwise
 */
bool JSONValue::IsBool() const
{
	return type == JSONType_Bool;
}

/**
 * Checks if the value is a Number
 *
 * @access public
 *
 * @return bool Returns true if it is a Number value, false otherwise
 */
bool JSONValue::IsNumber() const
{
	return type == JSONType_Number;
}

/**
 * Checks if the value is an Array
 *
 * @access public
 *
 * @return bool Returns true if it is an Array value, false otherwise
 */
bool JSONValue::IsArray() const
{
	return type == JSONType_Array;
}

/**
 * Checks if the value is an Object
 *
 * @access public
 *
 * @return bool Returns true if it is an Object value, false otherwise
 */
bool JSONValue::IsObject() const
{
	return type == JSONType_Object;
}

/**
 * Retrieves the String value of this JSONValue
 * Use IsString() before using this method.
 *
 * @access public
 *
 * @return std::string Returns the string value
 */
const std::string &JSONValue::AsString() const
{
	return string_value;
}

/**
 * Retrieves the Bool value of this JSONValue
 * Use IsBool() before using this method.
 *
 * @access public
 *
 * @return bool Returns the bool value
 */
bool JSONValue::AsBool() const
{
	return bool_value;
}

/**
 * Retrieves the Number value of this JSONValue
 * Use IsNumber() before using this method.
 *
 * @access public
 *
 * @return double Returns the number value
 */
double JSONValue::AsNumber() const
{
	return number_value;
}

/**
 * Retrieves the Array value of this JSONValue
 * Use IsArray() before using this method.
 *
 * @access public
 *
 * @return JSONArray Returns the array value
 */
const JSONArray &JSONValue::AsArray() const
{
	return array_value;
}

/**
 * Retrieves the Object value of this JSONValue
 * Use IsObject() before using this method.
 *
 * @access public
 *
 * @return JSONObject Returns the object value
 */
const JSONObject &JSONValue::AsObject() const
{
	return object_value;
}

/**
 * Retrieves the number of children of this JSONValue.
 * This number will be 0 or the actual number of children
 * if IsArray() or IsObject().
 *
 * @access public
 *
 * @return The number of children.
 */
std::size_t JSONValue::CountChildren() const
{
	switch (type)
	{
		case JSONType_Array:
			return array_value.size();
		case JSONType_Object:
			return object_value.size();
		default:
			return 0;
	}
}

/**
 * Checks if this JSONValue has a child at the given index.
 * Use IsArray() before using this method.
 *
 * @access public
 *
 * @return bool Returns true if the array has a value at the given index.
 */
bool JSONValue::HasChild(std::size_t index) const
{
	if (type == JSONType_Array)
	{
		return index < array_value.size();
	}
	else
	{
		return false;
	}
}

/**
 * Retrieves the child of this JSONValue at the given index.
 * Use IsArray() before using this method.
 *
 * @access public
 *
 * @return JSONValue* Returns JSONValue at the given index or NULL
 *                    if it doesn't exist.
 */
JSONValue *JSONValue::Child(std::size_t index)
{
	if (index < array_value.size())
	{
		return array_value[index];
	}
	else
	{
		return NULL;
	}
}

/**
 * Checks if this JSONValue has a child at the given key.
 * Use IsObject() before using this method.
 *
 * @access public
 *
 * @return bool Returns true if the object has a value at the given key.
 */
bool JSONValue::HasChild(const char* name) const
{
	if (type == JSONType_Object)
	{
		return object_value.find(name) != object_value.end();
	}
	else
	{
		return false;
	}
}

/**
 * Retrieves the child of this JSONValue at the given key.
 * Use IsObject() before using this method.
 *
 * @access public
 *
 * @return JSONValue* Returns JSONValue for the given key in the object
 *                    or NULL if it doesn't exist.
 */
JSONValue* JSONValue::Child(const char* name)
{
	JSONObject::const_iterator it = object_value.find(name);
	if (it != object_value.end())
	{
		return it->second;
	}
	else
	{
		return NULL;
	}
}

/**
 * Creates a JSON encoded string for the value with all necessary characters escaped
 *
 * @access public
 *
 * @return std::string Returns the JSON string
 */
std::string JSONValue::Stringify(bool pretty, int indent) const
{
	std::string ret_string;
	
	switch (type)
	{
		case JSONType_Null:
			ret_string = "null";
			break;
            
		case JSONType_String:
			ret_string = StringifyString(string_value);
			break;
            
		case JSONType_Bool:
			ret_string = bool_value ? "true" : "false";
			break;
            
		case JSONType_Number:
		{
			if (isinf(number_value) || isnan(number_value))
				ret_string = "null";
			else
			{
				std::stringstream ss;
				ss.precision(15);
				ss << number_value;
				ret_string = ss.str().c_str();
			}
			break;
		}
            
		case JSONType_Array:
		{
			ret_string = "[";
			if (pretty) ret_string += "\n";
			JSONArray::const_iterator iter = array_value.begin();
			while (iter != array_value.end())
			{
				if (pretty) ret_string += std::string(indent+1, '\t');
				ret_string += (*iter)->Stringify(pretty, indent+1);
				
				// Not at the end - add a separator
				if (++iter != array_value.end())
				{
					ret_string += ",";
					if (pretty) ret_string += "\n";
				}
			}
			if (pretty) ret_string += "\n" + std::string(indent, '\t');
			ret_string += "]";
			break;
		}
            
		case JSONType_Object:
		{
			ret_string = "{";
			if (pretty) ret_string += "\n";
			JSONObject::const_iterator iter = object_value.begin();
			while (iter != object_value.end())
			{
				if (pretty) ret_string += std::string(indent+1, '\t');
				ret_string += StringifyString((*iter).first);
				ret_string += ":";
				ret_string += (*iter).second->Stringify(pretty, indent+1);
				
				// Not at the end - add a separator
				if (++iter != object_value.end())
				{
					ret_string += ",";
					if (pretty) ret_string += "\n";
				}
			}
			if (pretty) ret_string += "\n" + std::string(indent, '\t');
			ret_string += "}";
			break;
		}
	}
    
	return ret_string;
}

/**
 * Creates a JSON encoded string with all required fields escaped
 * Works from http://www.ecma-internationl.org/publications/files/ECMA-ST/ECMA-262.pdf
 * Section 15.12.3.
 *
 * @access private
 *
 * @param std::string str The string that needs to have the characters escaped
 *
 * @return std::string Returns the JSON string
 */
std::string JSONValue::StringifyString(const std::string &str)
{
	std::string str_out = "\"";
	
	std::string::const_iterator iter = str.begin();
	while (iter != str.end())
	{
		char chr = *iter;
        
		if (chr == '"' || chr == '\\')
		{
			str_out += '\\';
			str_out += chr;
		}
		else if (chr == '\b')
		{
			str_out += "\\b";
		}
		else if (chr == '\f')
		{
			str_out += "\\f";
		}
		else if (chr == '\n')
		{
			str_out += "\\n";
		}
		else if (chr == '\r')
		{
			str_out += "\\r";
		}
		else if (chr == '\t')
		{
			str_out += "\\t";
		}
		else
		{
			str_out += chr;
		}
		
		iter++;
	}
	
	str_out += "\"";
	return str_out;
}
