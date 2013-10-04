#include "RESTInterface.h"
#include "Utility.h"
#include <vector>

using namespace std;

bool RESTInterface::ParseUrl(const string& url, string* server, string* path, map<string, string>* queryStringMap)
{
	size_t afterProtocol = url.find_first_of("://");
	if (afterProtocol == string::npos)
		return false;
	
	afterProtocol += 3;
	size_t serverPos = url.find_first_of("/", afterProtocol);

	if(server)
		*server = url.substr(0, serverPos);

	size_t queryStringPos = url.find_first_of("?", afterProtocol);

	if(path)
		*path = url.substr(serverPos, queryStringPos);

	if(queryStringPos != string::npos && queryStringMap != NULL)
	{
		
		string queryString = url.substr(queryStringPos + 1);
		vector<string>	parts;
		Tokenize(parts, queryString, "&");
		for (vector<string>::iterator i=parts.begin(); i != parts.end(); i++)
		{
			vector<string>	keyValue;
            Tokenize(keyValue, (*i), "=");
			if (keyValue.size() > 0 )
			{
				(*queryStringMap)[keyValue[0]] = keyValue.size() > 1 ? keyValue[1] : string("");
			}
		}
	}

	return true;
}