#ifndef _JSON_H_
#define _JSON_H_

// Incompatibilities
#if WIN32 || APPLE
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

#include "JSONValue.h"

class JSON
{
	friend class JSONValue;
	
	public:
		static JSONValue* Parse(const char *data);
		static std::string Stringify(const JSONValue *value);
	protected:
		static bool SkipWhitespace(const char **data);
		static bool ExtractString(const char **data, std::string &str);
		static double ParseInt(const char **data);
		static double ParseDecimal(const char **data);
	private:
		JSON();
};

#endif
